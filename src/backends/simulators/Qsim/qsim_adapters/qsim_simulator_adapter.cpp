#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>
#include <random>

#include "qsim_simulator_adapter.hpp"

#include "seqfor.h"
#include "parfor.h"
#include <gates_qsim.h>
#include <gate_appl.h>
#include <simulator_basic.h>
/* #include <simulator_avx.h>
#include <simulator_sse.h> */

#include "utils/constants.hpp"

#include "logger.hpp"

namespace {
using namespace cunqa;

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

struct CommunicationQubitsPair {
    int q0;
    int q1;
    bool idle = true;
    std::string sendr_qpu; // QSEND and EXPOSE
    std::string recvr_qpu; // QRECV and RCONTROL
    std::string qcomm_protocol;
    int label;
};

struct TaskState {
    std::string id;
    int local_n_clbits = 0;
    std::vector<constants::CUNQAInstruction>::const_iterator it, end;
    int zero_qubit = 0;
    int zero_clbit = 0;
    bool finished = false;
    bool blocked_by_teledata = false;
    bool blocked_by_telegate = false;
    bool blocked_by_cc = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas;
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::queue<int>> qc_meas_td;
    std::unordered_map<std::string, std::queue<int>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<int>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
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

qsim::Matrix<float> cunqamatrix_to_qsimmatrix(const CUNQAMatrix& cunqa_matrix)
{
    size_t n = cunqa_matrix.size();
    if (n == 0) return {};

    qsim::Matrix<float> qsim_mat;
    qsim_mat.resize(2 * n * n);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            const auto& complex_val = cunqa_matrix[i][j];
            
            size_t base_idx = 2 * (n * i + j);

            if (complex_val.size() >= 2) {
                qsim_mat[base_idx]     = static_cast<float>(complex_val[0]); // Real
                qsim_mat[base_idx + 1] = static_cast<float>(complex_val[1]); // Imag
            } else if (complex_val.size() == 1) {
                qsim_mat[base_idx]     = static_cast<float>(complex_val[0]);
                qsim_mat[base_idx + 1] = 0.0f;
            }
        }
    }

    return qsim_mat;
}


