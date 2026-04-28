
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
    bool blocked_by_teledata = false;
    bool blocked_by_telegate = false;
    bool blocked_by_cc = false;
    bool cat_entangled = false;
};

struct CommunicationQubitsPair {
    int q0;
    int q1;
    bool idle = true;
    std::string sendr_qpu; // QSEND and EXPOSE
    std::string recvr_qpu; // QRECV and RCONTROL
    std::string qcomm_protocol;
    int label;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::queue<UINT>> qc_meas_td;
    std::unordered_map<std::string, std::queue<UINT>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<UINT>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
};

std::vector<int> find_idle_communication_pairs(GlobalState& G, const size_t n_pairs)
{
    std::vector<int> indices_idle_pairs;
    size_t count = 0;
    for (int index = 0; index < G.communication_pairs.size() && count < n_pairs; index++) {
        if (G.communication_pairs[index].idle) {
            indices_idle_pairs.push_back(index);
            count++;
        } 
    } 

    if (count < n_pairs) 
        return std::vector<int>();

    for (const auto& index : indices_idle_pairs) {
        G.communication_pairs[index].idle = false;
    }

    return indices_idle_pairs;
}

std::vector<int> find_my_communication_pairs(const GlobalState& G, const std::string& sendr, const std::string recvr, const std::string qcomm_protocol, size_t n_pairs = 0)
{
    std::vector<int> comm_pairs;
    size_t count = 0;
    if (n_pairs == 0) n_pairs = G.communication_pairs.size();
    for (int index = 0; index < G.communication_pairs.size(); index++) {
        if (count == n_pairs) return comm_pairs;
        if (!G.communication_pairs[index].idle &&
            G.communication_pairs[index].sendr_qpu == sendr && 
            G.communication_pairs[index].recvr_qpu == recvr &&
            G.communication_pairs[index].qcomm_protocol == qcomm_protocol) {
                comm_pairs.push_back(index);
                count++;
        } 
    } 

    return comm_pairs;
}
 

