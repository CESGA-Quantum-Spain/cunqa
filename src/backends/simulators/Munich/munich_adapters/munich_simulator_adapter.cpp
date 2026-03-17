
#include "munich_simulator_adapter.hpp"

#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <thread>
#include <functional>

#include "StochasticNoiseSimulator.hpp"

#include "quantum_task.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "logger.hpp"

using namespace qc;

namespace {

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
};


struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
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
    std::unordered_map<std::string, std::stack<int>> qc_meas_td;
    std::unordered_map<std::string, std::stack<int>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<int>, LocalIDsHash> local_cc_queue;  // To mimic classical communications when executing with quantum communications
    bool ended = false;
};

int find_idle_communication_pair(GlobalState& G)
{
    int index = 0;
    for (; index < G.communication_pairs.size(); index++) {
        if (G.communication_pairs[index].idle) {
            G.communication_pairs[index].idle = false;
            return index;
        } 
    } 

    return -1;
}

int find_my_communication_pair(const GlobalState& G, const std::string& sendr, const std::string recvr, const std::string qcomm_protocol)
{
    int index = 0;
    for (; index < G.communication_pairs.size(); index++) {
        if (G.communication_pairs[index].sendr_qpu == sendr && 
            G.communication_pairs[index].recvr_qpu == recvr &&
            G.communication_pairs[index].qcomm_protocol == qcomm_protocol) {
            return index;
        } 
    } 
}


} // End of anonymous namespace

