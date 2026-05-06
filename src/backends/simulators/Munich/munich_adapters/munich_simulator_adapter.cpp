
#include "munich_simulator_adapter.hpp"

#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <thread>
#include <functional>
#include <optional>

#include "StochasticNoiseSimulator.hpp"

#include "quantum_task.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "logger.hpp"

using namespace qc;

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
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::queue<int>> qc_meas_td;
    std::unordered_map<std::string, std::queue<int>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<int>, LocalIDsHash> local_cc_queue;  // To mimic classical communications when executing with quantum communications
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


} // End of anonymous namespace

namespace cunqa {
namespace sim {
using namespace constants;


std::unordered_map<std::string, std::string> MunichSimulatorAdapter::execute_shot_(
    std::vector<StructuredQuantumTask>& st_qtasks, 
    comm::ClassicalChannel *classical_channel,
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
                int meas1 = measureAdapter(G.communication_pairs[index].q1) - '0';
                int meas2 = measureAdapter(G.communication_pairs[index].q0) - '0';
                if (meas1) {
                    auto x_op = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::X);
                    applyOperationToStateAdapter(std::move(x_op));
                }
                if (meas2) {
                    auto x_op = std::make_unique<StandardOperation>(G.communication_pairs[index].q0, OpType::X);
                    applyOperationToStateAdapter(std::move(x_op));
                }   
                auto std_op1 = std::make_unique<StandardOperation>(G.communication_pairs[index].q0, OpType::H);
                applyOperationToStateAdapter(std::move(std_op1));
                Control control(G.communication_pairs[index].q0);
                auto std_op2 = std::make_unique<StandardOperation>(control, G.communication_pairs[index].q1, OpType::X);
                applyOperationToStateAdapter(std::move(std_op2));
            }
        }

        return indices;
    };

    std::function<void(TaskState&, const std::optional<constants::CUNQAInstruction>&, const std::vector<int>)> apply_next_instr = 
        [&](TaskState& T, const std::optional<constants::CUNQAInstruction>& instruction = std::nullopt, const std::vector<int> comm_indices = {}) 
    {
        const CUNQAInstruction inst = !instruction.has_value() ? *T.it : instruction.value();
        auto inst_type = INSTRUCTIONS_MAP.at(inst.name);
        
        switch (inst_type) {
        case constants::MEASURE:
        {
            char char_measurement = measureAdapter(inst.qubits[0] + T.zero_qubit);
            G.creg[inst.clbits[0] + T.zero_clbit] = (char_measurement == '1');

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
        case constants::X:
        case constants::Y:
        case constants::Z:
        case constants::H:
        case constants::S:
        case constants::SDG:
        case constants::SX:
        case constants::SXDG:
        case constants::T:
        case constants::TDG:
        case constants::V:
        case constants::VDG:
        {
            auto simple_gate = std::make_unique<StandardOperation>(inst.qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(simple_gate));
            break;
        }
        case constants::RX:
        case constants::RY:
        case constants::RZ:
        case constants::GLOBALP:
        case constants::P:
        case constants::U1:
        case constants::U2:
        case constants::U3:
        case constants::U:
        {
            auto simple_gate = std::make_unique<StandardOperation>(inst.qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type), inst.params);
            applyOperationToStateAdapter(std::move(simple_gate));
            break;
        }
        case constants::ECR:
        case constants::SWAP:
        case constants::ISWAP:
        case constants::DCX:
        {
            Targets targets = {static_cast<unsigned int>(inst.qubits[0] + T.zero_qubit), static_cast<unsigned int>(inst.qubits[1] + T.zero_qubit)};
            auto two_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::CX:
        case constants::CY:
        case constants::CZ:
        case constants::CH:
        case constants::CSX:
        case constants::CS:
        case constants::CSDG:
        case constants::CSWAP:
        {
            std::vector<int> tmp_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
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
            Control control(tmp_qubits[0]);
            auto two_gate = std::make_unique<StandardOperation>(control, tmp_qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::RXX:
        case constants::RYY:
        case constants::RZZ:
        case constants::RZX:
        case constants::XXMYY:
        case constants::XXPYY:
        {
            Targets targets = {static_cast<unsigned int>(inst.qubits[0] + T.zero_qubit), static_cast<unsigned int>(inst.qubits[1] + T.zero_qubit)};
            auto two_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(inst_type), inst.params);
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::CP:
        case constants::CRX:
        case constants::CRY:
        case constants::CRZ:
        case constants::CU1:
        case constants::CU2:
        case constants::CU3:
        case constants::CU:
        {
            std::vector<int> tmp_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
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
            Control control(tmp_qubits[0]);
            auto two_gate = std::make_unique<StandardOperation>(control, tmp_qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), inst.params);
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::MCX:
        {
            std::vector<int> tmp_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
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
            Controls controls(tmp_qubits.begin(), tmp_qubits.end() - 1);
            auto mc_gate = std::make_unique<StandardOperation>(controls, tmp_qubits[tmp_qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(mc_gate));
            break;
        }
        case constants::MCP:
        {
            std::vector<int> tmp_qubits(inst.qubits.size());
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
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
            Controls controls(tmp_qubits.begin(), tmp_qubits.end() - 1);
            auto mc_gate = std::make_unique<StandardOperation>(controls, tmp_qubits[tmp_qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), inst.params);
            applyOperationToStateAdapter(std::move(mc_gate));
            break;
        }
        case constants::RESET:
        {
            //LOGGER_ERROR("RESET not supported because the following error raises: DD for non-unitary operation not available!");
            NonUnitaryOperation reset(inst.qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyresetadapter(reset);
            break;
        }
        case constants::BARRIER:
        {
            auto barrier = std::make_unique<StandardOperation>(inst.qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(barrier));
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
            bool init = (static_cast<bool>(inst.condition)) ? G.creg[inst.clbits[0] + T.zero_clbit] : !G.creg[inst.clbits[0] + T.zero_clbit];
            // Operates on the values provided, with the specified operation.
            // If there is only one value, sum = G.creg[inst.clbits[0] + T.zero_clbit]
            bool result = std::accumulate(inst.clbits.begin() + 1, inst.clbits.end(), 
                           init,
                           [&](bool acc, int clbit) { 
                               return constants::cif_ops[inst.operation](acc, G.creg[clbit + T.zero_clbit]); 
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
            Control control(inst.qubits[0] + T.zero_qubit);
            auto x = std::make_unique<StandardOperation>(control, G.communication_pairs[index].q0, OpType::X);
            applyOperationToStateAdapter(std::move(x));

            // H to the sent qubit
            auto h = std::make_unique<StandardOperation>(inst.qubits[0] + T.zero_qubit, OpType::H);
            applyOperationToStateAdapter(std::move(h));

            int result = measureAdapter(inst.qubits[0] + T.zero_qubit) - '0';

            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(measureAdapter(G.communication_pairs[index].q0) - '0');

            // We reset to 0 the qubit sent and the EPR (we cannot use the reset op in DD)
            if (result)
            {
                auto reset_teleported = std::make_unique<StandardOperation>(inst.qubits[0] + T.zero_qubit, OpType::X);
                applyOperationToStateAdapter(std::move(reset_teleported));
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

            // Receive the measurements from the sender
            int meas1 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();
            int meas2 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "teledata", 1);
            int index = indices[0];

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                auto x = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::X);
                applyOperationToStateAdapter(std::move(x));
            }
            if (meas2) {
                auto z = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::Z);
                applyOperationToStateAdapter(std::move(z));
            }

            // Swap the value to the desired qubit
            Targets targets = {static_cast<unsigned int>(G.communication_pairs[index].q1), static_cast<unsigned int>(inst.qubits[0] + T.zero_qubit)};
            auto swap = std::make_unique<StandardOperation>(targets, OpType::SWAP);
            applyOperationToStateAdapter(std::move(swap));

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
                    Control control(inst.qubits[qid] + T.zero_qubit);
                    auto cx = std::make_unique<StandardOperation>(control, G.communication_pairs[index].q0, OpType::X);
                    applyOperationToStateAdapter(std::move(cx));

                    int result = measureAdapter(G.communication_pairs[index].q0) - '0';

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
                        auto z = std::make_unique<StandardOperation>(inst.qubits[i] + T.zero_qubit, OpType::Z);
                        applyOperationToStateAdapter(std::move(z));
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

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "telegate");
            
            for (auto& index : indices) {
                int meas2 = G.qc_meas_tg[inst.qpus[0]].front();
                G.qc_meas_tg[inst.qpus[0]].pop();

                if (meas2) {
                    auto x = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::X);
                    applyOperationToStateAdapter(std::move(x));
                }
            }


            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                auto h = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::H);
                applyOperationToStateAdapter(std::move(h));

                int result = measureAdapter(G.communication_pairs[index].q1) - '0';
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
        for (const auto& [bitIndex, value] : G.creg) {
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

JSON MunichSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Munich usual simulation");
    auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
    auto quantum_task = p_qca->quantum_tasks[0];

    // TODO: Change the format with the free functions
    try
    {   
        size_t n_qubits = quantum_task.config.at("num_qubits");
        size_t n_clbits = quantum_task.config.at("num_clbits");
        size_t seed = quantum_task.config.contains("seed") ? quantum_task.config.at("seed").get<size_t>() : 0;

        auto mqt_circuit = std::make_unique<QuantumComputation>(n_qubits, n_clbits, seed); 

        quantum_task_to_mqt_circuit(quantum_task.circuit, *mqt_circuit);
        
        float time_taken;

        JSON noise_model_json = {};
        if (backend->config.contains("noise_model")) {
            noise_model_json = backend->config.at("noise_model").get<JSON>();
        }
        if (!noise_model_json.empty()) {
            const ApproximationInfo approx_info{noise_model_json["step_fidelity"], noise_model_json["approx_steps"], ApproximationInfo::FidelityDriven};
            StochasticNoiseSimulator sim(std::move(mqt_circuit), approx_info, seed, "APD", noise_model_json["noise_prob"],
                                            noise_model_json["noise_prob_t1"], noise_model_json["noise_prob_multi"]);

            auto start = std::chrono::high_resolution_clock::now();
            auto result = sim.simulate(quantum_task.config["shots"]);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        } else {
            CircuitSimulator sim(std::move(mqt_circuit));

            auto start = std::chrono::high_resolution_clock::now();
            // TODO: Change this to directly call the simulate without creating a new instance?
            auto result = sim.simulate(quantum_task.config["shots"]);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        }
    }
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return {}; // To avoid no-return warning
}

JSON MunichSimulatorAdapter::simulate(comm::ClassicalChannel *classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Munich dynamic simulation");
    // TODO: Avoid the static casting?
    auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  meas_counter;

    auto shots = p_qca->quantum_tasks[0].config.at("shots").get<std::size_t>();

    std::vector<StructuredQuantumTask> st_qtasks;
    for (auto& quantum_task : p_qca->quantum_tasks) {
        st_qtasks.push_back(from_quantum_task_to_structuredqtask(quantum_task));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++) {   
        initializeSimulationAdapter(p_qca->n_qubits);
        update_meas_counter(meas_counter, execute_shot_(st_qtasks, classical_channel, allows_qc, p_qca->n_comm_qubits));
    } // End all shots

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