
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "qulacs_simulator_adapter.hpp"

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "cppsim/utility.hpp"

#include "qulacs_utils.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

namespace {

UINT measure_adapter(QuantumState& state, UINT target_index)
{
    Random random;
    auto gate0 = gate::P0(target_index);
    auto gate1 = gate::P1(target_index);
    std::vector<QuantumGateBase*> _gate_list = {gate0, gate1};
    double r = random.uniform();

    double sum = 0.;
    double org_norm = state.get_squared_norm();

    auto buffer = state.copy();
    UINT index = 0;
    for (auto gate : _gate_list) {
        gate->update_quantum_state(buffer);
        auto norm = buffer->get_squared_norm() / org_norm;
        sum += norm;
        if (r < sum) {
            state.load(buffer);
            state.normalize(norm);
            break;
        } else {
            buffer->load(&state);
            index++;
        }
    }

    delete gate0;
    delete gate1;
    delete buffer;

    return index;
}

struct LocalCCIDs {
    std::string sendr;
    std::string recvr;

    bool operator==(const LocalCCIDs& other) const {
        return sendr == other.sendr && recvr == other.recvr;
    }
}; // Struct to mimic classical communications when vQPUs deployed with quantum communications

struct LocalIDsHash {
    std::size_t operator()(const LocalCCIDs& local_cc_ids) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(local_cc_ids.sendr);
        std::size_t h2 = std::hash<std::string>{}(local_cc_ids.recvr);
        return h1 ^ (h2 << 1);
    }
};

struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    UINT zero_qubit = 0;
    UINT zero_clbit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::stack<UINT>> qc_meas;
    std::unordered_map<LocalCCIDs, std::queue<UINT>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
};
 