namespace cunqa {
namespace sim {


std::string MunichSimulatorAdapter::execute_shot_(
    const std::vector<QuantumTask> &quantum_tasks, 
    comm::ClassicalChannel *classical_channel,
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

    auto generate_entanglement_ = [&]() {
        int index = find_idle_communication_pair(G);
        if (index != -1) {
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

        return index;
    };

    std::function<void(TaskState&, const JSON&, const int)> apply_next_instr = 
        [&](TaskState& T, const JSON& instruction = {}, const int comm_pair_index = -1) 
    {
        const JSON& inst = instruction.empty() ? *T.it : instruction;

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());
        
        switch (inst_type) {
        case constants::MEASURE:
        {
            char char_measurement = measureAdapter(qubits[0] + T.zero_qubit);
            auto clbits = inst.at("clbits").get<std::vector<int>>();
            G.creg[clbits[0] + T.zero_clbit] = (char_measurement == '1');
            break;
        }
        case constants::COPY:
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
            auto simple_gate = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
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
            auto params = inst.at("params").get<std::vector<double>>();
            auto simple_gate = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
            applyOperationToStateAdapter(std::move(simple_gate));
            break;
        }
        case constants::ECR:
        case constants::SWAP:
        case constants::ISWAP:
        case constants::DCX:
        {
            Targets targets = {static_cast<unsigned int>(qubits[0] + T.zero_qubit), static_cast<unsigned int>(qubits[1] + T.zero_qubit)};
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
            int ctrl = (qubits[0] == -1) ? G.communication_pairs[comm_pair_index].q1 : qubits[0] + T.zero_qubit;
            Control control(ctrl);
            auto two_gate = std::make_unique<StandardOperation>(control, qubits[1] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
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
            auto params = inst.at("params").get<std::vector<double>>();
            Targets targets = {static_cast<unsigned int>(qubits[0] + T.zero_qubit), static_cast<unsigned int>(qubits[1] + T.zero_qubit)};
            auto two_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
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
            auto params = inst.at("params").get<std::vector<double>>();
            int ctrl = (qubits[0] == -1) ? G.communication_pairs[comm_pair_index].q1 : qubits[0] + T.zero_qubit;
            Control control(ctrl);
            auto two_gate = std::make_unique<StandardOperation>(control, qubits[1] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::MCX:
        {
            for (size_t i = 0; i < qubits.size(); i++) {
                qubits[i] = (qubits[i] == -1) ? G.communication_pairs[comm_pair_index].q1 : qubits[i] + T.zero_qubit;
            }
            Controls controls(qubits.begin(), qubits.end() - 1);
            auto mc_gate = std::make_unique<StandardOperation>(controls, qubits[qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(mc_gate));
            break;
        }
        case constants::MCP:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            for (size_t i = 0; i < qubits.size(); i++) {
                qubits[i] = (qubits[i] == -1) ? G.communication_pairs[comm_pair_index].q1 : qubits[i] + T.zero_qubit;
            }
            Controls controls(qubits.begin(), qubits.end() - 1);
            auto mc_gate = std::make_unique<StandardOperation>(controls, qubits[qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
            applyOperationToStateAdapter(std::move(mc_gate));
            break;
        }
        case constants::RESET:
        {
            LOGGER_ERROR("RESET not supported because the following error raises: DD for gatereset not available!");
            //auto reset = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            //applyOperationToStateAdapter(std::move(reset));
            break;
        }
        case constants::BARRIER:
        {
            auto barrier = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(barrier));
            break;
        }
        case constants::SEND:
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
        case constants::RECV:
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
        case cunqa::constants::CIF:
        {
            const auto& clbits = inst.at("clbits").get<std::vector<int>>();
            if (G.creg[clbits.at(0) + T.zero_clbit]) {
                for(const auto& sub_inst: inst.at("instructions")) {
                    apply_next_instr(T, sub_inst, -1);
                }
            }
            break;
        }
        case constants::QSEND:
        {
            int index = generate_entanglement_();
            if (index == -1) {
                T.blocked_by_teledata = true;
                return;
            }
            T.blocked_by_teledata = false;
            G.communication_pairs[index].qcomm_protocol = "teledata";
            
            // CX to the entangled pair
            Control control(qubits[0] + T.zero_qubit);
            auto x = std::make_unique<StandardOperation>(control, G.communication_pairs[index].q0, OpType::X);
            applyOperationToStateAdapter(std::move(x));

            // H to the sent qubit
            auto h = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, OpType::H);
            applyOperationToStateAdapter(std::move(h));

            int result = measureAdapter(qubits[0] + T.zero_qubit) - '0';

            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(measureAdapter(G.communication_pairs[index].q0) - '0');

            // We reset to 0 the qubit sent and the EPR (we cannot use the reset op in DD)
            if (result)
            {
                auto reset_teleported = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, OpType::X);
                applyOperationToStateAdapter(std::move(reset_teleported));
            }

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[index].sendr_qpu = T.id;
            G.communication_pairs[index].recvr_qpu = inst.at("qpus")[0].get<std::string>();

            break;
        }
        case constants::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.at("qpus")[0])) {
                T.blocked_by_teledata = true;
                return;
            }
            if (T.blocked_by_teledata) return;

            // Receive the measurements from the sender
            int meas1 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();
            int meas2 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();

            int index = find_my_communication_pair(G, inst.at("qpus")[0], T.id, "teledata");

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
            Targets targets = {static_cast<unsigned int>(G.communication_pairs[index].q1), static_cast<unsigned int>(qubits[0] + T.zero_qubit)};
            auto swap = std::make_unique<StandardOperation>(targets, OpType::SWAP);
            applyOperationToStateAdapter(std::move(swap));

            G.communication_pairs[index].idle = true;
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                int index = generate_entanglement_();
                if (index == -1) {
                    T.blocked_by_telegate = true;
                    return;
                }
                G.communication_pairs[index].qcomm_protocol = "telegate";

                // CX to the entangled pair
                Control control(qubits[0] + T.zero_qubit);
                auto cx = std::make_unique<StandardOperation>(control, G.communication_pairs[index].q0, OpType::X);
                applyOperationToStateAdapter(std::move(cx));

                int result = measureAdapter(G.communication_pairs[index].q0) - '0';

                G.qc_meas_tg[T.id].push(result);
                T.cat_entangled = true;
                T.blocked_by_telegate = true;
                Ts[inst.at("qpus")[0]].blocked_by_telegate = false;

                // Update communication pair
                G.communication_pairs[index].sendr_qpu = T.id;
                G.communication_pairs[index].recvr_qpu = inst.at("qpus")[0].get<std::string>();
                return;
            } else {
                int meas = G.qc_meas_tg[inst.at("qpus")[0]].top();
                G.qc_meas_tg[inst.at("qpus")[0]].pop();

                if (meas) {
                    auto z = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, OpType::Z);
                    applyOperationToStateAdapter(std::move(z));
                }

                T.cat_entangled = false;

                int index = find_my_communication_pair(G, T.id, inst.at("qpus")[0], "telegate");
                G.communication_pairs[index].idle = true;
            }
            break;
        }
        case constants::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.at("qpus")[0]) || G.qc_meas_tg[inst.at("qpus")[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            int meas2 = G.qc_meas_tg[inst.at("qpus")[0]].top();
            G.qc_meas_tg[inst.at("qpus")[0]].pop();

            int index = find_my_communication_pair(G, inst.at("qpus")[0], T.id,"telegate");
            
            if (meas2) {
                auto x = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::X);
                applyOperationToStateAdapter(std::move(x));
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst, index);
            }

            auto h = std::make_unique<StandardOperation>(G.communication_pairs[index].q1, OpType::H);
            applyOperationToStateAdapter(std::move(h));

            int result = measureAdapter(G.communication_pairs[index].q1) - '0';
            G.qc_meas_tg[T.id].push(result);


            Ts[inst.at("qpus")[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!" << "\n";
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

            apply_next_instr(T, {}, -1);

            if (!(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc))
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    // result is a map from the cbit index to the Boolean value
    std::string result_bits(G.n_clbits, '0');
    for (const auto &[bitIndex, value] : G.creg)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
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
        JSON noise_model_json = backend->config.at("noise_model");
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
        LOGGER_ERROR("Error executing the circuit in the Munich simulator: {}", quantum_task.circuit.dump());
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return {}; // To avoid no-return warning
}

JSON MunichSimulatorAdapter::simulate(comm::ClassicalChannel *classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Munich dynamic simulation");
    // TODO: Avoid the static casting?
    auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
    std::map<std::string, std::size_t> meas_counter;

    auto shots = p_qca->quantum_tasks[0].config.at("shots").get<std::size_t>();

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++)
    {   
        initializeSimulationAdapter(p_qca->n_qubits);
        meas_counter[execute_shot_(p_qca->quantum_tasks, classical_channel, allows_qc, p_qca->n_comm_qubits)]++;
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