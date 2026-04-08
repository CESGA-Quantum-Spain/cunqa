
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>

#include "utils/constants.hpp"
#include "utils/helpers/reverse_bitstring.hpp"
#include "utils/helpers/json_to_qasm2.hpp"

#include "maestro_simulator_adapter.hpp"
#include "maestrolib/Interface.h"

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
    std::map<std::size_t, bool> local_creg;
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
    std::map<std::size_t, bool> global_creg;
    std::unordered_map<std::string, std::queue<int>> qc_meas_td;
    std::unordered_map<std::string, std::queue<int>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<int>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
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
    void* simulator, 
    std::vector<StructuredQuantumTask>& st_qtasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc, 
    const size_t& n_comm_qubits
)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    int qt_count = 0;
    for (auto &quantum_task : st_qtasks) {
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
        if (Ts.count(quantum_task.id)) {
            quantum_task.id += "_" + std::to_string(qt_count); 
        }
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.n_qubits;
        G.n_clbits += quantum_task.n_clbits;

        qt_count++;
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
                const unsigned long int q[]{ G.communication_pairs[index].q1, G.communication_pairs[index].q0 };
                ApplyReset(simulator, q, 2);
                ApplyH(simulator, G.communication_pairs[index].q0);
                ApplyCX(simulator, G.communication_pairs[index].q0, G.communication_pairs[index].q1);
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
            const unsigned long int q[]{ inst.qubits[0] + T.zero_qubit };
            const unsigned long long int measurement = Measure(simulator, q, 1);

            T.local_creg[inst.clbits[0]] = (measurement == 1);
            break;
        }
        case constants::COPY:
        {
            if(inst.l_clbits.size() != inst.r_clbits.size())
                throw std::runtime_error("The number of copied clbits and the number of clbits "
                                         "copied on does not match.");

            for (size_t i = 0; i < inst.l_clbits.size(); ++i)
                G.global_creg[inst.l_clbits[i] + T.zero_clbit] = G.global_creg[inst.r_clbits[i] + T.zero_clbit];
                
            break;
        }
        case constants::X:
            ApplyX(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::Y:
            ApplyY(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::Z:
            ApplyZ(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::H:
            ApplyH(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::S:
            ApplyS(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::SDG:
            ApplySDG(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::T:
            ApplyT(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::TDG:
            ApplyTDG(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::SX:
            ApplySX(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::K:
            ApplyK(simulator, inst.qubits[0] + T.zero_qubit);
            break;
        case constants::P:
        {
            ApplyP(simulator, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RX:
        {
            ApplyRx(simulator, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RY:
        {
            ApplyRy(simulator, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RZ:
        {
            ApplyRz(simulator, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::U:
        {
            ApplyU(simulator, inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1], inst.params[2], inst.params[3]);
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
            ApplyCX(simulator, control, inst.qubits[1] + T.zero_qubit);
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
            ApplyCY(simulator, control, inst.qubits[1] + T.zero_qubit);
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
            ApplyCZ(simulator, control, inst.qubits[1] + T.zero_qubit);
            break;
        }
        case constants::CH:
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
            ApplyCH(simulator, control, inst.qubits[1] + T.zero_qubit);
            break;
        }
        case constants::CSX:
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
            ApplyCSX(simulator, control, inst.qubits[1] + T.zero_qubit);
            break;
        }
        case constants::CSXDG:
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
            ApplyCSXDG(simulator, control, inst.qubits[1] + T.zero_qubit);
            break;
        }
        case constants::SWAP:
        {
            ApplySwap(simulator, inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit);
            break;
        }
        case constants::ECR:
            // TODO
            break;
        case constants::CP:
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
            ApplyCP(simulator, control, inst.qubits[1] + T.zero_qubit, inst.params[0]);
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
            ApplyCRx(simulator, control, inst.qubits[1] + T.zero_qubit, inst.params[0]);
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
            ApplyCRy(simulator, control, inst.qubits[1] + T.zero_qubit, inst.params[0]);
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
            ApplyCRz(simulator, control, inst.qubits[1] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::CCX:
        {
            std::vector<int> tmp_qubits;
            for (int i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[0] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            tmp_qubits[i] = G.communication_pairs[index].q1;
                            break;
                        }
                    }
                } else {
                    tmp_qubits[i] = inst.qubits[i] + T.zero_qubit;
                }
            }
            ApplyCCX(simulator, tmp_qubits[0], tmp_qubits[1], tmp_qubits[2]);
            break;
        }
        case constants::CSWAP:
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
            ApplyCSwap(simulator, control, inst.qubits[1] + T.zero_qubit, inst.qubits[2] + T.zero_qubit);
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
            ApplyCU(simulator, control, inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1], inst.params[2], inst.params[3]);
            break;
        }
        case constants::RESET:
        {
            std::vector<unsigned long int> uliqubits(
                inst.qubits.begin(), inst.qubits.end()
            );
		    ApplyReset(simulator, uliqubits.data(), inst.qubits.size());
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
                    G.local_cc_queue[local_cc_ids].push(G.global_creg[clbit + T.zero_clbit]);
                }
            } else {
                for (const auto& clbit: inst.clbits) {
                    classical_channel->send_measure(G.global_creg[clbit + T.zero_clbit], inst.qpus[0]);
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
                        G.global_creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }
            } else {
                for (const auto& clbit: inst.clbits) {
                    int measurement = classical_channel->recv_measure(inst.qpus[0]);
                    G.global_creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case constants::CIF:
        {
            const auto& clbits = inst.clbits;
            if (G.global_creg[inst.clbits[0] + T.zero_clbit]) {
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
            ApplyCX(simulator, inst.qubits[0] + T.zero_qubit, G.communication_pairs[index].q0);

            // H to the sent qubit
            ApplyH(simulator, inst.qubits[0] + T.zero_qubit);

            const unsigned long int q1[]{ inst.qubits[0] + T.zero_qubit };
            int measurement_as_int = static_cast<int>(Measure(simulator, q1, 1));
            G.qc_meas_td[T.id].push(measurement_as_int);

            const unsigned long int q2[]{ G.communication_pairs[index].q0 };
            int aux_meas = static_cast<int>(Measure(simulator, q2, 1));
            G.qc_meas_td[T.id].push(aux_meas);

            if (measurement_as_int) {
                const unsigned long int q3[]{ inst.qubits[0] + T.zero_qubit };
                ApplyReset(simulator, q3, 1);
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
                ApplyX(simulator, G.communication_pairs[index].q1);
            }
            if (meas2) {
                ApplyZ(simulator, G.communication_pairs[index].q1);
            }

            // Swap the value to the desired qubit
            ApplySwap(simulator, G.communication_pairs[index].q1, inst.qubits[0] + T.zero_qubit);

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
                    ApplyCX(simulator, inst.qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0);

                    const unsigned long int q[]{ G.communication_pairs[index].q0 };
                    int measurement_as_int = static_cast<int>(Measure(simulator, q, 1));

                    G.qc_meas_tg[T.id].push(measurement_as_int);
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
                        ApplyZ(simulator, inst.qubits[0] + T.zero_qubit);
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
                    ApplyX(simulator, G.communication_pairs[index].q1);
                }
            }

            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                ApplyH(simulator, G.communication_pairs[index].q1);

                const unsigned long int q[]{ G.communication_pairs[index].q1 };
                int measurement_as_int = static_cast<int>(Measure(simulator, q, 1));
                G.qc_meas_tg[T.id].push(measurement_as_int);
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
        std::string result_bits(T.local_n_clbits, '0');
        for (const auto &[bitIndex, value] : T.local_creg) {
            result_bits[T.local_n_clbits - bitIndex - 1] = value ? '1' : '0';
        }
        shot_bits[id] = result_bits;
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

MaestroSimulatorAdapter::MaestroSimulatorAdapter() 
{
    maestroInstance = GetMaestroObject();
}

MaestroSimulatorAdapter::MaestroSimulatorAdapter(MaestroComputationAdapter& qc) : qc{qc} 
{
    maestroInstance = GetMaestroObject();
}

JSON MaestroSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Maestro usual simulation");
    try {
        auto quantum_task = qc.quantum_tasks[0];
        auto n_qbits = quantum_task.config.at("num_qubits").get<unsigned long>();
 
        JSON circuit_json = quantum_task.circuit;
        JSON run_config_json(quantum_task.config);

        auto simulatorHandle = CreateSimpleSimulator(n_qbits);
        if (simulatorHandle == 0)
        {
            LOGGER_ERROR("Error creating the Maestro SimpleSimulator.");
            return {{"ERROR", "Unable to create the Maestro SimpleSimulator."}};
        }

        std::string method = quantum_task.config.at("method").get<std::string>();
        std::string sim_name;

        if (quantum_task.config.contains("simulator"))
            sim_name = quantum_task.config.at("simulator").get<std::string>();

        // -1 for simulator type means both qiskit aer and qcsim
        // -1 for simulation type means automatic, that is... statevector + stabilizer + matrix product state
        int simulatorType = -1; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
        int simulationType = -1; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state

        // TODO: set the method into the estimator
        // also the parameters if any and so on
        if (method != "automatic")
        {
            if (method == "statevector")
            {
                simulationType = 0;
            }
            else if (method == "matrix_product_state")
            {
                // matrix_product_state_truncation_threshold
                // matrix_product_state_max_bond_dimension
                // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
                simulationType = 1;
            }
            else if (method == "stabilizer")
            {
                simulationType = 2;
            }
            else if (method == "tensor_network")
            {
                // use qcsim for this, qiskit aer is not compiled with tensor network support
                // in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
                simulationType = 3;
            }
        }

        if (sim_name == "qiskit" || sim_name == "aer")
        {
            simulatorType = 0; // qiskit aer
        }
        else if (sim_name == "qcsim")
        {
            simulatorType = 1; // qcsim
        }
        else if (sim_name == "gpu" && simulationType != 2 && simulationType != 3) // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
        {
            simulatorType = 4; // gpu
        }
        else if (sim_name == "composite_qiskit")
        {
            simulatorType = 2; // p-blocks qiskit aer
            simulationType = 0; // statevector
        }
        else if (sim_name == "composite_qcsim")
        {
            simulatorType = 3; // p-blocks qcsim
            simulationType = 0; // statevector
        }

        if (simulatorType != -1 || simulationType != -1) // if both unspecified, leave the default
        {
            if (simulatorType == -1 && simulationType != -1) // simulator type not specified
            {
                // both qiskit aer and qcsim
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, 0, simulationType);
                AddOptimizationSimulator(simulatorHandle, 1, simulationType);
            }
            else if (simulationType == -1)
            {
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 0); // statevector
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 1); // mps
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 2); // stabilizer
            }
            else
            {
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, simulationType);
            }
        }

        char* result = SimpleExecute(simulatorHandle, circuit_json.dump().c_str(), run_config_json.dump().c_str());
        
        if (result)
        {
            JSON maestro_result = JSON::parse(result);
            FreeResult(result);

            JSON result_json = {
            {"counts", maestro_result.at("counts").get<JSON>()},
            {"time_taken", maestro_result.at("time_taken").get<JSON>()}
            };

            reverse_bitstring_keys_json(result_json);
            return result_json;
        }
        else
        {
            LOGGER_ERROR("Error executing the circuit in the Maestro simulator.");
            return {{"ERROR", "Unable to execute the circuit in the Maestro simulator."}};
        }
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Maestro simulator.\n\tTry checking the format of the circuit sent.");
        return {{"ERROR", std::string(e.what())}};
    }

    return {};
}

JSON MaestroSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Maestro dynamic simulation");
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();

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

    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();
    // is qcsim or gpu specified?
    // otherwise use qiskit aer by default
    std::string sim_name;
    
    if (qc.quantum_tasks[0].config.contains("simulator"))
        sim_name = qc.quantum_tasks[0].config.at("simulator").get<std::string>();

    int simulatorType = 0; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
    int simulationType = 0; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state
    // the p-blocks simulators use statevector only

    if (method == "automatic")
    {
        // TODO: use the estimator to pick the best method
        // need to use the given circuit(s) in quantum_tasks for that, also the number of shots and the usage of multithreading in the simulator (as opposed to using multiple simulators in different threads)!

        // for now pick up the statevector simulator
    }
    else if (method == "statevector")
    {
        simulationType = 0;
    }
    else if (method == "matrix_product_state")
    {
        // matrix_product_state_truncation_threshold
        // matrix_product_state_max_bond_dimension
        // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
        simulationType = 1;
    }
    else if (method == "stabilizer")
    {
        simulationType = 2;
    }
    else if (method == "tensor_network")
    {
        // use qcsim for this, qiskit aer is not compiled with tensor network support
        // in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
        simulationType = 3;
    }

    if (sim_name == "qcsim")
    {
        simulatorType = 1; // qcsim
    }
    else if (sim_name == "gpu" && simulationType != 2 && simulationType == 3) // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
    {
        simulatorType = 4; // gpu
    }
    else if (sim_name == "composite_qiskit")
    {
        simulatorType = 2; // p-blocks qiskit aer
        simulationType = 0; // statevector
    }
    else if (sim_name == "composite_qcsim")
    {
        simulatorType = 3; // p-blocks qcsim
        simulationType = 0; // statevector
    }

    auto start = std::chrono::high_resolution_clock::now();
#ifdef OPENMP_IN_QC
    if (size(qc.quantum_tasks) > 1) { // Quantum communications 
        #pragma omp parallel
        {
            std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> local_counter;
            
            auto simulatorHandle = CreateSimulator(simulatorType, simulationType);
            auto simulator = GetSimulator(simulatorHandle); // Not error handling

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                AllocateQubits(simulator, n_qubits);
                InitializeSimulator(simulator);
                update_meas_counter(local_counter, execute_shot_(simulator, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
                ClearSimulator(simulator);
            }

            #pragma omp critical
            for (const auto& [id, bitstrings_counter] : local_counter) {
                for (const auto& [bitstring, counts] : bitstrings_counter) {
                    meas_counter[id][bitstring] += counts;
                } 
            }
        }
    } else { // As if OPENMP_IN_QC not enabled
        auto simulatorHandle = CreateSimulator(simulatorType, simulationType);
        if (simulatorHandle == 0) {
            LOGGER_ERROR("Error creating the Maestro Simulator.");
            return {{"ERROR", "Unable to create the Maestro Simulator."}};
        }
        auto simulator = GetSimulator(simulatorHandle);

        for (std::size_t i = 0; i < shots; i++)
        {
            AllocateQubits(simulator, n_qubits); // From CUNQA: Maybe allocate after shots and restart the state in each shot for better performance?
            InitializeSimulator(simulator);
            update_meas_counter(meas_counter, execute_shot_(simulator, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
            ClearSimulator(simulator);
        } // End all shots
    }
#else
    auto simulatorHandle = CreateSimulator(simulatorType, simulationType);
    if (simulatorHandle == 0) {
        LOGGER_ERROR("Error creating the Maestro Simulator.");
        return {{"ERROR", "Unable to create the Maestro Simulator."}};
    }
    auto simulator = GetSimulator(simulatorHandle);

    for (std::size_t i = 0; i < shots; i++)
    {
        AllocateQubits(simulator, n_qubits); // From CUNQA: Maybe allocate after shots and restart the state in each shot for better performance?
        InitializeSimulator(simulator);
        update_meas_counter(meas_counter, execute_shot_(simulator, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
        ClearSimulator(simulator);
    } // End all shots
#endif
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result_json = {
        {"id_counts", meas_counter},
        {"time_taken", time_taken} };

    return result_json;
}


} // End of sim namespace
} // End of cunqa namespace