std::unordered_map<std::string, std::string> execute_shot_(
    qsim::StateSpaceBasic<qsim::ParallelFor, float>& state_space,
    qsim::SimulatorBasic<qsim::ParallelFor>::State& state,
    qsim::SimulatorBasic<qsim::ParallelFor>& simulator,
    std::mt19937& rgen,
    std::vector<StructuredQuantumTask>& st_qtasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc, 
    const size_t& n_comm_qubits
)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (const auto &quantum_task : st_qtasks) {
        TaskState T;
        T.id = quantum_task.id;
        T.local_n_clbits = quantum_task.n_clbits;
        T.zero_qubit = G.n_qubits;
        T.zero_clbit = G.n_clbits;
        T.it = quantum_task.instructions.begin();
        T.end = quantum_task.instructions.end();
        T.blocked_by_teledata = false;
        T.blocked_by_telegate = false;
        T.blocked_by_cc = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.n_qubits;
        G.n_clbits += quantum_task.n_clbits;
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
                auto meas1 = state_space.Measure({G.communication_pairs[index].q1}, rgen, state);
                if (meas1.bitstring[0]) {
                    qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, G.communication_pairs[index].q1), state);
                } 
                auto meas2 = state_space.Measure({G.communication_pairs[index].q0}, rgen, state);
                if (meas2.bitstring[0]) {
                    qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, G.communication_pairs[index].q0), state);
                }
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, G.communication_pairs[index].q0), state);
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, G.communication_pairs[index].q0, G.communication_pairs[index].q1), state);
            }
        }

        return indices;
    };

    std::function<void(TaskState&, const std::optional<constants::CUNQAInstruction>&, const std::vector<int>)> apply_next_instr = 
        [&](TaskState& T, const std::optional<constants::CUNQAInstruction>& instruction = std::nullopt, const std::vector<int> comm_indices = {}) 
    {
        const CUNQAInstruction inst = !instruction.has_value() ? *T.it : instruction.value();
        auto inst_type = INSTRUCTIONS_MAP.at(inst.name);

        switch (inst_type)
        {
        case constants::MEASURE:
        {
            auto measure_result = state_space.Measure({inst.qubits[0] + T.zero_qubit}, rgen, state);
            G.creg[inst.clbits[0] + T.zero_clbit] = (measure_result.bitstring[0] == 1);
            break;
        }
        case constants::COPY:
        {
            if(inst.l_clbits.size() != inst.r_clbits.size())
                throw std::runtime_error("The number of copied clbits and the number of clbits "
                                         "copied on does not match.");

            for (size_t i = 0; i < inst.l_clbits.size(); ++i)
                G.creg[inst.l_clbits[i] + T.zero_clbit] = G.creg[inst.r_clbits[i] + T.zero_clbit];
                
            break;
        }
        case constants::ID:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId1<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::X:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::Y:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::Z:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::H:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::S:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateS<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::T:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateT<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::SX:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX2<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::SY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY2<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::HZ2:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHZ2<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            break;
        }
        case constants::RX:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRX<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.params[0]), state);
            break;
        }
        case constants::RY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRY<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.params[0]), state);
            break;
        }
        case constants::RZ:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRZ<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.params[0]), state);
            break;
        }
        case constants::ID2:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId2<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit), state);
            break;
        }
        case constants::CX:
        {
            int control;
            if (inst.qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = inst.qubits[0] + T.zero_qubit;
            } 

            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, control, inst.qubits[1] + T.zero_qubit), state);
            break;
        }
        case constants::CZ:
        {
            int control;
            if (inst.qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = inst.qubits[0] + T.zero_qubit;
            } 

            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCZ<float>::Create(0, control, inst.qubits[1] + T.zero_qubit), state);
            break;
        }
        case constants::SWAP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateSwap<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit), state);
            break;
        }
        case constants::ISWAP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateIS<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit), state);
            break;
        }
        case constants::CP:
        {
            int control;
            if (inst.qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = inst.qubits[0] + T.zero_qubit;
            } 

            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCP<float>::Create(0, control, inst.qubits[1] + T.zero_qubit, inst.params[0]), state);
            break;
        }
        case constants::RXY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRXY<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1]), state);
            break;
        }
        case constants::FS:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateFS<float>::Create(0, inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit, inst.params[0], inst.params[1]), state);
            break;
        }
        case constants::GLOBALP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateGPh<float>::Create(0, inst.params[0]), state);
            break;
        }
        case constants::UNITARY:
        {
            auto cunqa_matrix = inst.matrix[0];
            qsim::Matrix<float> qsim_matrix = cunqamatrix_to_qsimmatrix(cunqa_matrix);
            std::vector<unsigned> unsigned_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            unsigned_qubits[i] = G.communication_pairs[index].q1;
                            break;
                        }
                    }
                } else {
                    unsigned_qubits[i] = inst.qubits[i] + T.zero_qubit;
                }
            }
            if (inst.qubits.size() > 1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix2<float>::Create(0, unsigned_qubits[0], unsigned_qubits[1], std::move(qsim_matrix)), state);
            } else {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix1<float>::Create(0, unsigned_qubits[0], std::move(qsim_matrix)), state);
            }
            break;
        }
        case constants::CUNITARY:
        {
            auto cunqa_matrix = inst.matrix[0];
            size_t dim = cunqa_matrix.size();
            size_t ctrl_dim = 2 * dim;

            // Build controlled-U as a CUNQAMatrix, reusing cunqamatrix_to_qsimmatrix
            CUNQAMatrix ctrl_cunqa_matrix(ctrl_dim,
                std::vector<std::vector<double>>(ctrl_dim, {0.0, 0.0}));

            // Top-left block: Identity (control = |0>)
            for (size_t i = 0; i < dim; i++) {
                ctrl_cunqa_matrix[i][i] = {1.0, 0.0};
            }

            // Bottom-right block: U (control = |1>)
            for (size_t i = 0; i < dim; i++) {
                for (size_t j = 0; j < dim; j++) {
                    ctrl_cunqa_matrix[dim + i][dim + j] = cunqa_matrix[i][j];
                }
            }

            qsim::Matrix<float> ctrl_qsim_matrix = cunqamatrix_to_qsimmatrix(ctrl_cunqa_matrix);

            // Resolve qubits the same way as UNITARY case
            std::vector<unsigned> unsigned_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            unsigned_qubits[i] = G.communication_pairs[index].q1;
                            break;
                        }
                    }
                } else {
                    unsigned_qubits[i] = inst.qubits[i] + T.zero_qubit;
                }
            }

            // qubits[0] = control, qubits[1] = target
            // GateMatrix2::Create internally swaps them, consistent with UNITARY case
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(
                simulator,
                qsim::GateMatrix2<float>::Create(0, unsigned_qubits[0], unsigned_qubits[1], std::move(ctrl_qsim_matrix)),
                state);
            break;
        }
        case constants::SEND:
        {
            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = T.id, 
                    .recvr = Ts[inst.qpus[0]].id
                };  
                for (auto& clbit : inst.clbits) {
                    G.local_cc_queue[local_cc_ids].push(G.creg[clbit + T.zero_clbit]);
                }
            } else {
                for (const auto& clbit: inst.clbits) {
                    classical_channel->send_measure(G.creg[clbit + T.zero_clbit], inst.qpus[0]);
                }
            }
            break;
        }
        case constants::RECV:
        {
            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = Ts[inst.qpus[0]].id, 
                    .recvr = T.id
                };
                if (G.local_cc_queue.contains(local_cc_ids) && !G.local_cc_queue.at(local_cc_ids).empty()) {
                    for (const auto& clbit: inst.clbits) {
                        G.creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }    
            } else {
                for (const auto& clbit: inst.clbits) {
                    int measurement = classical_channel->recv_measure(inst.qpus[0]);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case constants::CIF:
        {
            std::unordered_map<std::string, std::function<bool(bool, bool)>> ops{
                {"and", [](bool a, bool b) { return a & b; }},
                {"or",  [](bool a, bool b) { return a | b; }},
                {"xor", [](bool a, bool b) { return a ^ b; }},
                {"0and", [](bool a, bool b) { return !a & !b; }},
                {"0or",  [](bool a, bool b) { return !a | !b; }},
                {"0xor", [](bool a, bool b) { return !a ^ !b; }}
            };
            // Operates on the values provided, with the specified operation.
            // If there is only one value, sum = G.creg[inst.clbits[0] + T.zero_clbit]
            bool result = std::accumulate(inst.clbits.begin() + 1, inst.clbits.end(), 
                           G.creg[inst.clbits[0] + T.zero_clbit],
                           [&](bool acc, int clbit) { 
                               return ops[inst.operation](acc, G.creg[clbit + T.zero_clbit]); 
                           });
            result = (static_cast<bool>(inst.condition)) ? result : !result;

            if (static_cast<bool>(inst.condition) == result) {
                for(const auto& sub_inst: inst.instructions) {
                    apply_next_instr(T, sub_inst, {});
                }
            }
            break;
        }
        case constants::QSEND:
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
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, inst.qubits[0] + T.zero_qubit, G.communication_pairs[index].q0), state);

            // H to the sent qubit
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);

            auto result1 = state_space.Measure({inst.qubits[0] + T.zero_qubit}, rgen, state);
            G.qc_meas_td[T.id].push(result1.bitstring[0]);
            auto result2 = state_space.Measure({G.communication_pairs[index].q0}, rgen, state);
            G.qc_meas_td[T.id].push(result2.bitstring[0]);

            if (result1.bitstring[0]) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
            }

            // Unlock QRECV
            Ts[inst.qpus[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[index].sendr_qpu = T.id;
            G.communication_pairs[index].recvr_qpu = inst.qpus[0];

            break;
        }
        case constants::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.qpus[0]) || G.qc_meas_td[inst.qpus[0]].empty()) {
                T.blocked_by_teledata = true;
                return;
            }
            if (T.blocked_by_teledata) return;

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();
            std::size_t meas2 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "teledata", 1);
            int index = indices[0];

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, G.communication_pairs[index].q1), state);
            }
            if (meas2) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, G.communication_pairs[index].q1), state);
            }

            // Swap the value to the desired qubit
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateSwap<float>::Create(0, G.communication_pairs[index].q1, inst.qubits[0] + T.zero_qubit), state);

            G.communication_pairs[index].idle = true;
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                std::vector<int> indices = generate_entanglement_(inst.qubits.size());
                if (indices.empty()) {
                    T.blocked_by_telegate = true;
                    return;
                }

                int qid = 0;
                for (auto& index : indices) {
                    G.communication_pairs[index].qcomm_protocol = "telegate";
                    G.communication_pairs[index].label = -(qid + 1);

                    // CX to the entangled pair
                    qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, inst.qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0), state);
                    auto result = state_space.Measure({G.communication_pairs[index].q0}, rgen, state);

                    G.qc_meas_tg[T.id].push(result.bitstring[0]);
                    T.cat_entangled = true;
                    T.blocked_by_telegate = true;
                    Ts[inst.qpus[0]].blocked_by_telegate = false;

                    // Update communication pair
                    G.communication_pairs[index].sendr_qpu = T.id;
                    G.communication_pairs[index].recvr_qpu = inst.qpus[0];

                    qid++;
                }
                return;
            } else {
                for (int i = 0; i < inst.qubits.size(); i++) {
                    int meas = G.qc_meas_tg[inst.qpus[0]].front();
                    G.qc_meas_tg[inst.qpus[0]].pop();

                    if (meas) {
                        qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, inst.qubits[0] + T.zero_qubit), state);
                    }
                }

                T.cat_entangled = false;

                std::vector<int> indices = find_my_communication_pairs(G, T.id, inst.qpus[0], "telegate", inst.qubits.size());
                for (auto& index : indices) {
                    G.communication_pairs[index].idle = true;
                }
            }
            break;
        }
        case constants::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.qpus[0]) || G.qc_meas_tg[inst.qpus[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "telegate");

            for (auto& index : indices) {
                int meas2 = G.qc_meas_tg[inst.qpus[0]].front();
                G.qc_meas_tg[inst.qpus[0]].pop();

                if (meas2) {
                    qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, G.communication_pairs[index].q1), state);
                }
            }

            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, G.communication_pairs[index].q1), state);

                auto result = state_space.Measure({G.communication_pairs[index].q1}, rgen, state);
                G.qc_meas_tg[T.id].push(result.bitstring[0]);
            }

            Ts[inst.qpus[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << inst.name << "\n";
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

            apply_next_instr(T, std::nullopt, {});

            if (!(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc))
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    std::unordered_map<std::string, std::string> shot_bits;
    for (auto& [id, T]: Ts) {
        std::string bitstring(T.local_n_clbits, '0');
        for (const auto &[bitIndex, value] : G.creg) {
            if (T.zero_clbit <= bitIndex && bitIndex < (T.zero_clbit + T.local_n_clbits)) {
                bitstring[T.local_n_clbits + T.zero_clbit - bitIndex - 1] = value ? '1' : '0';
            }
        }
        shot_bits[id] = bitstring;
    }

    return shot_bits;
}

void update_meas_counter(std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>& meas_counter, const std::unordered_map<std::string, std::string>& shot_bitstrings)
{
    for (const auto& [circ_id, bitstring] : shot_bitstrings) {
        meas_counter[circ_id][bitstring]++;
    }
}

void update_qsim_state(const JSON& circuit_json, qsim::SimulatorBasic<qsim::ParallelFor>& simulator, qsim::SimulatorBasic<qsim::ParallelFor>::State& state)
{
    for (const auto& instruction : circuit_json) {
        auto inst_type = INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        std::vector<unsigned> qubits = instruction.at("qubits").get<std::vector<unsigned>>();

        switch (inst_type)
        {
        case constants::MEASURE:
            LOGGER_DEBUG("Measure in Qsim usual simulation performed by sampling. Skiping.");
            break;
        case constants::ID:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId1<float>::Create(0, qubits[0]), state);
            break;
        case constants::X:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, qubits[0]), state);
            break;
        case constants::Y:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY<float>::Create(0, qubits[0]), state);
            break;
        case constants::Z:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, qubits[0]), state);
            break;
        case constants::H:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, qubits[0]), state);
            break;
        case constants::S:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateS<float>::Create(0, qubits[0]), state);
            break;
        case constants::T:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateT<float>::Create(0, qubits[0]), state);
            break;
        case constants::SX:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX2<float>::Create(0, qubits[0]), state);
            break;
        case constants::SY:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY2<float>::Create(0, qubits[0]), state);
            break;
        case constants::HZ2:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHZ2<float>::Create(0, qubits[0]), state);
            break;
        case constants::RX: 
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRX<float>::Create(0, qubits[0], params[0]), state);
            break;
        }
        case constants::RY: 
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRY<float>::Create(0, qubits[0], params[0]), state);
            break;
        }
        case constants::RZ: 
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRZ<float>::Create(0, qubits[0], params[0]), state);
            break;
        }
        case constants::ID2:
        qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId2<float>::Create(0, qubits[0], qubits[1]), state);
        break;
        case constants::CX:
        qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, qubits[0], qubits[1]), state);
        break;
        case constants::CZ:
        qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCZ<float>::Create(0, qubits[0], qubits[1]), state);
        break;
        case constants::SWAP:
        qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateSwap<float>::Create(0, qubits[0], qubits[1]), state);
        break;
        case constants::ISWAP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateIS<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case constants::CP:
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCP<float>::Create(0, qubits[0], qubits[1], params[0]), state);
            break;
        }
        case constants::RXY: 
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRXY<float>::Create(0, qubits[0], params[0], params[1]), state);
            break;
        }
        case constants::FS:
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateFS<float>::Create(0, qubits[0], qubits[1], params[0], params[1]), state);
            break;
        }
        case constants::GLOBALP:
        {
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateGPh<float>::Create(0, params[0]), state);
            break;
        }
        case constants::UNITARY:
        {
            auto cunqa_matrix = instruction.at("matrix").get<std::vector<CUNQAMatrix>>()[0];
            qsim::Matrix<float> qsim_matrix = cunqamatrix_to_qsimmatrix(cunqa_matrix);

            if (qubits.size() > 1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix2<float>::Create(0, qubits[0], qubits[1], std::move(qsim_matrix)), state);
            } else {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix1<float>::Create(0, qubits[0], std::move(qsim_matrix)), state);
            }
            break;
        }
        case constants::CUNITARY:
        {
            auto cunqa_matrix = instruction.at("matrix").get<std::vector<CUNQAMatrix>>()[0];
            size_t dim = cunqa_matrix.size();
            size_t ctrl_dim = 2 * dim;

            CUNQAMatrix ctrl_cunqa_matrix(ctrl_dim,
                std::vector<std::vector<double>>(ctrl_dim, {0.0, 0.0}));

            for (size_t i = 0; i < dim; i++) {
                ctrl_cunqa_matrix[i][i] = {1.0, 0.0};
            }

            for (size_t i = 0; i < dim; i++) {
                for (size_t j = 0; j < dim; j++) {
                    ctrl_cunqa_matrix[dim + i][dim + j] = cunqa_matrix[i][j]; 
                }
            }

            qsim::Matrix<float> ctrl_qsim_matrix = cunqamatrix_to_qsimmatrix(ctrl_cunqa_matrix);

            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(
                simulator,
                qsim::GateMatrix2<float>::Create(0, qubits[0], qubits[1], std::move(ctrl_qsim_matrix)),
                state);
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << instruction.at("name") << "\n";
        };
    }

}

