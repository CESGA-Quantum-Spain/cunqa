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
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

namespace {
struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    int zero_qubit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas;
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg, rcreg;
    std::map<std::size_t, bool> cvalues;
    std::unordered_map<std::string, std::stack<int>> qc_meas;
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
};
}

namespace cunqa {
namespace sim {

std::string execute_shot_(Executor& executor, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (auto &quantum_task : quantum_tasks)
    {
        TaskState T;
        T.id = quantum_task.id;
        T.zero_qubit = G.n_qubits;
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
        //Reset
        int meas1 = executor.apply_measure({G.n_qubits - 1});
        int meas2 = executor.apply_measure({G.n_qubits - 2});
        if (meas1) {
            executor.apply_gate("x", {G.n_qubits - 1});
        }
        if (meas2) {
            executor.apply_gate("x", {G.n_qubits - 1});
        }
        // Apply H to the first entanglement qubit
        executor.apply_gate("h", {G.n_qubits - 2});

        // Apply a CX to the second one to generate an ent pair
        executor.apply_gate("cx", {G.n_qubits - 2, G.n_qubits - 1});
    };

    std::function<void(TaskState&, const JSON&)> apply_next_instr = [&](TaskState& T, const JSON& instruction = {}) {

        // This is added to be able to add instructions outside the main loop
        const JSON& inst = instruction.empty() ? *T.it : instruction;

        if (inst.contains("conditional_reg")) {
            auto v = inst.at("conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.creg[v[0]]) return;
        } else if (inst.contains("remote_conditional_reg") && inst.at("name") != "recv") { // TODO: Cambiar el nombre para el recv y para el resto
            auto v = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.rcreg[v[0]]) return;
        }

        std::vector<int> qubits = inst.at("qubits").get<std::vector<int>>();
        std::string inst_name = inst.at("name").get<std::string>();
        auto inst_type = constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case constants::MEASURE:
        {
            auto clreg = inst.at("clreg").get<std::vector<std::uint64_t>>();
            int measurement = executor.apply_measure({qubits[0] + T.zero_qubit});
            G.cvalues[qubits[0] + T.zero_qubit] = (measurement == 1);
            if (!clreg.empty())
            {
                G.creg[clreg[0]] = (measurement == 1);
            }
            break;
        }
        case constants::ID:
        case constants::X:
        case constants::Y:
        case constants::Z:
        case constants::H:
        case constants::SX:
            executor.apply_gate(inst_name, {qubits[0] + T.zero_qubit});
            break;
        case constants::CX:
        case constants::CY:
        case constants::CZ:
        {
            int control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            executor.apply_gate(inst_name, {control, qubits[1] + T.zero_qubit});
            break;
        }
        case constants::ECR:
            // TODO
            break;
        case constants::RX:
        case constants::RY:
        case constants::RZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            executor.apply_parametric_gate(inst_name, {qubits[0] + T.zero_qubit}, params);
            break;
        }
        case constants::CRX:
        case constants::CRY:
        case constants::CRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            int control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            executor.apply_parametric_gate(inst_name, {control, qubits[1] + T.zero_qubit}, params);
            break;
        }
        case constants::C_IF_H:
        case constants::C_IF_X:
        case constants::C_IF_Y:
        case constants::C_IF_Z:
        case constants::C_IF_CX:
        case constants::C_IF_CY:
        case constants::C_IF_CZ:
        case constants::C_IF_ECR:
        case constants::C_IF_RX:
        case constants::C_IF_RY:
        case constants::C_IF_RZ:
            // Already managed 
            break;
        case constants::SWAP:
        {
            executor.apply_gate(inst_name, {qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit});
            break;
        }
        case constants::MEASURE_AND_SEND:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            int measurement = executor.apply_measure({qubits[0] + T.zero_qubit});
            int measurement_as_int = static_cast<int>(measurement);
            classical_channel->send_measure(measurement_as_int, endpoint[0]); 
            break;
        }
        case cunqa::constants::RECV:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            auto conditional_reg = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            int measurement = classical_channel->recv_measure(endpoint[0]);
            G.rcreg[conditional_reg[0]] = (measurement == 1);
            break;
        }
        case constants::QSEND:
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
        case constants::QRECV:
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
        case constants::EXPOSE:
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
        case constants::RCONTROL:
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
    for (const auto &[bitIndex, value] : G.cvalues)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
}

JSON CunqaSimulatorAdapter::simulate([[maybe_unused]] const Backend* backend)
{
    auto n_qubits = qc.quantum_tasks[0].config.at("num_qubits").get<int>();
    auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();
    Executor executor(n_qubits);
    QuantumCircuit circuit = qc.quantum_tasks[0].circuit;
    JSON result = executor.run(circuit, shots);

    return result;

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

    reverse_bitstring_keys_json(meas_counter);
    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}


} // End of sim namespace
} // End of cunqa namespace