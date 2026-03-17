#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "cunqa_simulator_adapter.hpp"

#include "result_cunqasim.hpp"
#include "executor.hpp"
#include "utils/types_cunqasim.hpp"

#include "utils/constants.hpp"

#include "logger.hpp"

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
    std::stack<int> telep_meas;
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::stack<int>> qc_meas_td;
    std::unordered_map<std::string, std::stack<int>> qc_meas_tg;
    std::unordered_map<std::string, CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<int>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
};


std::string find_idle_communication_pair(GlobalState& G)
{
    for (auto& [key, comm_pair] : G.communication_pairs) {
        if (comm_pair.idle) {
            comm_pair.idle = false;
            return key;
        } 
    }
    return "NOIDLEPAIRS";
}

std::string find_my_communication_pair(const GlobalState& G, const std::string& sendr, const std::string recvr, const std::string qcomm_protocol)
{
    for (auto& [key, comm_pair] : G.communication_pairs) {
        if (comm_pair.sendr_qpu == sendr && comm_pair.recvr_qpu == recvr && comm_pair.qcomm_protocol == qcomm_protocol) return key;
    }
}


std::string execute_shot_(
    Executor& executor, 
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
    if (n_comm_qubits) {
        G.n_qubits += n_comm_qubits;
        for (int i = 0; i < n_comm_qubits; i+=2) {
            CommunicationQubitsPair cqp = {
                .q0 = G.n_qubits - n_comm_qubits + i,
                .q1 = G.n_qubits - n_comm_qubits + i + 1
            };
            G.communication_pairs[std::to_string(i)] = cqp;
        }
    }
    
    auto generate_entanglement_ = [&]() {
        std::string key = find_idle_communication_pair(G);
        if (key != "NOIDLEPAIRS") {
            int meas1 = executor.apply_measure({G.n_qubits - 1});
            if (meas1) {
                executor.apply_gate("x", {G.n_qubits - 1});
            } 
            int meas2 = executor.apply_measure({G.n_qubits - 2});
            if (meas2) {
                executor.apply_gate("x", {G.n_qubits - 2});
            }
            executor.apply_gate("h", {G.n_qubits - 2});
            executor.apply_gate("cx", {G.n_qubits - 2, G.n_qubits - 1});
        }

        return key;
    };

    std::function<void(TaskState&, const cunqa::JSON&, const std::string&)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}, const std::string comm_pair_key = "") 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = cunqa::constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());
        std::string inst_name = inst.at("name").get<std::string>();

        switch (inst_type)
        {
        case cunqa::constants::MEASURE:
        {
            int measurement = executor.apply_measure({qubits[0] + T.zero_qubit});
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
        case cunqa::constants::X:
        case cunqa::constants::Y:
        case cunqa::constants::Z:
        case cunqa::constants::H:
        case cunqa::constants::SX:
            executor.apply_gate(inst_name, {qubits[0] + T.zero_qubit});
            break;
        case cunqa::constants::CX:
        case cunqa::constants::CY:
        case cunqa::constants::CZ:
        {
            int control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            executor.apply_gate(inst_name, {control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::ECR:
            // TODO
            break;
        case cunqa::constants::RX:
        case cunqa::constants::RY:
        case cunqa::constants::RZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            executor.apply_parametric_gate(inst_name, {qubits[0] + T.zero_qubit}, params);
            break;
        }
        case cunqa::constants::CRX:
        case cunqa::constants::CRY:
        case cunqa::constants::CRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            int control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            executor.apply_parametric_gate(inst_name, {control, qubits[1] + T.zero_qubit}, params);
            break;
        }
        case cunqa::constants::SWAP:
        {
            executor.apply_gate(inst_name, {qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit});
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
                    apply_next_instr(T, sub_inst, "");
                }
            }
            break;
        }
        case cunqa::constants::QSEND:
        {
            std::string key = generate_entanglement_();
            if (key == "NOIDLEPAIRS") {
                T.blocked_by_teledata = true;
                return;
            }
            T.blocked_by_teledata = false;
            G.communication_pairs[key].qcomm_protocol = "teledata";

            // CX to the entangled pair
            executor.apply_gate("cx", {qubits[0] + T.zero_qubit, G.communication_pairs[key].q0});

            // H to the sent qubit
            executor.apply_gate("h", {qubits[0] + T.zero_qubit});

            int result = executor.apply_measure({qubits[0] + T.zero_qubit});

            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(executor.apply_measure({G.communication_pairs[key].q0}));

            if (result) {
                executor.apply_gate("x", {qubits[0] + T.zero_qubit});
            }

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[key].sendr_qpu = T.id;
            G.communication_pairs[key].recvr_qpu = inst.at("qpus")[0].get<std::string>();

            break;
        }
        case cunqa::constants::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.at("qpus")[0])) {
                T.blocked_by_teledata = true;
                return;
            }
            if (T.blocked_by_teledata) return;

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();
            std::size_t meas2 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();

            std::string key = find_my_communication_pair(G, inst.at("qpus")[0], T.id, "teledata");

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                executor.apply_gate("x", {G.communication_pairs[key].q1});
            }
            if (meas2) {
                executor.apply_gate("z", {G.communication_pairs[key].q1});
            }

            // Swap the value to the desired qubit
            executor.apply_gate("swap", {G.communication_pairs[key].q1, qubits[0] + T.zero_qubit});

            G.communication_pairs[key].idle = true;
            break;
        }
        case cunqa::constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                std::string key = generate_entanglement_();
                if (key == "NOIDLEPAIRS") {
                    T.blocked_by_telegate = true;
                    return;
                }
                G.communication_pairs[key].qcomm_protocol = "telegate";

                // CX to the entangled pair
                executor.apply_gate("cx", {qubits[0] + T.zero_qubit, G.communication_pairs[key].q0});

                int result = executor.apply_measure({G.communication_pairs[key].q0});

                G.qc_meas_tg[T.id].push(result);
                T.cat_entangled = true;
                T.blocked_by_telegate = true;
                Ts[inst.at("qpus")[0]].blocked_by_telegate = false;

                // Update communication pair
                G.communication_pairs[key].sendr_qpu = T.id;
                G.communication_pairs[key].recvr_qpu = inst.at("qpus")[0].get<std::string>();
                return;
            } else {
                int meas = G.qc_meas_tg[inst.at("qpus")[0]].top();
                G.qc_meas_tg[inst.at("qpus")[0]].pop();

                if (meas) {
                    executor.apply_gate("z", {qubits[0] + T.zero_qubit}); 
                }

                T.cat_entangled = false;

                std::string key = find_my_communication_pair(G, T.id, inst.at("qpus")[0], "telegate");
                G.communication_pairs[key].idle = true;
            }
            break;
        }
        case cunqa::constants::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.at("qpus")[0]) || G.qc_meas_tg[inst.at("qpus")[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            int meas2 = G.qc_meas_tg[inst.at("qpus")[0]].top();
            G.qc_meas_tg[inst.at("qpus")[0]].pop();

            std::string key = find_my_communication_pair(G, inst.at("qpus")[0], T.id, "telegate");

            if (meas2) {
                executor.apply_gate("x", {G.communication_pairs[key].q1});
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst, key);
            }

            executor.apply_gate("h", {G.communication_pairs[key].q1});

            int result = executor.apply_measure({G.communication_pairs[key].q1});
            G.qc_meas_tg[T.id].push(result);

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

            apply_next_instr(T, {}, "");

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

JSON CunqaSimulatorAdapter::simulate([[maybe_unused]] const Backend* backend)
{
    LOGGER_DEBUG("Cunqa usual simulation");
    try
    { 
        auto n_qubits = qc.quantum_tasks[0].config.at("num_qubits").get<int>();
        auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();
        Executor executor(n_qubits);
        QuantumCircuit circuit = qc.quantum_tasks[0].circuit;
        JSON result = executor.run(circuit, shots);

        return result;
    } 
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Cunqa simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return {};

}

JSON CunqaSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Cunqa dynamic simulation");
    std::map<std::string, std::size_t> meas_counter;

    auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

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
            
            Executor executor(n_qubits);

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                local_counter[execute_shot_(executor, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
                executor.restart_statevector();
            }

            #pragma omp critical
            for (auto& [key, val] : local_counter)
                meas_counter[key] += val;
        }
    } else { // As if OPENMP_IN_QC not enabled
        Executor executor(n_qubits);
        for (int i = 0; i < shots; i++)
        {
            meas_counter[execute_shot_(executor, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
            executor.restart_statevector();
            
        } // End all shots
    }
#else
    Executor executor(n_qubits);
    for (int i = 0; i < shots; i++)
    {
        meas_counter[execute_shot_(executor, qc.quantum_tasks, classical_channel, allows_qc,n_comm_qubits)]++;
        executor.restart_statevector();
        
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