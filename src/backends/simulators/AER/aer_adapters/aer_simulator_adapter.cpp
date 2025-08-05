
#include <unordered_map>
#include <stack>
#include <chrono>

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

std::string execute_shot_(AER::AerState* state, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
{
    std::vector<JSON::const_iterator> its;
    std::vector<JSON::const_iterator> ends;
    std::vector<bool> finished;
    std::unordered_map<std::string, bool> blocked;
    std::vector<unsigned long> zero_qubit;
    std::vector<unsigned long> zero_clbit;
    unsigned long n_qubits = 0;
    unsigned long n_clbits = 0;

    for (auto &quantum_task : quantum_tasks)
    {
        zero_qubit.push_back(n_qubits);
        zero_clbit.push_back(n_clbits);
        its.push_back(quantum_task.circuit.begin());
        ends.push_back(quantum_task.circuit.end());
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
        n_clbits += quantum_task.config.at("num_clbits").get<unsigned long>();
        blocked[quantum_task.id] = false;
        finished.push_back(false);
    }

    std::string resultString(n_clbits, '0');
    if (size(quantum_tasks) > 1)
        n_qubits += 2; 

    std::vector<unsigned long> qubits;
    std::map<std::size_t, bool> classic_values;
    std::map<std::size_t, bool> classic_reg;
    std::map<std::size_t, bool> r_classic_reg;
    std::unordered_map<std::string, std::stack<std::size_t>> qc_meas;

    bool ended = false;
    while (!ended)
    {
        ended = true;
        for (size_t i = 0; i < its.size(); ++i)
        {
            if (finished[i] || blocked[quantum_tasks[i].id])
                continue;

            auto &instruction = *its[i];
            qubits = instruction.at("qubits").get<std::vector<unsigned long>>();
            switch (constants::INSTRUCTIONS_MAP.at(instruction.at("name")))
            {
            case constants::MEASURE:
            {
                auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
                uint_t measurement = state->apply_measure({qubits[0] + zero_qubit[i]});
                classic_values[qubits[0] + zero_qubit[i]] = (measurement == 1);
                if (!clreg.empty())
                {
                    classic_reg[clreg[0]] = (measurement == 1);
                }
                break;
            }
            case constants::X:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcx({qubits[0] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcx({qubits[0] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcx({qubits[0] + zero_qubit[i]});
                }
                break;
            }
            case constants::Y:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcy({qubits[0] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcy({qubits[0] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcy({qubits[0] + zero_qubit[i]});
                }
                break;
            }
            case constants::Z:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcz({qubits[0] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcz({qubits[0] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcz({qubits[0] + zero_qubit[i]});
                }
                break;
            }
            case constants::H:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_h(qubits[0] + zero_qubit[i]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_h(qubits[0] + zero_qubit[i]);
                    }
                } else {
                    state->apply_h(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::SX:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcsx({qubits[0] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcsx({qubits[0] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcsx({qubits[0] + zero_qubit[i]});
                }
                break;
            }
            case constants::CX:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                }
                break;
            }
            case constants::CY:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcy({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcy({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcy({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                }
                break;
            }
            case constants::CZ:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
                    }
                } else {
                    state->apply_mcz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]});
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
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcrx({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcrx({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcrx({qubits[0] + zero_qubit[i]}, params[0]);
                }
                break;
            }
            case constants::RY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcry({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcry({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcry({qubits[0] + zero_qubit[i]}, params[0]);
                }
                break;
            }
            case constants::RZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcrz({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcrz({qubits[0] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcrz({qubits[0] + zero_qubit[i]}, params[0]);
                }
                break;
            }
            case constants::CRX:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcrx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcrx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcrx({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                }
                break;
            }
            case constants::CRY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcry({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcry({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcry({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                }
                break;
            }
            case constants::CRZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        state->apply_mcrz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        state->apply_mcrz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
                    }
                } else {
                    state->apply_mcrz({qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]}, params[0]);
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
                uint_t measurement = state->apply_measure({qubits[0] + zero_qubit[i]});
                int measurement_as_int = static_cast<int>(measurement);
                classical_channel->send_measure(measurement_as_int, endpoint[0]); 
                break;
            }
            case cunqa::constants::RECV:
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
                state->apply_h(n_qubits - 2);
                state->apply_mcx({n_qubits - 2, n_qubits - 1});
                //----------------------------------------------------


                // CX to the entangled pair
                state->apply_mcx({qubits[0] + zero_qubit[i], n_qubits - 2});

                // H to the sent qubit
                state->apply_h(qubits[0] + zero_qubit[i]);

                uint_t result = state->apply_measure({qubits[0] + zero_qubit[i]});

                qc_meas[quantum_tasks[i].id].push(result);
                qc_meas[quantum_tasks[i].id].push(state->apply_measure({n_qubits - 2}));
                state->apply_reset({n_qubits - 2, qubits[0] + zero_qubit[i]});

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
                    state->apply_mcx({n_qubits - 1});
                }
                if (meas2)
                {
                    state->apply_mcz({n_qubits - 1});
                }

                // Swap the value to the desired qubit
                state->apply_mcswap({n_qubits - 1, qubits[0] + zero_qubit[i]});
                state->apply_reset({n_qubits - 1});
                break;
            }
            default:
                std::cerr << "Instruction not suported!" << "\n";
            } // End switch  
            
            ++its[i];
            if (its[i] != ends[i])
                ended = false;
            else
                finished[i] = true;
        }

    } // End one shot

    // result is a map from the cbit index to the Boolean value
    for (const auto &[bitIndex, value] : classic_values)
    {
        resultString[n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return resultString;

}

JSON AerSimulatorAdapter::simulate(const Backend* backend)
{
    try {
        auto quantum_task = qc.quantum_tasks[0];

        auto aer_quantum_task = quantum_task_to_AER(quantum_task);
        int n_clbits = quantum_task.config.at("num_clbits");
        JSON circuit_json = aer_quantum_task.circuit;

        Circuit circuit(circuit_json);
        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        JSON run_config_json(aer_quantum_task.config);
        run_config_json["seed_simulator"] = quantum_task.config.at("seed");
        Config aer_config(run_config_json);

        Noise::NoiseModel noise_model(backend->config.at("noise_model"));

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


JSON AerSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    std::map<std::string, std::size_t> meas_counter;

    // This is for distinguising classical and quantum communications
    // TODO: Make it more clear
    if (classical_channel && qc.quantum_tasks.size() == 1)
    {
        std::vector<std::string> connect_with = qc.quantum_tasks[0].sending_to;
        classical_channel->connect(connect_with);
    }

    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

    AER::AerState* state = new AER::AerState();
    state->configure("method", method);
    state->configure("device", "CPU");
    state->configure("precision", "double");
    state->configure("seed_simulator", std::to_string(qc.quantum_tasks[0].config.at("seed").get<int>()));


    unsigned long n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks)
    {
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
    }
    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    reg_t qubit_ids;
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++)
    {
        qubit_ids = state->allocate_qubits(n_qubits);
        state->initialize();
        meas_counter[execute_shot_(state, qc.quantum_tasks, classical_channel)]++;
        state->clear();
    } // End all shots

    delete state;

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