std::string execute_shot_(
    QuantumState& state, 
    const std::vector<cunqa::QuantumTask>& quantum_tasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc,
    const size_t& n_comm_qubits
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
        T.blocked_by_teledata = false;
        T.blocked_by_telegate = false;
        T.blocked_by_cc = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.config.at("num_qubits").get<int>();
        G.n_clbits += quantum_task.config.at("num_clbits").get<int>();
    }
    
    // Here we add the communication qubits
    if (n_comm_qubits != 0) {
        G.n_qubits += n_comm_qubits;
        for (int i = 0; i < n_comm_qubits; i+=2) {
            CommunicationQubitsPair cqp = {
                .q0 = G.n_qubits - n_comm_qubits + i,
                .q1 = G.n_qubits - n_comm_qubits + i + 1
            };
            G.communication_pairs.push_back(cqp);
        }
    }

    auto generate_entanglement_ = [&](const size_t n_pairs) {
        std::vector<int> indices = find_idle_communication_pairs(G, n_pairs);

        if (!indices.empty()) {
            for (auto& index : indices) {
                UINT meas1 = measure_adapter(state, G.communication_pairs[index].q1);
                if (meas1) {
                    gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
                }
                UINT meas2 = measure_adapter(state, G.communication_pairs[index].q0);
                if (meas2) {
                    gate::X(G.communication_pairs[index].q0)->update_quantum_state(&state);
                }
                gate::H(G.communication_pairs[index].q0)->update_quantum_state(&state);
                gate::CNOT(G.communication_pairs[index].q0, G.communication_pairs[index].q1)->update_quantum_state(&state);
            }
        }
        
        return indices;
    };


    std::function<void(TaskState&, const cunqa::JSON&, const std::vector<int>)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}, const std::vector<int> comm_indices = {}) 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = cunqa::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case cunqa::MEASURE:
        {
            UINT measurement = measure_adapter(state, qubits[0] + T.zero_qubit);
            auto clbits = inst.at("clbits").get<std::vector<int>>();
            G.creg[clbits[0] + T.zero_clbit] = (measurement == 1);
            break;
        }
        case cunqa::COPY:
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
        case cunqa::ID:
            gate::Identity(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::X:
            gate::X(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::Y:
            gate::Y(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::Z:
            gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::H:
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::S:
            gate::S(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::SDG:
            gate::Sdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::T:
            gate::T(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::TDG:
            gate::Tdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::SX:
            gate::sqrtX(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::SXDG:
            gate::sqrtXdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::SY:
            gate::sqrtY(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::SYDG:
            gate::sqrtYdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::P0:
            gate::P0(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::P1:
            gate::P1(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case cunqa::U1: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U1(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::RX: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::RY: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::RZ: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::ROTINVX: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::ROTINVY: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::ROTINVZ: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::RotInvZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case cunqa::U2: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U2(qubits[0] + T.zero_qubit, params[0], params[1])->update_quantum_state(&state);
            break;
        }
        case cunqa::U3: 
        {
            auto params = inst.at("params").get<std::vector<double>>();
            gate::U3(qubits[0] + T.zero_qubit, params[0], params[1], params[2])->update_quantum_state(&state);
            break;
        }
        case cunqa::CX:
        {
            UINT control;
            if (qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = qubits[0] + T.zero_qubit;
            } 
            gate::CNOT(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::CZ:
        {
            UINT control;
            if (qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = qubits[0] + T.zero_qubit;
            } 
            gate::CZ(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::ECR:
        {
            gate::ECR(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::SWAP:
        {
            gate::SWAP(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case cunqa::FUSEDSWAP:
        {
            auto block_size = inst.at("block_size").get<unsigned int>();
            gate::FusedSWAP(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit, block_size)->update_quantum_state(&state);
            break;
        }
        case cunqa::MULTIPAULI:
        {
            auto pauli_id_list = inst.at("pauli_id_list").get<std::vector<unsigned int>>();
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i] + T.zero_qubit);
            }
            gate::Pauli(uiqubits, pauli_id_list)->update_quantum_state(&state);
            break;
        }
        case cunqa::MULTIPAULIROTATION:
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
        case cunqa::UNITARY:
        {
            auto cunqa_matrix = inst.at("matrix").get<std::vector<CunqaQulacsMatrix>>()[0];
            ComplexMatrix qulacs_matrix = cunqa::sim::cunqamatrix_to_qulacsdensematrix(cunqa_matrix);

            if (qubits.size() > 1) {
                std::vector<unsigned int> unsigned_qubits;
                for (size_t i = 0; i < qubits.size(); i++) {
                    if (qubits[i] < 0) {
                        for (auto& index : comm_indices) {
                            if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == qubits[i]) {
                                unsigned_qubits.push_back(G.communication_pairs[index].q1);
                                break;
                            }
                        }
                    } else {
                        unsigned_qubits.push_back(qubits[i] + T.zero_qubit);
                    }
                }
                gate::DenseMatrix(unsigned_qubits, qulacs_matrix)->update_quantum_state(&state);
            } else {
                gate::DenseMatrix(qubits[0] + T.zero_qubit, qulacs_matrix)->update_quantum_state(&state);
            }
            break;
        }
        case cunqa::SPARSEMATRIX:
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
        case cunqa::DIAGONAL:
        {   
            auto cunqa_diagonal = inst.at("matrix").get<std::vector<CunqaQulacsDiagonalMatrix>>()[0];
            ComplexVector qulacs_diagonal = cunqa::sim::cunqadiagonal_to_qulacsdiagonal(cunqa_diagonal);
            std::vector<unsigned int> unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                if (qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == qubits[i]) {
                            unsigned_qubits.push_back(G.communication_pairs[index].q1);
                            break;
                        }
                    }
                } else {
                    unsigned_qubits.push_back(qubits[i] + T.zero_qubit);
                }
            }

            gate::DiagonalMatrix(unsigned_qubits, qulacs_diagonal)->update_quantum_state(&state);
            break;
        }
        case cunqa::RANDOMUNITARY:
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
        case cunqa::BITFLIPNOISE:
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
        case cunqa::DEPHASINGNOISE:
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
        case cunqa::INDEPENDENTXZNOISE:
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
        case cunqa::DEPOLARIZINGNOISE:
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
        case cunqa::TWOQUBITDEPOLARIZINGNOISE:
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
        case cunqa::AMPLITUDEDAMPINGNOISE:
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
        case cunqa::SEND:
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
        case cunqa::RECV:
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
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }
            } else {
                for (const auto& clbit: clbits) {
                    int measurement = classical_channel->recv_measure(qpu_id);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case cunqa::CIF:
        {
            const auto& clbits = inst.at("clbits").get<std::vector<int>>();
            if (G.creg[clbits.at(0) + T.zero_clbit]) {
                for(const auto& sub_inst: inst.at("instructions")) {
                    apply_next_instr(T, sub_inst, {});
                }
            }
            break;
        }
        case cunqa::QSEND:
        {
            std::vector<int> indices = generate_entanglement_(1);
            if (indices.empty()) {
                T.blocked_by_teledata = true;
                return;
            }
            T.blocked_by_teledata = false;
            int index = indices[0];
            G.communication_pairs[index].qcomm_protocol = "teledata";

            // CX to the entangled pair
            gate::CNOT(qubits[0] + T.zero_qubit, G.communication_pairs[index].q0)->update_quantum_state(&state);

            // H to the sent qubit
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            UINT result = measure_adapter(state, qubits[0] + T.zero_qubit);

            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(measure_adapter(state, G.communication_pairs[index].q0));

            if (result) {
                gate::X(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            }

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[index].sendr_qpu = T.id;
            G.communication_pairs[index].recvr_qpu = inst.at("qpus")[0].get<std::string>();

            break;
        }
        case cunqa::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.at("qpus")[0]) || G.qc_meas_td[inst.at("qpus")[0]].empty()) {
                T.blocked_by_teledata = true;
                return;
            }

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas_td[inst.at("qpus")[0]].front();
            G.qc_meas_td[inst.at("qpus")[0]].pop();
            std::size_t meas2 = G.qc_meas_td[inst.at("qpus")[0]].front();
            G.qc_meas_td[inst.at("qpus")[0]].pop();

            std::vector<int> indices = find_my_communication_pairs(G, inst.at("qpus")[0], T.id, "teledata", 1);
            int index = indices[0];

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
            }
            if (meas2) {
                gate::Z(G.communication_pairs[index].q1)->update_quantum_state(&state);
            }

            // Swap the value to the desired qubit
            gate::SWAP(G.communication_pairs[index].q1, qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            G.communication_pairs[index].idle = true;
            break;
        }
        case cunqa::EXPOSE:
        {
            if (!T.cat_entangled) {
                std::vector<int> indices = generate_entanglement_(qubits.size());
                if (indices.empty()) {
                    T.blocked_by_telegate = true;
                    return;
                }

                int qid = 0;
                for (auto& index : indices) {
                    G.communication_pairs[index].qcomm_protocol = "telegate";
                    G.communication_pairs[index].label = -(qid + 1);

                    // CX to the entangled pair
                    gate::CNOT(qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0)->update_quantum_state(&state);

                    UINT result = measure_adapter(state, G.communication_pairs[index].q0);

                    G.qc_meas_tg[T.id].push(result);
                    T.cat_entangled = true;
                    T.blocked_by_telegate = true;
                    Ts[inst.at("qpus")[0]].blocked_by_telegate = false;

                    // Update communication pair
                    G.communication_pairs[index].sendr_qpu = T.id;
                    G.communication_pairs[index].recvr_qpu = inst.at("qpus")[0].get<std::string>();

                    qid++;
                }
                return;
            } else {
                for (int i = 0; i < qubits.size(); i++) {
                    UINT meas = G.qc_meas_tg[inst.at("qpus")[0]].front();
                    G.qc_meas_tg[inst.at("qpus")[0]].pop();

                    if (meas) {
                        gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
                    }
                }

                T.cat_entangled = false;

                std::vector<int> indices = find_my_communication_pairs(G, T.id, inst.at("qpus")[0], "telegate", qubits.size());
                for (auto& index : indices) {
                    G.communication_pairs[index].idle = true;
                }
            }
            break;
        }
        case cunqa::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.at("qpus")[0]) || G.qc_meas_tg[inst.at("qpus")[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            std::vector<int> indices = find_my_communication_pairs(G, inst.at("qpus")[0], T.id, "telegate");

            for (auto& index : indices) {
                UINT meas2 = G.qc_meas_tg[inst.at("qpus")[0]].front();
                G.qc_meas_tg[inst.at("qpus")[0]].pop();

                if (meas2) {
                    gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
                }
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                gate::H(G.communication_pairs[index].q1)->update_quantum_state(&state);

                UINT result = measure_adapter(state, G.communication_pairs[index].q1);
                G.qc_meas_tg[T.id].push(result);
            }

            Ts[inst.at("qpus")[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
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
            else if(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc) {
                G.ended = false;
                continue;
            }

            apply_next_instr(T, {}, {});

            if (!(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc))
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

    size_t n_qubits = 0;
    for (auto& quantum_task : qc.quantum_tasks) {
        n_qubits += quantum_task.config.at("num_qubits").get<size_t>();
    }

    size_t n_comm_qubits = 0;
    if (qc.quantum_tasks.size() > 1) { // Quantum Communications 
        if (qc.quantum_tasks[0].config.contains("n_communication_qubits")) {
            n_comm_qubits = qc.quantum_tasks[0].config.at("n_communication_qubits").get<size_t>();
            if (n_comm_qubits % 2 != 0) { // Ensure communication qubits always in pairs
                n_comm_qubits++;
            }
        } else {
            n_comm_qubits = 2;
        }

        n_qubits += n_comm_qubits;
    }    

    auto start = std::chrono::high_resolution_clock::now();
#ifdef OPENMP_IN_QC
    if (size(qc.quantum_tasks) > 1) { // Quantum communications 
        #pragma omp parallel
        {
            std::map<std::string, std::size_t> local_counter;
            
            QuantumState state(n_qubits);

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                local_counter[execute_shot_(state, qc.quantum_tasks, classical_channel, allows_qc,n_comm_qubits)]++;
                state.set_zero_state();
            }

            #pragma omp critical
            for (auto& [key, val] : local_counter)
                meas_counter[key] += val;
        }
    } else { // As if OPENMP_IN_QC not enabled
        QuantumState state(n_qubits);
        for (std::size_t i = 0; i < shots; i++) {
            meas_counter[execute_shot_(state, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
            state.set_zero_state();
        } // End all shots
    }
#else
    QuantumState state(n_qubits);
    for (std::size_t i = 0; i < shots; i++) {
        meas_counter[execute_shot_(state, qc.quantum_tasks, classical_channel, allows_qc,n_comm_qubits)]++;
        state.set_zero_state();
    } // End all shots
#endif
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