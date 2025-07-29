
#include "circuit_simulator_adapter.hpp"
#include "munich_helpers.hpp"

#include <chrono>

#include "quantum_task.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

namespace cunqa
{
namespace sim
{
JSON CircuitSimulatorAdapter::simulate(const SimpleBackend& backend, comm::ClassicalChannel *classical_channel)
{
    try
    {   
        auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
        auto quantum_task = p_qca->quantum_tasks[0];

        // TODO: Change the format with the free functions
        std::string circuit = quantum_task_to_Munich(quantum_task);
        auto mqt_circuit = std::make_unique<qc::QuantumComputation>(std::move(qc::QuantumComputation::fromQASM(circuit)));

        float time_taken;
        int n_qubits = quantum_task.config.at("num_qubits");

        JSON noise_model_json = backend.config.noise_model;
        if (!noise_model_json.empty()) {
            LOGGER_DEBUG("Noise model execution");
            const ApproximationInfo approx_info{noise_model_json["step_fidelity"], noise_model_json["approx_steps"], ApproximationInfo::FidelityDriven};
            StochasticNoiseSimulator sim(std::move(mqt_circuit), approx_info, quantum_task.config["seed"], "APD", noise_model_json["noise_prob"],
                                            noise_model_json["noise_prob_t1"], noise_model_json["noise_prob_multi"]);

            auto start = std::chrono::high_resolution_clock::now();
            auto result = sim.simulate(quantum_task.config["shots"]);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                LOGGER_DEBUG("Result non empty");
                reverse_bitstring_keys_json(result);
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
                LOGGER_DEBUG("Result non empty");
                reverse_bitstring_keys_json(result);
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        }
    }
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent and/or of the noise model."}};
    }
    return {};
}

JSON CircuitSimulatorAdapter::simulate(comm::ClassicalChannel *classical_channel)
{
    // TODO: Avoid the static casting?
    auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
    std::map<std::string, std::size_t> meas_counter;

    // This is for distinguising classical and quantum communications
    // TODO: Make it more clear
    if (classical_channel && p_qca->quantum_tasks.size() == 1)
    {
        std::vector<std::string> connect_with = p_qca->quantum_tasks[0].sending_to;
        classical_channel->connect(connect_with);
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto shots = p_qca->quantum_tasks[0].config.at("shots").get<std::size_t>();
    for (std::size_t i = 0; i < shots; i++)
    {
        meas_counter[execute_shot_(classical_channel, p_qca->quantum_tasks)]++;
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

void CircuitSimulatorAdapter::apply_gate_(const JSON &instruction, std::unique_ptr<qc::StandardOperation> &&std_op, std::map<std::size_t, bool> &classic_reg, std::map<std::size_t, bool> &r_classic_reg)
{
    if (instruction.contains("conditional_reg"))
    {
        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
        if (classic_reg[conditional_reg[0]])
        {
            applyOperationToStateAdapter(std::move(std_op));
        }
    }
    else if (instruction.contains("remote_conditional_reg"))
    {
        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
        if (r_classic_reg[conditional_reg[0]])
        {
            applyOperationToStateAdapter(std::move(std_op));
        }
    }
    else
        applyOperationToStateAdapter(std::move(std_op));
}

void CircuitSimulatorAdapter::generate_entanglement_(const int &n_qubits)
{
    // Apply H to the first entanglement qubit
    auto std_op1 = std::make_unique<qc::StandardOperation>(n_qubits - 2, qc::OpType::H);
    applyOperationToStateAdapter(std::move(std_op1));

    // Apply a CX to the second one to generate an ent pair
    qc::Control control(n_qubits - 2);
    auto std_op2 = std::make_unique<qc::StandardOperation>(control, n_qubits - 1, qc::OpType::X);
    applyOperationToStateAdapter(std::move(std_op2));
}

std::string CircuitSimulatorAdapter::execute_shot_(comm::ClassicalChannel *classical_channel, const std::vector<QuantumTask> &quantum_tasks)
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

    initializeSimulationAdapter(n_qubits);

    std::vector<int> qubits;
    std::map<std::size_t, bool> classic_values;
    std::map<std::size_t, bool> classic_reg;
    std::map<std::size_t, bool> r_classic_reg;
    std::unordered_map<std::string, std::queue<int>> qc_meas;

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
            switch (constants::INSTRUCTIONS_MAP.at(instruction.at("name")))
            {
            case constants::MEASURE:
            {
                auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
                char char_measurement = measureAdapter(qubits[0] + zero_qubit[i]);
                classic_values[qubits[0] + zero_qubit[i]] = (char_measurement == '1');
                if (!clreg.empty())
                {
                    classic_reg[clreg[0]] = (char_measurement == '1');
                }
                break;
            }
            case constants::X:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::X);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::Y:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::Y);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::Z:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::Z);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::H:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::H);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::SX:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::SX);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::RX:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::RX, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::RY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::RY, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::RZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::RZ, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::SWAP:
            {
                qc::Targets targets = {static_cast<unsigned int>(n_qubits - 1), static_cast<unsigned int>(qubits[0] + zero_qubit[i])};
                auto std_op = std::make_unique<qc::StandardOperation>(targets, qc::OpType::SWAP);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CX:
            {
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::X);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CY:
            {
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::Y);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CZ:
            {
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::Z);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CRX:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::RX, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CRY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::RY, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CRZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::RZ, params);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::ECR:
            {
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::ECR);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::CECR:
            {
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op = std::make_unique<qc::StandardOperation>(control, qubits[1] + zero_qubit[i], qc::OpType::ECR);
                apply_gate_(instruction, std::move(std_op), classic_reg, r_classic_reg);
                break;
            }
            case constants::C_IF_H:
            case constants::C_IF_X:
            case constants::C_IF_Y:
            case constants::C_IF_Z:
            case constants::C_IF_ECR:
            case constants::C_IF_RX:
            case constants::C_IF_RY:
            case constants::C_IF_RZ:
                // TODO: Look how Munich natively applies C_IFs operations
                /* clreg = std::make_pair(conditional_reg[0], 1);
                auto std_op = std::make_unique<qc::StandardOperation>(qubits[1] + zero_qubit[i], qc::OpType::X);
                c_op = std::make_unique<qc::ClassicControlledOperation>(std_op, clreg);
                CCcircsim.CCapplyOperationToState(c_op); */
                break;
            case constants::MEASURE_AND_SEND:
            {
                auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                char char_measurement = measureAdapter(qubits[0] + zero_qubit[i]);
                int measurement = char_measurement - '0';
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
                generate_entanglement_(n_qubits);

                // CX to the entangled pair
                qc::Control control(qubits[0] + zero_qubit[i]);
                auto std_op1 = std::make_unique<qc::StandardOperation>(control, n_qubits - 2, qc::OpType::X);
                apply_gate_(instruction, std::move(std_op1), classic_reg, r_classic_reg);

                // H to the sent qubit
                auto std_op2 = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::H);
                apply_gate_(instruction, std::move(std_op2), classic_reg, r_classic_reg);

                int result = measureAdapter(qubits[0] + zero_qubit[i]) - '0';

                qc_meas[quantum_tasks[i].id].push(result);
                qc_meas[quantum_tasks[i].id].push(measureAdapter(n_qubits - 2) - '0');

                // We reset to 0 the qubit sent
                if (result)
                {
                    auto std_op3 = std::make_unique<qc::StandardOperation>(qubits[0] + zero_qubit[i], qc::OpType::X);
                    apply_gate_(instruction, std::move(std_op3), classic_reg, r_classic_reg);
                }

                // Desbloquear el QRECV
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
                int meas1 = qc_meas[instruction.at("qpus")[0]].front();
                qc_meas[instruction.at("qpus")[0]].pop();
                int meas2 = qc_meas[instruction.at("qpus")[0]].front();
                qc_meas[instruction.at("qpus")[0]].pop();

                // Apply, conditioned to the measurement, the X and Z gates
                if (meas1)
                {
                    auto std_op1 = std::make_unique<qc::StandardOperation>(n_qubits - 1, qc::OpType::X);
                    apply_gate_(instruction, std::move(std_op1), classic_reg, r_classic_reg);
                }
                if (meas2)
                {
                    auto std_op2 = std::make_unique<qc::StandardOperation>(n_qubits - 1, qc::OpType::Z);
                    apply_gate_(instruction, std::move(std_op2), classic_reg, r_classic_reg);
                }

                // Swap the value to the desired qubit
                qc::Targets targets = {static_cast<unsigned int>(n_qubits - 1), static_cast<unsigned int>(qubits[0] + zero_qubit[i])};
                auto std_op3 = std::make_unique<qc::StandardOperation>(targets, qc::OpType::SWAP);
                apply_gate_(instruction, std::move(std_op3), classic_reg, r_classic_reg);
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

} // End of sim namespace
} // End of cunqa namespace