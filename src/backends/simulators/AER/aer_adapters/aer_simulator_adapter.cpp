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
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

// Free function used in both simple and distributed case
JSON usual_execution_(const SimpleBackend& backend, const QuantumTask& quantum_task)
{
    try {
        //TODO: Maybe improve them to send several circuits at once
        auto aer_quantum_task = quantum_task_to_AER(quantum_task);
        int n_clbits = quantum_task.config.at("num_clbits");
        JSON circuit_json = aer_quantum_task.circuit;

        Circuit circuit(circuit_json);
        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        JSON run_config_json(aer_quantum_task.config);
        run_config_json["seed_simulator"] = quantum_task.config.at("seed");
        Config aer_config(run_config_json);

        //LOGGER_DEBUG("circuit: {}.", backend.config.noise_model);
        Noise::NoiseModel noise_model(backend.config.noise_model);

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

JSON dynamic_execution_(const QuantumTask& quantum_task, comm::ClassicalChannel* classical_channel)
{
    LOGGER_DEBUG("Starting dynamic_execution_ on Aer.");
    // Connect to the classical communications endpoints
    if (classical_channel) {
        std::vector<std::string> connect_with = quantum_task.sending_to;
        classical_channel->connect(connect_with);
    }

    std::vector<JSON> instructions = quantum_task.circuit;
    JSON run_config = quantum_task.config;
    uint_t n_qubits = run_config.at("num_qubits").get<uint_t>();
    int shots = run_config.at("shots");
    std::string instruction_name;
    std::vector<uint_t> qubits;
    std::map<std::string, std::size_t> measurementCounter;
    JSON result;
    float time_taken;

    std::map<std::size_t, bool> classicValues; // To mimic the way Munich counts
    std::map<std::size_t, bool> classicRegister;
    std::map<std::size_t, bool> remoteClassicRegister;

    AER::AerState *state = new AER::AerState();
    state->configure("method", "statevector");
    state->configure("device", "CPU");
    state->configure("precision", "double");
    state->configure("seed_simulator", std::to_string(quantum_task.config.at("seed").get<int>()));
    
    LOGGER_DEBUG("AER variables ready.");

    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < shots; i++) {
        
        auto qubit_ids = state->allocate_qubits(n_qubits);
        state->initialize();

        for (auto& instruction : instructions) {
            instruction_name = instruction.at("name").get<std::string>();
            qubits = instruction.at("qubits").get<std::vector<uint_t>>();
            switch (constants::INSTRUCTIONS_MAP.at(instruction_name))
            {
                case constants::MEASURE:
                {
                    auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
                    std::size_t measurement = state->apply_measure(qubits);
                    classicValues[qubits[0]] = (measurement == 1);
                    if (!clreg.empty()) {
                        classicRegister[clreg[0]] = (measurement == 1);
                    }
                    break;
                }
                case constants::ID:
                    break;
                case constants::X:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcx(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcx(qubits);
                        }
                    } else {
                        state->apply_mcx(qubits);
                    }
                    break;
                }
                case constants::Y:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcy(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcy(qubits);
                        }
                    } else {
                        state->apply_mcy(qubits);
                    }
                    break;
                }
                case constants::Z:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcz(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcz(qubits);
                        }
                    } else {
                        state->apply_mcz(qubits);
                    }
                    break;
                }
                case constants::H:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_h(qubits[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_h(qubits[0]);
                        }
                    } else {
                        state->apply_h(qubits[0]);
                    }
                    break;
                }
                case constants::SX:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcsx(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcsx(qubits);
                        }
                    } else {
                        state->apply_mcsx(qubits);
                    }
                    break;
                }
                case constants::CX:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcx(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcx(qubits);
                        }
                    } else {
                        state->apply_mcx(qubits);
                    }
                    break;
                }
                case constants::CY:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcy(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcy(qubits);
                        }
                    } else {
                        state->apply_mcy(qubits);
                    }
                    break;
                }
                case constants::CZ:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcz(qubits);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcz(qubits);
                        }
                    } else {
                        state->apply_mcz(qubits);
                    }
                    break;
                }
                case constants::ECR:
                    // TODO
                    break;
                case constants::RX:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcrx(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcrx(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcrx(qubits, params[0]);
                    }
                    break;
                }
                case constants::RY:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcry(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcry(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcry(qubits, params[0]);
                    }
                    break;
                }
                case constants::RZ:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcrz(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcrz(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcrz(qubits, params[0]);
                    }
                    break;
                }
                case constants::CRX:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcrx(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcrx(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcrx(qubits, params[0]);
                    }
                    break;
                }
                case constants::CRY:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcry(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcry(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcry(qubits, params[0]);
                    }
                    break;
                }
                case constants::CRZ:
                {
                    auto params = instruction.at("params").get<std::vector<double>>();
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classicRegister[conditional_reg[0]]) {
                            state->apply_mcrz(qubits, params[0]);
                        }
                    } else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (remoteClassicRegister[conditional_reg[0]]) {
                            state->apply_mcrz(qubits, params[0]);
                        }
                    } else {
                        state->apply_mcrz(qubits, params[0]);
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
                    // //TODO: Look how Aer natively applies C_IFs operations
                    break;
                case constants::MEASURE_AND_SEND:
                {
                    auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                    uint_t measurement = state->apply_measure(qubits);
                    int measurement_as_int = static_cast<int>(measurement);
                    classical_channel->send_measure(measurement_as_int, endpoint[0]); 
                    break;
                }
                case cunqa::constants::RECV:
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
        std::string resultString(n_qubits, '0');
        // result is a map from the cbit index to the Boolean value
        for (const auto& [bitIndex, value] : classicValues) {
            resultString[n_qubits - bitIndex - 1] = value ? '1' : '0';
        }
        measurementCounter[resultString]++;

        classicRegister.clear();
        remoteClassicRegister.clear();
        state->clear();
    } // End all shots

    auto stop_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = stop_time - start_time;
    time_taken = duration.count();
    
    reverse_bitstring_keys_json(measurementCounter);
    result = {
        {"counts", measurementCounter},
        {"time_taken", time_taken}
    }; 

    return result;
}

} // End of sim namespace
} // End of cunqa namespace