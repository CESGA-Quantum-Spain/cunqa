#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>
#include <random>
#include <stdexcept>

#include "quest_simulator_adapter.hpp"

#include "quest.h"

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

std::unordered_map<std::string, std::string> execute_shot_(
    quest::QuESTEnv& env,
    quest::Qureg& qubits_state,
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
                auto meas1 = applyQubitMeasurement(qubits_state, G.communication_pairs[index].q1);
                if (meas1.bitstring[0]) {
                    applyPauliX(qubits_state, G.communication_pairs[index].q1);
                } 
                auto meas2 = applyQubitMeasurement(qubits_state, G.communication_pairs[index].q0);
                if (meas2.bitstring[0]) {
                    applyPauliX(qubits_state, G.communication_pairs[index].q0);
                }
                applyHadamard(qubits_state, G.communication_pairs[index].q0);
                applyControlledPauliX(qubits_state, G.communication_pairs[index].q0, G.communication_pairs[index].q1);
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
            int measurement = applyQubitMeasurement(qubits_state, inst.qubits[0] + T.zero_qubit);
            G.creg[inst.clbits[0] + T.zero_clbit] = (measurement == 1);
            break;
        }
        case constants::RESET:
        {
            initZeroState(qubits_state);
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
        {
            applyPauliX(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::Y:
        {
            applyPauliY(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::Z:
        {
            applyPauliZ(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::H:
        {
            applyHadamard(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::S:
        {
            applyS(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::T:
        {
            applyT(qubits_state, inst.qubits[0] + T.zero_qubit);
            break;
        }
        case constants::P:
        {
            applyPhaseShift(qubits_state, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RX:
        {
            applyRotateX(qubits_state, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RY:
        {
            applyRotateY(qubits_state, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RZ:
        {
            applyRotateZ(qubits_state, inst.qubits[0] + T.zero_qubit, inst.params[0]);
            break;
        }
        case constants::RAXIS:
        {
            applyRotateAroundAxis(qubits_state, inst.qubits[0] + T.zero_qubit, inst.params[0], inst.axis[0], inst.axis[1], inst.axis[2]);
            break;
        }
        // Two qubits
        case constants::SWAP:
        {
            std::vector<int> unsigned_qubits;
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
            applySwap(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::SQRTSWAP:
        {
            std::vector<int> unsigned_qubits;
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
            applySqrtSwap(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CX:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledPauliX(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CY:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledPauliY(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CZ:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledPauliZ(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CH:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledHadamard(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CS:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledS(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CT:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledT(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back());
            break;
        }
        case constants::CP:
        {
            std::vector<int> unsigned_qubits;
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

            applyTwoQubitPhaseShift(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::CRX:
        {
            std::vector<int> unsigned_qubits;
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

            applyTwoQubitRotateX(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::CRY:
        {
            std::vector<int> unsigned_qubits;
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

            applyTwoQubitRotateY(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::CRZ:
        {
            std::vector<int> unsigned_qubits;
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

            applyTwoQubitRotateZ(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::CRAXIS:
        {
            std::vector<int> unsigned_qubits;
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

            applyControlledRotateAroundAxis(qubits_state, *unsigned_qubits.begin(), unsigned_qubits.back(), inst.params[0], inst.axis[0], inst.axis[1], inst.axis[2]);
            break;
        }
        // Several qubits
        case constants::CSWAP:
        {   
            std::vector<int> unsigned_qubits;
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
            applyControlledSwap(qubits_state, unsigned_qubits[0], unsigned_qubits[1], unsigned_qubits[2]);
            break;
        }
        case constants::CSQRTSWAP:
        {
            std::vector<int> unsigned_qubits;
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
            applyControlledSqrtSwap(qubits_state, unsigned_qubits[0], unsigned_qubits[1], unsigned_qubits[2]);
            break;
        }
        case constants::PAULISTR:
        {
            applyPauliStr(qubits_state, getPauliStr(inst.paulistr));
            break;
        }
        case constants::PAULIGADGET:
        {
            applyPauliGadget(qubits_state, getPauliStr(inst.paulistr), inst.params[0]);
            break;
        }
        case constants::NONUNITARYPAULIGADGET:
        {
            applyNonUnitaryPauliGadget(qubits_state, getPauliStr(inst.paulistr), inst.params[0]);
            break;
        }
        case constants::PHASEGADGET:
        {
            std::vector<int> unsigned_qubits;
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
            applyPhaseGadget(qubits_state, unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::CPAULISTR:
        {
            applyControlledPauliStr(qubits_state, qubits[0], getPauliStr(inst.paulistr));
            break;
        }
        case constants::CPAULIGADGET:
        {
            applyControlledPauliGadget(qubits_state, qubits[0], getPauliStr(inst.paulistr), inst.params[0]);
            break;
        }
        case constants::CPHASEGADGET:
        {
            std::vector<int> targets(qubits.begin()+1, qubits.end());
            applyControlledPhaseGadget(qubits_state, qubits[0], targets, inst.params[0]);
            break;
        }
        //Multicontrolled
        case constants::MCX:
        {   
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledPauliX(qubits_state, controls, unsigned_qubits.back());
            break;
        }
        case constants::MCY:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledPauliY(qubits_state, controls, unsigned_qubits.back());
            break;
        }
        case constants::MCZ:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledPauliZ(qubits_state, controls, unsigned_qubits.back());
            break;
        }
        case constants::MCH:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledHadamard(qubits_state, controls, unsigned_qubits.back());
            break;
        }
        case constants::MCS:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledS(qubits_state, controls, inst.qubits.back());
            break;
        }
        case constants::MCT:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledT(qubits_state, controls, inst.qubits.back());
            break;
        }
        case constants::MCP:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiQubitPhaseShift(qubits_state, unsigned_qubits, inst.params[0]);
            break;
        }
        case constants::MCRX:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledRotateX(qubits_state, controls, unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::MCRY:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledRotateY(qubits_state, controls, unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::MCRZ:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledRotateZ(qubits_state, controls, unsigned_qubits.back(), inst.params[0]);
            break;
        }
        case constants::MCRAxis:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-1);
            applyMultiControlledRotateAroundAxis(qubits_state, controls, unsigned_qubits.back(), inst.params[0], inst.axis[0], inst.axis[1], inst.axis[2]);
            break;
        }
        case constants::MCSWAP:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-2);
            applyMultiControlledSwap(qubits_state, controls, *(unsigned_qubits.end()-2), unsigned_qubits.back());
            break;
        }
        case constants::MCSqrtSWAP:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(), unsigned_qubits.end()-2);
            applyMultiControlledSqrtSwap(qubits_state, controls, *(unsigned_qubits.end()-2), unsigned_qubits.back());
            break;
        }
        case constants::MCPAULISTR:
        {
            std::vector<int> unsigned_qubits;
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
            applyMultiControlledPauliStr(qubits_state, unsigned_qubits, getPauliStr(inst.paulistr));
            break;
        }
        case constants::MCPAULIGADGET:
        {
            std::vector<int> unsigned_qubits;
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
            applyMultiControlledPauliGadget(qubits_state, unsigned_qubits, getPauliStr(inst.paulistr), inst.params[0]);
            break;
        }
        case constants::MCPHASEGADGET:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(),                        unsigned_qubits.begin() + inst.num_controls + 1);
            std::vector<int> targets(unsigned_qubits.begin() + inst.num_controls + 1, unsigned_qubits.end());
            applyMultiControlledPhaseGadget(qubits_state, controls, targets, inst.params[0]);
            break;
        }
        // --- Multi-qubit X ---
        case constants::MX:
        {
            td::vector<int> unsigned_qubits;
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
            applyMultiQubitNot(qubits_state, unsigned_qubits);
            break;
        }
        case constants::CMX:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> targets(unsigned_qubits.begin()+1, unsigned_qubits.end());
            applyControlledMultiQubitNot(qubits_state, unsigned_qubits[0], targets);
            break;
        }
        case constants::MCMX:
        {
            std::vector<int> unsigned_qubits;
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
            std::vector<int> controls(unsigned_qubits.begin(),                        unsigned_qubits.begin() + inst.num_controls + 1);
            std::vector<int> targets(unsigned_qubits.begin() + inst.num_controls + 1, unsigned_qubits.end());
            applyMultiControlledMultiQubitNot(qubits_state, controls, targets);
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
            applyControlledPauliX(qubits_state, inst.qubits[0] + T.zero_qubit, G.communication_pairs[index].q0);

            // H to the sent qubit
            applyHadamard(qubits_state, inst.qubits[0] + T.zero_qubit);

            int result1 = applyQubitMeasurement(qubits_state, inst.qubits[0] + T.zero_qubit);
            G.qc_meas_td[T.id].push(result1);
            int result2 = applyQubitMeasurement(qubits_state, G.communication_pairs[index].q0);
            G.qc_meas_td[T.id].push(result2);

            // Reset origin qubit
            if (result1) {
                applyPauliX(qubits_state, inst.qubits[0] + T.zero_qubit);
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
                applyPauliX(qubits_state, G.communication_pairs[index].q1);
            }
            if (meas2) {
                applyPauliZ(qubits_state, G.communication_pairs[index].q1);
            }

            // Swap the value to the desired qubit
            applySwap(G.communication_pairs[index].q1, inst.qubits[0] + T.zero_qubit);

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
                    applyControlledPauliX(qubits_state, inst.qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0);
                    int result = applyQubitMeasurement(qubits_state, G.communication_pairs[index].q0);

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
                    int meas = G.qc_meas_tg[inst.qpus[0]].front();
                    G.qc_meas_tg[inst.qpus[0]].pop();

                    if (meas) {
                        applyPauliZ(qubits_state, inst.qubits[0] + T.zero_qubit);
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
                    applyPauliX(qubits_state, G.communication_pairs[index].q1);
                }
            }

            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                applyHadamard(qubits_state, G.communication_pairs[index].q1);

                int result = applyQubitMeasurement(qubits_state, G.communication_pairs[index].q1);
                G.qc_meas_tg[T.id].push(result);
            }

            Ts[inst.qpus[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << inst.name << "\n";
            throw std::invalid_argument("Unknown instruction type");
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

void update_meas_counter(std::unordered_map<std::string, std::unordered_map<std::size_t, std::size_t>>& meas_counter, const std::unordered_map<std::string, long long int>& shot_outcomes)
{
    for (const auto& [circ_id, outcome] : shot_outcomes) {
        meas_counter[circ_id][outcome]++;
    }
}

std::unordered_map<std::string, std::size_t> convert_quest_result(
    const std::unordered_map<std::size_t, std::size_t>& quest_counts, 
    const int n_qubits) {
    std::unordered_map<std::string, std::size_t> counts;
    counts.reserve(quest_counts.size());
    
    std::string binary_str(n_qubits, '0');
    
    for (const auto& [num, count] : quest_counts) {
        // Convert number to binary string (in-place)
        for (int i = n_qubits - 1; i >= 0; --i) {
            binary_str[i] = ((num >> (n_qubits - 1 - i)) & 1) ? '1' : '0';
        }
        counts[binary_str] = count;
    }
    
    return counts;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON QuestSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
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
            
            int useMultithread = (num_threads > 1) ? 1 : 0;
            int useGpuAccel = (quantum_task.config.at("device")["device_name"] == "GPU") ? 1 : 0; 
            initCustomQuESTEnv(0, useGpuAccel, useMultithread); // args: (int useDistrib, int useGpuAccel, int useMultithread)
            QuESTEnv env = getQuESTEnv();
            if (quantum_task.config.contains("seed")) {
                seedQuEST(env, &seed, 1);
            }
                
            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                initZeroState(qubits_state);
                update_meas_counter(local_counter, execute_shot_(env, qubits_state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));
            }

            #pragma omp critical
            for (const auto& [id, bitstrings_counter] : local_counter) {
                for (const auto& [bitstring, counts] : bitstrings_counter) {
                    meas_counter[id][bitstring] += counts;
                } 
            }
        }
    } else { // As if OPENMP_IN_QC not enabled
        int useMultithread = (num_threads > 1) ? 1 : 0;
        int useGpuAccel = (quantum_task.config.at("device")["device_name"] == "GPU") ? 1 : 0; 
        initCustomQuESTEnv(0, useGpuAccel, useMultithread); // args: (int useDistrib, int useGpuAccel, int useMultithread)
        QuESTEnv env = getQuESTEnv();
        if (quantum_task.config.contains("seed")) {
            seedQuEST(env, &seed, 1);
        }
        for (int i = 0; i < shots; i++) {
            initZeroState(qubits_state);
            update_meas_counter(meas_counter, execute_shot_(env, qubits_state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));            
        } // End all shots
    }
#else
    int useMultithread = (num_threads > 1) ? 1 : 0;
    int useGpuAccel = (quantum_task.config.at("device")["device_name"] == "GPU") ? 1 : 0; 
    initCustomQuESTEnv(0, useGpuAccel, useMultithread); // args: (int useDistrib, int useGpuAccel, int useMultithread)
    QuESTEnv env = getQuESTEnv();
    if (quantum_task.config.contains("seed")) {
        seedQuEST(env, &seed, 1);
    }
    for (int i = 0; i < shots; i++) {
        initZeroState(qubits_state);
        update_meas_counter(meas_counter, execute_shot_(env, qubits_state, st_qtasks, classical_channel, allows_qc, n_comm_qubits));        
    } // End all shots
#endif
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    destroyQureg(qubits_state);
    finalizeQuESTEnv();

    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> results;
    for (auto& [key, counts] : meas_counter) {
        results[key] = convert_quest_result(counts, n_qubits);
    }

    JSON result_json = {
        {"id_counts", results},
        {"time_taken", time_taken}};
    return result_json;

    return JSON();
}


} // End of sim namespace
} // End of cunqa namespace