std::string execute_shot_(
    QuantumState& state, 
    const std::vector<cunqa::QuantumTask>& quantum_tasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc
)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (auto &quantum_task : quantum_tasks)
    {
        TaskState T;
        T.id = quantum_task.id;
        T.zero_qubit = G.n_qubits;
        T.zero_clbit = G.n_clbits;
        T.it = quantum_task.circuit.begin();
        T.end = quantum_task.circuit.end();
        T.blocked = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.config.at("num_qubits").get<int>();
        G.n_clbits += quantum_task.config.at("num_clbits").get<int>();
    }
    
    // Here we add the two communication qubits
    if (size(quantum_tasks) > 1)
        G.n_qubits += 2;

    auto generate_entanglement_ = [&]() {
        UINT meas1 = measure_adapter(state, G.n_qubits - 1);
        if (meas1) {
            gate::X(G.n_qubits - 1)->update_quantum_state(&state);
        }
        UINT meas2 = measure_adapter(state, G.n_qubits - 2);
        if (meas2) {
            gate::X(G.n_qubits - 2)->update_quantum_state(&state);
        }
        gate::H(G.n_qubits - 2)->update_quantum_state(&state);
        gate::CNOT(G.n_qubits - 2, G.n_qubits - 1)->update_quantum_state(&state);
    };


    std::function<void(TaskState&, const cunqa::JSON&)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}) 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = cunqa::constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case cunqa::constants::MEASURE:
        {
            UINT measurement = measure_adapter(state, qubits[0] + T.zero_qubit);
            auto clbits = inst.at("clbits").get<std::vector<int>>();
            G.creg[clbits[0] + T.zero_clbit] = (measurement == 1);
            break;
        }
        case cunqa::constants::COPY:
        {
            auto l_clbits = inst.at("l_clbits").get<std::vector<int>>();
            auto r_clbits = inst.at("r_clbits").get<std::vector<int>>();

            if(l_clbits.size() != r_clbits.size())
                throw std::runtime_error("The number of copied clbits and the number of clbits "
                                         "copied on does not match.");

            for (size_t i = 0; i < l_clbits.size(); ++i)
                G.creg[l_clbits[i] + T.zero_clbit] = G.creg[r_clbits[i] + T.zero_clbit];
                
            break;
        }
        case cunqa::constants::ID:
            gate::Identity(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::X:
            gate::X(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::Y:
            gate::Y(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::Z:
            gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::H:
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::S:
            gate::S(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::SDG:
            gate::Sdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::T:
            gate::T(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::TDG:
            gate::Tdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::SX:
            gate::sqrtX(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::SXDG:
            gate::sqrtXdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::SY:
            gate::sqrtY(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::SYDG:
            gate::sqrtYdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::P0:
            gate::P0(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::P1:
            gate::P1(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::constants::U1: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U1(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::RX: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::RY: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::RZ: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTINVX: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTINVY: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTINVZ: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTX: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTY: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ROTZ: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::U2: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U2(qubits[0] + T.zero_qubit, params[0], params[1])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::U3: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U3(qubits[0] + T.zero_qubit, params[0], params[1], params[2])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::CX:
        {
            UINT control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            gate::CNOT(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::CZ:
        {
            UINT control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            gate::CZ(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::ECR:
        {
            gate::ECR(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::SWAP:
        {
            gate::SWAP(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::FUSEDSWAP:
        {
            auto block_size = inst.at("block_size").get<unsigned int>();
            gate::FusedSWAP(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit, block_size)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::MULTIPAULI:
        {
            auto pauli_id_list = inst.at("pauli_id_list").get<std::vector<unsigned int>>();
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            gate::Pauli(uiqubits, pauli_id_list)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::MULTIPAULIROTATION:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            auto pauli_id_list = inst.at("pauli_id_list").get<std::vector<unsigned int>>();
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            gate::PauliRotation(uiqubits, pauli_id_list, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::UNITARY:
        {
            auto cunqa_matrix = inst.at("matrix").get<std::vector<CunqaQulacsMatrix>>()[0];
            ComplexMatrix qulacs_matrix = cunqa::sim::cunqamatrix_to_qulacsdensematrix(cunqa_matrix);

            if (qubits.size() > 1) {
                std::vector<unsigned int> uiqubits;
                for (int i = 0; i < qubits.size(); i++) {
                    uiqubits.push_back(qubits[i] + T.zero_qubit);
                }
                gate::DenseMatrix(uiqubits, qulacs_matrix)->update_quantum_state(&state);
            } else {
                gate::DenseMatrix(qubits[0] + T.zero_qubit, qulacs_matrix)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::SPARSEMATRIX:
        {
            auto cunqa_matrix = inst.at("matrix").get<std::vector<CunqaQulacsMatrix>>()[0];
            SparseComplexMatrix qulacs_sparse = cunqa::sim::cunqamatrix_to_sparse(cunqa_matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            gate::SparseMatrix(uiqubits, qulacs_sparse)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::DIAGONAL:
        {   
            auto cunqa_diagonal = inst.at("matrix").get<std::vector<CunqaQulacsDiagonalMatrix>>()[0];
            ComplexVector qulacs_diagonal = cunqa::sim::cunqadiagonal_to_qulacsdiagonal(cunqa_diagonal);
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            gate::DiagonalMatrix(uiqubits, qulacs_diagonal)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::RANDOMUNITARY:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::RandomUnitary(uiqubits, seed)->update_quantum_state(&state);
            } else {
                gate::RandomUnitary(uiqubits)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::BITFLIPNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::BitFlipNoise(qubits[0], prob, seed)->update_quantum_state(&state);
            } else {
                gate::BitFlipNoise(qubits[0], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::DEPHASINGNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::DephasingNoise(qubits[0], prob, seed)->update_quantum_state(&state);
            } else {
                gate::DephasingNoise(qubits[0], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::INDEPENDENTXZNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::IndependentXZNoise(qubits[0], prob, seed)->update_quantum_state(&state);
            } else {
                gate::IndependentXZNoise(qubits[0], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::DEPOLARIZINGNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::DepolarizingNoise(qubits[0], prob, seed)->update_quantum_state(&state);
            } else {
                gate::DepolarizingNoise(qubits[0], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::TWOQUBITDEPOLARIZINGNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::TwoQubitDepolarizingNoise(qubits[0], qubits[1], prob, seed)->update_quantum_state(&state);
            } else {
                gate::TwoQubitDepolarizingNoise(qubits[0], qubits[1], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::AMPLITUDEDAMPINGNOISE:
        {
            auto prob = inst.at("params").get<double>();
            if (inst.contains("seed")) {
                auto seed = inst.at("seed").get<unsigned int>();
                gate::AmplitudeDampingNoise(qubits[0], prob, seed)->update_quantum_state(&state);
            } else {
                gate::AmplitudeDampingNoise(qubits[0], prob)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::constants::SEND:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();   

            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = T.id, 
                    .recvr = Ts[qpu_id].id
                };  
                for (auto& clbit : clbits) {
                    G.local_cc_queue[local_cc_ids].push(G.creg[clbit + T.zero_clbit]);
                }
            } else {
                for (const auto& clbit: clbits) {
                    classical_channel->send_measure(G.creg[clbit + T.zero_clbit], qpu_id);
                }
            }
            break;
        }
        case cunqa::constants::RECV:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();

            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = Ts[qpu_id].id, 
                    .recvr = T.id
                };
                if (G.local_cc_queue.contains(local_cc_ids) && !G.local_cc_queue.at(local_cc_ids).empty()) {
                    for (const auto& clbit: clbits) {
                        G.creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked = false;
                } else {
                    T.blocked = true;
                }
            } else {
                for (const auto& clbit: clbits) {
                    int measurement = classical_channel->recv_measure(qpu_id);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case cunqa::constants::CIF:
        {
            const auto& clbits = inst.at("clbits").get<std::vector<int>>();
            if (G.creg[clbits.at(0) + T.zero_clbit]) {
                for(const auto& sub_inst: inst.at("instructions")) {
                    apply_next_instr(T, sub_inst);
                }
            }
            break;
        }
        case cunqa::constants::QSEND:
        {
            //------------- Generate Entanglement ---------------
            generate_entanglement_();
            //----------------------------------------------------

            // CX to the entangled pair
            gate::CNOT(qubits[0] + T.zero_qubit, G.n_qubits - 2)->update_quantum_state(&state);

            // H to the sent qubit
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            UINT result = measure_adapter(state, qubits[0] + T.zero_qubit);

            G.qc_meas[T.id].push(result);
            G.qc_meas[T.id].push(measure_adapter(state, G.n_qubits - 2));

            if (result) {
                gate::X(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            }

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked = false;
            break;
        }
        case cunqa::constants::QRECV:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();
            std::size_t meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                gate::X(G.n_qubits - 1)->update_quantum_state(&state);
            }
            if (meas2) {
                gate::Z(G.n_qubits - 1)->update_quantum_state(&state);
            }

            // Swap the value to the desired qubit
            gate::SWAP(G.n_qubits - 1, qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                gate::CNOT(qubits[0] + T.zero_qubit, G.n_qubits - 2)->update_quantum_state(&state);

                UINT result = measure_adapter(state, G.n_qubits - 2);

                G.qc_meas[T.id].push(result);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                UINT meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
                }

                T.cat_entangled = false;
            }
            break;
        }
        case cunqa::constants::RCONTROL:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0]) || G.qc_meas[inst.at("qpus")[0]].empty()) {
                T.blocked = true;
                return;
            }

            UINT meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                gate::X(G.n_qubits - 1)->update_quantum_state(&state);
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            gate::H(G.n_qubits - 1)->update_quantum_state(&state);

            UINT result = measure_adapter(state, G.n_qubits - 1);
            G.qc_meas[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked = false;
            T.blocked = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!" << "\n" << "Instruction that failed: " << inst.dump(4) << "\n";
        } // End switch
    };

    while (!G.ended)
    {
        G.ended = true;
        for (auto& [id, T]: Ts)
        {
            if (T.finished)
                continue;
            else if(T.blocked) {
                G.ended = false;
                continue;
            }

            apply_next_instr(T, {});

            if (!T.blocked)
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    std::string result_bits(G.n_clbits, '0');
    for (const auto &[bitIndex, value] : G.creg)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON QulacsSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Qulacs usual simulation");
    try {
        auto quantum_task = qc.quantum_tasks[0];

        size_t n_qubits = quantum_task.config.at("num_qubits").get<size_t>();
        auto shots = qc.quantum_tasks[0].config.at("shots").get<size_t>();
        JSON circuit_json = quantum_task.circuit;

        QuantumCircuit circuit(n_qubits);
        update_qulacs_circuit(circuit, circuit_json);

        QuantumState state(n_qubits);
        circuit.update_quantum_state(&state);

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<ITYPE> samples = state.sampling(shots);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts = convert_to_counts(samples, n_qubits);

        JSON result_json = 
        {
            {"counts", counts},
            {"time_taken", time_taken}
        };

        return result_json;

    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Qulacs simulator.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}


JSON QulacsSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Qulacs dynamic simulation");
    std::map<std::string, std::size_t> meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

    unsigned long n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks) {
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
    }

    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    QuantumState state(n_qubits);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++) {
        meas_counter[execute_shot_(state, qc.quantum_tasks, classical_channel, allows_qc)]++;
        state.set_zero_state();
    } // End all shots
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

} // End of sim namespace
} // End of cunqa namespace