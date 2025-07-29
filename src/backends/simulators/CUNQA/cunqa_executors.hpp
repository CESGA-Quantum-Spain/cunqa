#pragma once

#include "src/result_cunqasim.hpp"
#include "src/executor.hpp"
#include "src/utils/types_cunqasim.hpp"

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "utils/json.hpp"
#include "utils/constants.hpp"
#include "cunqa_helpers.hpp"

#include "logger.hpp"

namespace cunqa {

inline JSON cunqa_execution_(const QuantumTask& quantum_task, comm::ClassicalChannel* classical_channel = nullptr)
{
    LOGGER_DEBUG("Starting cunqa_execution_.");
    // Connect to the classical communications endpoints
    if (classical_channel) {
        std::vector<std::string> connect_with = quantum_task.sending_to;
        classical_channel->connect(connect_with);
    }
    
    std::vector<JSON> instructions = quantum_task.circuit;
    JSON run_config = quantum_task.config;
    int shots = run_config.at("shots");
    std::string instruction_name;
    std::vector<int> qubits;
    JSON result;
    std::map<std::size_t, bool> classicRegister;
    std::map<std::size_t, bool> remoteClassicRegister;
    std::unordered_map<int, int> counts;
    float time_taken;

    int n_qubits = quantum_task.config.at("num_qubits");
    
    Executor executor(n_qubits);

    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < shots; i++) {
        for (auto& instruction : instructions) {
            instruction_name = instruction.at("name").get<std::string>();
            qubits = instruction.at("qubits").get<std::vector<int>>();
            switch (constants::INSTRUCTIONS_MAP.at(instruction_name))
            {
                case constants::MEASURE:
                {
                    auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
                    int measurement = executor.apply_measure(qubits);
                    if (!clreg.empty()) {
                        classicRegister[clreg[0]] = (measurement == 1);
                    }   
                    break;
                }
                case constants::UNITARY:
                {
                    auto matrix = instruction.at("params").get<Matrix>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            executor.apply_unitary("unitary", matrix, qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            executor.apply_unitary("unitary", matrix, qubits);
                        }
                    } else {
                        executor.apply_unitary("unitary", matrix, qubits);
                    }
                    break;
                }
                case constants::ID:
                case constants::X:
                case constants::Y:
                case constants::Z:
                case constants::H:
                case constants::SX:
                case constants::CX:
                case constants::CY:
                case constants::CZ:
                case constants::ECR:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            executor.apply_gate(instruction_name, qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            LOGGER_DEBUG("Entramos aquí a aplicar la puerta: {} {}", instruction.at("name").get<std::string>(), qubits[0]);
                            executor.apply_gate(instruction_name, qubits);
                        }
                    } else {
                        executor.apply_gate(instruction_name, qubits);
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
                case constants::CRX:
                case constants::CRY:
                case constants::CRZ:
                {
                    auto param =  instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            executor.apply_parametric_gate(instruction_name, qubits, param);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            executor.apply_parametric_gate(instruction_name, qubits, param);
                        }
                    } else {
                        executor.apply_parametric_gate(instruction_name, qubits, param);
                    }
                    break;
                }
                case constants::MEASURE_AND_SEND:
                {
                    auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                    int measurement = executor.apply_measure(qubits); 
                    classical_channel->send_measure(measurement, endpoint[0]); 
                    break;
                }
                case constants::RECV:
                {
                    auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    int measurement = classical_channel->recv_measure(endpoint[0]); 
                    remoteClassicRegister[conditional_reg[0]] = (measurement == 1);
                    break;
                }
                default:
                    LOGGER_ERROR("Invalid gate name."); 
                    throw std::runtime_error("Invalid gate name.");
                    break;
            }
        } // End one shot
        int position = executor.get_nonzero_position();
        if(n_qubits == 2)
            LOGGER_DEBUG("MEDIDA: {}", position);
        counts[position]++;

        classicRegister.clear();
        remoteClassicRegister.clear();
        executor.restart_statevector();
    } // End all shots

    auto stop_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = stop_time - start_time;
    time_taken = duration.count();

    result = {
        {"counts", convert_standard_results_cunqa(counts, n_qubits)},
        {"time_taken", time_taken}
    }; 

    counts.clear();

    return result;

}

} // End namespace cunqa