JSON convert_qsim_result(const std::vector<uint64_t>& sample, const int n_qubits) {
    std::unordered_map<uint64_t, int> counts;
    for (uint64_t v : sample)
        counts[v]++;

    JSON result_json;
    for (const auto& [value, count] : counts) {
        std::string bitstring(n_qubits, '0');
        for (int i = 0; i < n_qubits; ++i)
            bitstring[n_qubits - 1 - i] = ((value >> i) & 1) ? '1' : '0';

        result_json[bitstring] = count;
    }
    return result_json;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON QsimSimulatorAdapter::simulate([[maybe_unused]] const Backend* backend)
{
    LOGGER_DEBUG("Qsim usual simulation");
    try
    { 
        auto quantum_task = qc.quantum_tasks[0];
        auto n_qubits = quantum_task.config.at("num_qubits").get<unsigned>();
        auto shots = quantum_task.config.at("shots").get<uint64_t>();
        unsigned seed = 0;
        if (quantum_task.config.contains("seed")) {
            seed = quantum_task.config.at("seed").get<unsigned>();
        }
        const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
        unsigned num_threads = 1;
        if (num_threads_char != nullptr) {
            num_threads = std::stoi(num_threads_char);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
        qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(n_qubits); 
        state_space.SetStateZero(state);
        qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);
        
        JSON circuit_json = quantum_task.circuit;
        update_qsim_state(circuit_json, simulator, state);
        std::vector<uint64_t> results = state_space.Sample(state, shots, seed);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts_json = convert_qsim_result(results, n_qubits);
        
        JSON result_json = {
            {"counts", counts_json},
            {"time_taken", time_taken}};

        return result_json;
    } 
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Qsim simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return JSON();
}

JSON QsimSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Qsim dynamic simulation");
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  meas_counter;

    JSON config = qc.quantum_tasks[0].config;
    auto shots = config.at("shots").get<int>();

    std::vector<StructuredQuantumTask> st_qtasks;
    size_t n_qubits = 0;
    for (auto& quantum_task : qc.quantum_tasks) {
        st_qtasks.push_back(from_quantum_task_to_structuredqtask(quantum_task));
        n_qubits += quantum_task.config.at("num_qubits").get<size_t>();
    }

    size_t n_comm_qubits = 0;
    if (qc.quantum_tasks.size() > 1) { // Quantum Communications 
        if (config.contains("n_communication_qubits")) {
            n_comm_qubits = config.at("n_communication_qubits").get<size_t>();
            if (n_comm_qubits % 2 != 0) { // Ensure communication qubits always in pairs
                n_comm_qubits++;
            }
        } else {
            n_comm_qubits = 2;
        }

        n_qubits += n_comm_qubits;
    }

    unsigned seed = 0;
    if (config.contains("seed")) {
        seed = config.at("seed").get<unsigned>();
    }
    std::mt19937 rgen(seed);

    const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
    unsigned num_threads = 1;
    if (num_threads_char != nullptr) {
        num_threads = std::stoi(num_threads_char);
    }


    auto start = std::chrono::high_resolution_clock::now();
#ifdef OPENMP_IN_QC
    if (size(qc.quantum_tasks) > 1) { // Quantum communications 
        #pragma omp parallel
        {
            std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  local_counter;
            
            qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
            qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(n_qubits); 
            qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);
            
            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                state_space.SetStateZero(state);
                update_meas_counter(local_counter, execute_shot_(state_space, state, simulator, rgen, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
            }

            #pragma omp critical
            for (const auto& [id, bitstrings_counter] : local_counter) {
                for (const auto& [bitstring, counts] : bitstrings_counter) {
                    meas_counter[id][bitstring] += counts;
                } 
            }
        }
    } else { // As if OPENMP_IN_QC not enabled
        qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
        qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(n_qubits); 
        qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);
        for (int i = 0; i < shots; i++) {
            state_space.SetStateZero(state);
            update_meas_counter(meas_counter, execute_shot_(state_space, state, simulator, rgen, st_qtasks, classical_channel, allows_qc, n_comm_qubits));            
        } // End all shots
    }
#else
    qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
    qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(n_qubits); 
    qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);
    for (int i = 0; i < shots; i++) {
        state_space.SetStateZero(state);
        update_meas_counter(meas_counter, execute_shot_(state_space, state, simulator, rgen, st_qtasks, classical_channel, allows_qc, n_comm_qubits));        
    } // End all shots
#endif
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result_json = {
        {"id_counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}


} // End of sim namespace
} // End of cunqa namespace