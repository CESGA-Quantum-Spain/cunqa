
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

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
            gate::Identity(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::X:
            gate::X(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::Y:
            gate::Y(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::Z:
            gate::Z(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::H:
            gate::H(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::S:
            gate::S(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::SDG:
            gate::Sdag(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::T:
            gate::T(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::TDG:
            gate::Tdag(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::SX:
            gate::sqrtX(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::SXDG:
            gate::sqrtXdag(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::SY:
            gate::sqrtY(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::SYDG:
            gate::sqrtYdag(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::P0:
            gate::P0(payload.qubit)->update_quantum_state(&state);
            break;

        case InstructionType::P1:
            gate::P1(payload.qubit)->update_quantum_state(&state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::U1:
            gate::U1(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::RX:
            gate::RX(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::RY:
            gate::RY(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::RZ:
            gate::RZ(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::ROTINVX:
            gate::RotInvX(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::ROTINVY:
            gate::RotInvY(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        case InstructionType::ROTINVZ:
            gate::RotInvZ(payload.qubit, payload.param)->update_quantum_state(&state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CX:
            gate::CNOT(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state);
            break;

        case InstructionType::CZ:
            gate::CZ(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state);
            break;

        case InstructionType::ECR:
            gate::ECR(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state);
            break;

        case InstructionType::SWAP:
            gate::SWAP(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state);
            break;

        

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const FusedSwap& payload)
{
    switch (type)
    {
        case InstructionType::FUSEDSWAP:
            gate::FusedSWAP(payload.qubits[0], payload.qubits[1], payload.block_size)->update_quantum_state(&state);
            break;        

        default:
            unsupported_gate(type, payload);
    }
}





JSON QulacsSimulatorAdapter::native_execute(const Backend* backend)
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