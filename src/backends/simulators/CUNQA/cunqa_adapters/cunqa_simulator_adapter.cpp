#include <string>
#include <unordered_map>
#include <stack>
#include <chrono>

#include "cunqa_simulator_adapter.hpp"

#include "src/result_cunqasim.hpp"
#include "src/executor.hpp"
#include "src/utils/types_cunqasim.hpp"

#include "utils/constants.hpp"
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

std::string execute_shot_(Executor& executor, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
{
    std::vector<JSON::const_iterator> its;
    std::vector<JSON::const_iterator> ends;
    std::vector<bool> finished;
    std::unordered_map<std::string, bool> blocked;
    std::vector<int> zero_qubit;
    std::vector<int> zero_clbit;
    int n_qubits = 0;
    int n_clbits = 0;

    for (auto &quantum_task : quantum_tasks)
    {
        zero_qubit.push_back(n_qubits);
        zero_clbit.push_back(n_clbits);
        its.push_back(quantum_task.circuit.begin());
        ends.push_back(quantum_task.circuit.end());
        n_qubits += quantum_task.config.at("num_qubits").get<int>();
        n_clbits += quantum_task.config.at("num_clbits").get<int>();
        blocked[quantum_task.id] = false;
        finished.push_back(false);
    }

    std::string resultString(n_clbits, '0');
    if (size(quantum_tasks) > 1)
        n_qubits += 2; 

    std::vector<int> qubits;
    std::map<std::size_t, bool> classic_values;
    std::map<std::size_t, bool> classic_reg;
    std::map<std::size_t, bool> r_classic_reg;
    std::unordered_map<std::string, std::stack<int>> qc_meas;

    bool ended = false;
    while (!ended)
    {
        ended = true;
        for (size_t i = 0; i < its.size(); ++i)
        {
            if (finished[i] || blocked[quantum_tasks[i].id])
                continue;

            auto &instruction = *its[i];
            qubits = instruction.at("qubits").get<std::vector<int>>();
            std::string instruction_name = instruction.at("name");
            switch (constants::INSTRUCTIONS_MAP.at(instruction_name))
            {
            case constants::MEASURE:
            {
                auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
                int measurement = executor.apply_measure({qubits[0] + zero_qubit[i]});
                classic_values[qubits[0] + zero_qubit[i]] = (measurement == 1);
                if (!clreg.empty()) {
                    classic_reg[clreg[0]] = (measurement == 1);
                }   
                break;
            }
            case constants::UNITARY:
            {
                // TODO: Manage 2-qubit unitaries
                auto matrix = instruction.at("params").get<Matrix>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        if (matrix.size() == 2) {
                            executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i]});
                        } else if (matrix.size() == 4) {
                            executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                        }
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        if (matrix.size() == 2) {
                            executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i]});
                        } else if (matrix.size() == 4) {
                            executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                        }
                    }
                } else {
                    if (matrix.size() == 2) {
                        executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i]});
                    } else if (matrix.size() == 4) {
                        executor.apply_unitary("unitary", matrix, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                }
                break;
            }
            case constants::ID:
            case constants::X:
            case constants::Y:
            case constants::Z:
            case constants::H:
            case constants::SX:
                {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i]});
                    }
                } else {
                    executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i]});
                }
                break;
            }
            case constants::CX:
            case constants::CY:
            case constants::CZ:
            case constants::ECR:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else {
                    executor.apply_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                }
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
                // Already managed on each individual gate
                break;
            case constants::RX:
            case constants::RY:
            case constants::RZ:
            {
                auto param =  instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i]}, param);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i]}, param);
                    }
                } else {
                    executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i]}, param);
                }
                break;
            }
            case constants::CRX:
            case constants::CRY:
            case constants::CRZ:
            {
                auto param =  instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, param);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, param);
                    }
                } else {
                    executor.apply_parametric_gate(instruction_name, {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, param);
                }
                break;
            }
            case constants::SWAP:
            {
                executor.apply_gate("swap", {qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                break;
            }
            case constants::MEASURE_AND_SEND:
            {
                auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                int measurement = executor.apply_measure({qubits[0] + zero_qubit[i]}); 
                classical_channel->send_measure(measurement, endpoint[0]); 
                break;
            }
            case constants::RECV:
            {
                auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                int measurement = classical_channel->recv_measure(endpoint[0]); 
                r_classic_reg[conditional_reg[0]] = (measurement == 1);
                break;
            }
            case constants::QSEND:
            {
                //------------- Generate Entanglement ---------------
                executor.apply_gate("h", {n_qubits - 2});
                executor.apply_gate("cx", {n_qubits - 2, n_qubits - 1});
                //----------------------------------------------------


                // CX to the entangled pair
                executor.apply_gate("cx", {qubits[0] + zero_qubit[i], n_qubits - 2});

                // H to the sent qubit
                executor.apply_gate("h", {qubits[0] + zero_qubit[i]});

                int result = executor.apply_measure({qubits[0] + zero_qubit[i]});
                int communication_result = executor.apply_measure({n_qubits - 2});

                qc_meas[quantum_tasks[i].id].push(result);
                qc_meas[quantum_tasks[i].id].push(communication_result);
                if (result) {
                    executor.apply_gate("x", {qubits[0] + zero_qubit[i]});
                }
                if (communication_result) {
                    executor.apply_gate("x", {n_qubits - 2});
                }

                // Unlock QRECV
                blocked[instruction.at("qpus")[0]] = false;
                break;
            }
            case constants::QRECV:
            {
                if (!qc_meas.contains(instruction.at("qpus")[0]))
                {
                    blocked[quantum_tasks[i].id] = true;
                    continue;
                }
 
                // Receive the measurements from the sender
                std::size_t meas1 = qc_meas[instruction.at("qpus")[0]].top();
                qc_meas[instruction.at("qpus")[0]].pop();
                std::size_t meas2 = qc_meas[instruction.at("qpus")[0]].top();
                qc_meas[instruction.at("qpus")[0]].pop();

                // Apply, conditioned to the measurement, the X and Z gates
                if (meas1)
                {
                    executor.apply_gate("x", {n_qubits - 1});
                }
                if (meas2)
                {
                    executor.apply_gate("z", {n_qubits - 1});
                }

                // Swap the value to the desired qubit
                executor.apply_gate("swap", {n_qubits - 1, qubits[0] + zero_qubit[i]});

                int communcation_result = executor.apply_measure({n_qubits - 1});
                if (communcation_result) {
                    executor.apply_gate("x", {n_qubits - 1});
                }
                break;
            }
            default:
                LOGGER_ERROR("Invalid gate name."); 
                throw std::runtime_error("Invalid gate name.");
                break;
            } // End switch 
            ++its[i];
            if (its[i] != ends[i])
                ended = false;
            else
                finished[i] = true;
        }
    }

    for (const auto &[bitIndex, value] : classic_values)
    {
        resultString[n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return resultString;
}

JSON CunqaSimulatorAdapter::simulate([[maybe_unused]] const Backend* backend)
{
    auto n_qubits = qc.quantum_tasks[0].config.at("num_qubits").get<int>();
    auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();
    Executor executor(n_qubits);
    QuantumCircuit circuit = qc.quantum_tasks[0].circuit;
    JSON result = executor.run(circuit, shots);
    reverse_bitstring_keys_json(result);

    return result;

}

JSON CunqaSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    std::map<std::string, std::size_t> meas_counter;

    // This is for distinguising classical and quantum communications
    // TODO: Make it more clear
    if (classical_channel && qc.quantum_tasks.size() == 1)
    {
        std::vector<std::string> connect_with = qc.quantum_tasks[0].sending_to;
        classical_channel->connect(connect_with);
    }

    auto shots = qc.quantum_tasks[0].config.at("shots").get<int>();

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