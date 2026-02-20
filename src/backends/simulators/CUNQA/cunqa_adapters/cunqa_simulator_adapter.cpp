#include <string>
#include <unordered_map>
#include <stack>
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
struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    int zero_qubit = 0;
    int zero_clbit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas;
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::stack<int>> qc_meas;
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
};


std::string execute_shot_(
    Executor& executor, 
    const std::vector<cunqa::QuantumTask>& quantum_tasks, 
    cunqa::comm::ClassicalChannel* classical_channel
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
        int meas1 = executor.apply_measure({G.n_qubits - 1});
        int meas2 = executor.apply_measure({G.n_qubits - 2});
        if (meas1) executor.apply_gate("x", {G.n_qubits - 1});
        if (meas2) executor.apply_gate("x", {G.n_qubits - 1});
        executor.apply_gate("h", {G.n_qubits - 2});
        executor.apply_gate("cx", {G.n_qubits - 2, G.n_qubits - 1});
    };

    std::function<void(TaskState&, const cunqa::JSON&)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}) 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = cunqa::constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());
        auto inst_name = inst.at("name").get<std::string>();

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
            int control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
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
            int control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
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

            for (const auto& clbit: clbits)
                classical_channel->send_measure(G.creg[clbit + T.zero_clbit], qpu_id);
            break;
        }
        case cunqa::constants::RECV:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();

            for (const auto& clbit: clbits) {
                int measurement = classical_channel->recv_measure(qpu_id);
                G.creg[clbit + T.zero_clbit] = (measurement == 1);
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
            executor.apply_gate("h", {G.n_qubits - 2});
            executor.apply_gate("cx", {G.n_qubits - 2, G.n_qubits - 1});
            //----------------------------------------------------

            // CX to the entangled pair
            executor.apply_gate("cx", {qubits[0] + T.zero_qubit, G.n_qubits - 2});

            // H to the sent qubit
            executor.apply_gate("h", {qubits[0] + T.zero_qubit});

            int result = executor.apply_measure({qubits[0] + T.zero_qubit});
            int communication_result = executor.apply_measure({G.n_qubits - 2});

            G.qc_meas[T.id].push(result);
            G.qc_meas[T.id].push(communication_result);
            //Reset
            if (result) {
                executor.apply_gate("x", {qubits[0] + T.zero_qubit});
            }
            if (communication_result) {
                executor.apply_gate("x", {G.n_qubits - 2});
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
                executor.apply_gate("x", {G.n_qubits - 1});
            }
            if (meas2) {
                executor.apply_gate("z", {G.n_qubits - 1});
            }

            // Swap the value to the desired qubit
            executor.apply_gate("swap", {G.n_qubits - 1, qubits[0] + T.zero_qubit});
            //Reset
            int communcation_result = executor.apply_measure({G.n_qubits - 1});
            if (communcation_result) {
                executor.apply_gate("x", {G.n_qubits - 1});
            }
            break;
        }
        case cunqa::constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                executor.apply_gate("cx", {qubits[0] + T.zero_qubit, G.n_qubits - 2});

                int result = executor.apply_measure({G.n_qubits - 2});

                G.qc_meas[T.id].push(result);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                int meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    executor.apply_gate("z", {qubits[0] + T.zero_qubit}); 
                }

                T.cat_entangled = false;
            }
            break;
        }
        case cunqa::constants::RCONTROL:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            int meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                executor.apply_gate("x", {G.n_qubits - 1});
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            executor.apply_gate("h", {G.n_qubits - 1});

            int result = executor.apply_measure({G.n_qubits - 1});
            G.qc_meas[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked = false;
            size_t erased_elements = G.qc_meas.erase(inst.at("qpus")[0]); 
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
            if (T.finished || T.blocked)
                continue;

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

JSON CunqaSimulatorAdapter::simulate([[maybe_unused]] const Backend* backend)
{
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

JSON CunqaSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    std::map<std::string, std::size_t> meas_counter;

    auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

    int n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks)
    {
        n_qubits += quantum_task.config.at("num_qubits").get<int>();
    }
    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    Executor executor(n_qubits);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < shots; i++)
    {
        meas_counter[execute_shot_(executor, qc.quantum_tasks, classical_channel)]++;
        executor.restart_statevector();
        
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