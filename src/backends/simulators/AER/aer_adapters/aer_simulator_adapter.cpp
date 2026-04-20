
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>

#include "aer_simulator_adapter.hpp"

#include "simulators/circuit_executor.hpp"
#include "framework/config.hpp"
#include "noise/noise_model.hpp"
#include "framework/circuit.hpp"
#include "controllers/controller_execute.hpp"
#include "framework/results/result.hpp"
#include "controllers/aer_controller.hpp"
#include "controllers/state_controller.hpp"
#include "aer_helpers.hpp"

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
    unsigned long zero_qubit = 0;
    unsigned long zero_clbit = 0;
    bool finished = false;
    bool blocked_by_teledata = false;
    bool blocked_by_telegate = false;
    bool blocked_by_cc = false;
    bool cat_entangled = false;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::queue<uint_t>> qc_meas_td;
    std::unordered_map<std::string, std::queue<uint_t>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<uint_t>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
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


std::unordered_map<std::string, std::string> execute_shot_(
    AER::AerState* state, 
    std::vector<StructuredQuantumTask>& st_qtasks,
    comm::ClassicalChannel* classical_channel,
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
                state->apply_reset({G.communication_pairs[index].q1});
                state->apply_reset({G.communication_pairs[index].q0});
                state->apply_h(G.communication_pairs[index].q0);
                state->apply_mcx({G.communication_pairs[index].q0, G.communication_pairs[index].q1});
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
            uint_t measurement = state->apply_measure({inst.qubits[0] + T.zero_qubit});
            G.creg[inst.clbits[0] + T.zero_clbit] = (measurement == 1);
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
        case constants::X:
            state->apply_x(inst.qubits[0] + T.zero_qubit);
            break;
        case constants::Y:
            state->apply_y(inst.qubits[0] + T.zero_qubit);
            break;
        case constants::Z:
            state->apply_z(inst.qubits[0] + T.zero_qubit);
            break;
        case constants::H:
            state->apply_h(inst.qubits[0] + T.zero_qubit);
            break;
        case constants::SX:
            state->apply_mcsx({inst.qubits[0] + T.zero_qubit});
            break;
        case constants::RESET:
            state->apply_reset({inst.qubits[0] + T.zero_qubit});
            break;
        case constants::RX:
        {
            state->apply_mcrx({inst.qubits[0] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::RY:
        {
            state->apply_mcry({inst.qubits[0] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::RZ:
        {
            state->apply_mcrz({inst.qubits[0] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::U3:
        {
            state->apply_u(inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1], inst.params[2]);
            break;
        }
        case constants::CX:
        {
            unsigned long control;
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
            state->apply_mcx({control, inst.qubits[1] + T.zero_qubit});
            break;
        }
        case constants::CY:
        {
            unsigned long control;
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
            state->apply_mcy({control, inst.qubits[1] + T.zero_qubit});
            break;
        }
        case constants::CZ:
        {
            unsigned long control;
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
            state->apply_mcz({control, inst.qubits[1] + T.zero_qubit});
            break;
        }
        case constants::CRX:
        {
            unsigned long control;
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
            state->apply_mcrx({control, inst.qubits[1] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::CRY:
        {
            unsigned long control;
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
            state->apply_mcry({control, inst.qubits[1] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::CRZ:
        {
            unsigned long control;
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
            state->apply_mcrz({control, inst.qubits[1] + T.zero_qubit}, inst.params[0]);
            break;
        }
        case constants::CU:
        {
            unsigned long control;
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
            state->apply_cu({control, inst.qubits[1] + T.zero_qubit}, inst.params[0], inst.params[1], inst.params[2], inst.params[3]);
            break;
        }
        case constants::SWAP:
        {
            state->apply_mcswap({inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit});
            break;
        }
        case constants::MCX:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcx(unsigned_qubits);
            break;
        }
        case constants::MCY:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcy(unsigned_qubits);
            break;
        }
        case constants::MCZ:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcz(unsigned_qubits);
            break;
        }
        case constants::MCSX:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcsx(unsigned_qubits);
            break;
        }
        case constants::MCP:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcphase(unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::MCRX:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcrx(unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::MCRY:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcry(unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::MCRZ:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcrz(unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::MCU:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcu(unsigned_qubits, inst.params[0], inst.params[1], inst.params[2], inst.params[3]);
            break;
        }
        case constants::MCSWAP:
        {
            reg_t unsigned_qubits(inst.qubits.size());
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
            state->apply_mcswap(unsigned_qubits);
            break;
        }
        case constants::GLOBALP:
        {
            state->apply_global_phase(inst.params[0]);
            break;
        }
        case constants::UNITARY:
        {
            auto cunqa_matrix = inst.matrix[0];
            CUNQAComplexVector matrix_data;
            sim::convert_cunqa_matrix_to_complex_vector(cunqa_matrix, matrix_data);
            size_t dim = cunqa_matrix.size();
            matrix<complex_t> aer_matrix(dim, dim, matrix_data.data());
            reg_t unsigned_qubits;
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            unsigned_qubits.push_back(G.communication_pairs[index].q1);
                            break;
                        }
                    }
                } else {
                    unsigned_qubits.push_back(inst.qubits[i] + T.zero_qubit);
                }
            }
            state->apply_unitary(unsigned_qubits, aer_matrix);
            break;
        }
        case constants::DIAGONAL:
        {
            auto cunqa_diagonal = inst.diagonal[0];
            AER::cvector_t aer_diagonal;
            sim::convert_cunqadiagonal_to_aerdiagonal(cunqa_diagonal, aer_diagonal);
            reg_t unsigned_qubits;
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            unsigned_qubits.push_back(G.communication_pairs[index].q1);
                            break;
                        }
                    }
                } else {
                    unsigned_qubits.push_back(inst.qubits[i] + T.zero_qubit);
                }
            }
            state->apply_diagonal_matrix(unsigned_qubits, aer_diagonal);
            break;
        }
        case constants::MULTIPLEXER:
        {
            LOGGER_ERROR("Multiplexer instruction is not supported in CUNQA-AER");
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
                    state->flush_ops(); // Execute operations to empty the buffer 
                    for (const auto& clbit: inst.clbits) {
                        G.creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }   
            } else {
                state->flush_ops(); // Execute operations to empty the buffer 
                for (const auto& clbit: inst.clbits) {
                    int measurement = classical_channel->recv_measure(inst.qpus[0]);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case constants::CIF:
        {
            if ((bool)inst.condition == G.creg[inst.clbits[0] + T.zero_clbit]) {
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
            state->apply_mcx({inst.qubits[0] + T.zero_qubit, G.communication_pairs[index].q0});

            // H to the sent qubit
            state->apply_h(inst.qubits[0] + T.zero_qubit);

            uint_t result = state->apply_measure({inst.qubits[0] + T.zero_qubit});
            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(state->apply_measure({G.communication_pairs[index].q0}));

            if (result) {
                state->apply_reset({inst.qubits[0] + T.zero_qubit});
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
                state->apply_x(G.communication_pairs[index].q1);
            }
            if (meas2) {
                state->apply_z(G.communication_pairs[index].q1);
            }

            // Swap the value to the desired qubit
            state->apply_mcswap({G.communication_pairs[index].q1, inst.qubits[0] + T.zero_qubit});

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
                    state->apply_mcx({inst.qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0});

                    uint_t result = state->apply_measure({G.communication_pairs[index].q0});

                    G.qc_meas_tg[T.id].push(result);
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
                    uint_t meas = G.qc_meas_tg[inst.qpus[0]].front();
                    G.qc_meas_tg[inst.qpus[0]].pop();

                    if (meas) {
                        state->apply_z(inst.qubits[i] + T.zero_qubit); 
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
                uint_t meas2 = G.qc_meas_tg[inst.qpus[0]].front();
                G.qc_meas_tg[inst.qpus[0]].pop();

                if (meas2) {
                    state->apply_mcx({G.communication_pairs[index].q1});
                }
            }

            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                state->apply_h(G.communication_pairs[index].q1);

                uint_t result = state->apply_measure({G.communication_pairs[index].q1});
                G.qc_meas_tg[T.id].push(result);
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
            else if (T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc) {
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

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON AerSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Aer usual simulation");
    try {
        auto quantum_task = qc.quantum_tasks[0];

        JSON aer_quantum_task = quantum_task_to_AER(quantum_task);
        int n_clbits = quantum_task.config.at("num_clbits");

        Circuit circuit(aer_quantum_task);
        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        JSON run_config_json(aer_quantum_task.at("config").get<JSON>());
        if (quantum_task.config.contains("seed")) {
            run_config_json["seed_simulator"] = quantum_task.config.at("seed");
        }
        Config aer_config(run_config_json);
        Noise::NoiseModel noise_model(backend->config.at("noise_model"));

        Result result = controller_execute<Controller>(circuits, noise_model, aer_config);

        JSON result_json = result.to_json();
        convert_standard_results_Aer(result_json, n_clbits);

        return result_json;

    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the AER simulator.\n\tTry checking the format of the circuit sent and/or of the noise model.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}

AER::AerState get_configured_aer_state(const JSON& config);
JSON AerSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Aer dynamic simulation");
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  meas_counter;
    
    JSON qt_config = qc.quantum_tasks[0].config;
    auto shots = qt_config.at("shots").get<std::size_t>();
    std::string device = qt_config.at("device")["device_name"];
    reg_t target_gpus = (device == "GPU") ? qt_config.at("device")["target_devices"].get<reg_t>() : reg_t();

    std::vector<StructuredQuantumTask> st_qtasks;
    size_t n_qubits = 0;
    for (auto& quantum_task : qc.quantum_tasks) {
        st_qtasks.push_back(from_quantum_task_to_structuredqtask(quantum_task));
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
            std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> local_counter;

            AER::AerState state = get_configured_aer_state(qt_config);

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                reg_t qubit_ids = state.allocate_qubits(n_qubits);
                state.initialize();
                /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
                state.set_target_gpus(target_gpus);
                update_meas_counter(local_counter, execute_shot_(&state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
                state.clear();
            }

            #pragma omp critical
            for (const auto& [id, bitstrings_counter] : local_counter) {
                for (const auto& [bitstring, counts] : bitstrings_counter) {
                    meas_counter[id][bitstring] += counts;
                } 
            }
        }
    } else { // As if OPENMP_IN_QC not enabled
        AER::AerState state = get_configured_aer_state(qt_config);
        reg_t qubit_ids;
        for (std::size_t i = 0; i < shots; i++) {
            qubit_ids = state.allocate_qubits(n_qubits);
            state.initialize();
            /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
            state.set_target_gpus(target_gpus);
            update_meas_counter(meas_counter, execute_shot_(&state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
            state.clear();
        } // End all shots
    }
#else
    AER::AerState state = get_configured_aer_state(qt_config);
    reg_t qubit_ids;
    for (std::size_t i = 0; i < shots; i++) {
        qubit_ids = state.allocate_qubits(n_qubits);
        state.initialize();
        /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
        state.set_target_gpus(target_gpus);
        update_meas_counter(meas_counter, execute_shot_(&state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
        state.clear();
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

AER::AerState get_configured_aer_state(const JSON& config)
{
    AER::AerState state;

    std::string method = config.at("method").get<std::string>();
    std::string sim_method = (method == "automatic") ? "statevector" : method;
    std::string device = config.at("device")["device_name"];
    state.configure("method", sim_method);
    state.configure("device", device);
    state.configure("precision", "double");
    if (config.contains("seed")) {
        state.configure("seed_simulator", std::to_string(config.at("seed").get<int>()));
    }

    return state;
}

} // End of sim namespace
} // End of cunqa namespace
