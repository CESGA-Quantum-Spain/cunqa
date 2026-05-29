
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "utils/constants.hpp"
#include "utils/helpers/reverse_bitstring.hpp"
#include "utils/helpers/json_to_qasm2.hpp"

#include "maestro_simulator_adapter.hpp"
#include "maestrolib/Interface.h"

#include "logger.hpp"


namespace cunqa {
namespace sim {

MaestroSimulatorAdapter::MaestroSimulatorAdapter() {
    maestroInstance = GetMaestroObject();
}
MaestroSimulatorAdapter::~MaestroSimulatorAdapter() = default;

void MaestroSimulatorAdapter::initialize()
{
    std::string method = config.method;
    std::string sim_name;

    if (config.maestro_simulator.has_value())
        sim_name = config.maestro_simulator.value_or("qiskit");

    // -1 for simulator type means both qiskit aer and qcsim
    // -1 for simulation type means automatic, that is... statevector + stabilizer + matrix product state
    int simulatorType = -1; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
    int simulationType = -1; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state

    // TODO: set the method into the estimator
    // also the parameters if any and so on
    if (method != "automatic")
    {
        if (method == "statevector") {
            simulationType = 0;
        } else if (method == "matrix_product_state") {
            // matrix_product_state_truncation_threshold
            // matrix_product_state_max_bond_dimension
            // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
            simulationType = 1;
        } else if (method == "stabilizer") {
            simulationType = 2;
        } else if (method == "tensor_network") {
            // use qcsim for this, qiskit aer is not compiled with tensor network support
            // in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
            simulationType = 3;
        }
    }

    if (sim_name == "qiskit" || sim_name == "aer") {
        simulatorType = 0; // qiskit aer
    } else if (sim_name == "qcsim") {
        simulatorType = 1; // qcsim
    } else if (sim_name == "gpu" && simulationType != 2 && simulationType != 3) { // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
        simulatorType = 4; // gpu
    } else if (sim_name == "composite_qiskit") {
        simulatorType = 2; // p-blocks qiskit aer
        simulationType = 0; // statevector
    } else if (sim_name == "composite_qcsim") {
        simulatorType = 3; // p-blocks qcsim
        simulationType = 0; // statevector
    }

    /* if (simulatorType != -1 || simulationType != -1) { // if both unspecified, leave the default
        if (simulatorType == -1 && simulationType != -1) { // simulator type not specified
            // both qiskit aer and qcsim
            RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, 0, simulationType);
            AddOptimizationSimulator(simulatorHandle, 1, simulationType);
        } else if (simulationType == -1) {
            RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 0); // statevector
            RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 1); // mps
            RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 2); // stabilizer
        } else {
            RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, simulationType);
        }
    } */
    auto simulatorHandle = CreateSimulator(simulatorType, simulationType);
    if (simulatorHandle == 0) {
        LOGGER_ERROR("Error creating the Maestro Simulator.");
        throw std::runtime_error("ERROR: Unable to create the Maestro Simulator.");
    }
    auto simulator = GetSimulator(simulatorHandle);
    AllocateQubits(simulator, config.num_qubits); // From CUNQA: Maybe allocate after shots and restart the state in each shot for better performance?
    InitializeSimulator(simulator);
}

void MaestroSimulatorAdapter::clear()
{
    ClearSimulator(simulator);
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::X:
            ApplyX(simulator, payload.qubit);
            break;
        case InstructionType::Y:
            ApplyY(simulator, payload.qubit);
            break;
        case InstructionType::Z:
            ApplyZ(simulator, payload.qubit);
            break;
        case InstructionType::H:
            ApplyH(simulator, payload.qubit);
            break;
        case InstructionType::S:
            ApplyS(simulator, payload.qubit);
            break;
        case InstructionType::SDG:
            ApplySDG(simulator, payload.qubit);
            break;
        case InstructionType::T:
            ApplyT(simulator, payload.qubit);
            break;
        case InstructionType::TDG:
            ApplyTDG(simulator, payload.qubit);
            break;
        case InstructionType::SX:
            ApplySX(simulator, payload.qubit);
            break;
        case InstructionType::K:
            ApplyK(simulator, payload.qubit);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::P:
        {
            ApplyP(simulator, payload.qubit, payload.param);
            break;
        }
        case InstructionType::RX:
        {
            ApplyRx(simulator, payload.qubit, payload.param);
            break;
        }
        case InstructionType::RY:
        {
            ApplyRy(simulator, payload.qubit, payload.param);
            break;
        }
        case InstructionType::RZ:
        {
            ApplyRz(simulator, payload.qubit, payload.param);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitFourParam& payload)
{
    switch (type)
    {
        case InstructionType::U:
        {
            ApplyU(simulator, payload.qubit, payload.params[0], payload.params[1], payload.params[2], payload.params[3]);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CX:
        {
            ApplyCX(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::CY:
        {
            ApplyCY(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::CZ:
        {
            ApplyCZ(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::CH:
        {
            ApplyCH(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::CSX:
        {
            ApplyCSX(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::CSXDG:
        {
            ApplyCSXDG(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }
        case InstructionType::SWAP:
        {
            ApplySwap(simulator, payload.qubits[0], payload.qubits[1]);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::CP:
        {
            ApplyCP(simulator, payload.qubits[0], payload.qubits[1], payload.param);
            break;
        }
        case InstructionType::CRX:
        {
            ApplyCRx(simulator, payload.qubits[0], payload.qubits[1], payload.param);
            break;
        }
        case InstructionType::CRY:
        {
            ApplyCRy(simulator, payload.qubits[0], payload.qubits[1], payload.param);
            break;
        }
        case InstructionType::CRZ:
        {
            ApplyCRz(simulator, payload.qubits[0], payload.qubits[1], payload.param);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitFourParam& payload)
{
    switch (type)
    {
        case InstructionType::CU:
        {
            ApplyCU(simulator, payload.qubits[0], payload.qubits[1], payload.params[0], payload.params[1], payload.params[2], payload.params[3]);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CCX:
        {
            ApplyCCX(simulator, payload.qubits[0], payload.qubits[1], payload.qubits[2]);
            break;
        }
        case InstructionType::CSWAP:
        {
            ApplyCSwap(simulator, payload.qubits[0], payload.qubits[1], payload.qubits[2]);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
        {
            const unsigned long int q[]{ payload.qubit };
            const unsigned long long int measurement = ::Measure(simulator, q, 1);

            creg[payload.clbit] = (measurement == 1);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}


void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const Reset& payload)
{
    switch (type)
    {
        case InstructionType::RESET:
        {
            std::vector<unsigned long int> uliqubits(
                payload.qubits.begin(), payload.qubits.end()
            );
		    ApplyReset(simulator, uliqubits.data(), payload.qubits.size());
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void MaestroSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
{
    switch (type)
    {
        case InstructionType::COPY:
        {
            if (payload.l_clbits.size() != payload.r_clbits.size()) {
                throw std::runtime_error(
                    "The number of copied clbits and the number of clbits "
                    "copied on does not match."
                );
            }

            for (size_t i = 0; i < payload.l_clbits.size(); ++i)
                creg[payload.l_clbits[i]] = creg[payload.r_clbits[i]];

            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

JSON MaestroSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    LOGGER_DEBUG("Maestro usual simulation");
    try { 
        JSON circuit_json = circuit.to_json();
        JSON run_config_json = config.to_json();

        auto simulatorHandle = CreateSimpleSimulator(config.num_qubits);
        if (simulatorHandle == 0) {
            LOGGER_ERROR("Error creating the Maestro SimpleSimulator.");
            return {{"ERROR", "Unable to create the Maestro SimpleSimulator."}};
        }

        std::string method = config.method;
        std::string sim_name;

        if (config.maestro_simulator.has_value())
            sim_name = config.maestro_simulator.value_or("qiskit");

        // -1 for simulator type means both qiskit aer and qcsim
        // -1 for simulation type means automatic, that is... statevector + stabilizer + matrix product state
        int simulatorType = -1; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
        int simulationType = -1; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state

        // TODO: set the method into the estimator
        // also the parameters if any and so on
        if (method != "automatic")
        {
            if (method == "statevector") {
                simulationType = 0;
            } else if (method == "matrix_product_state") {
                // matrix_product_state_truncation_threshold
                // matrix_product_state_max_bond_dimension
                // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
                simulationType = 1;
            } else if (method == "stabilizer") {
                simulationType = 2;
            } else if (method == "tensor_network") {
                // use qcsim for this, qiskit aer is not compiled with tensor network support
                // in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
                simulationType = 3;
            }
        }

        if (sim_name == "qiskit" || sim_name == "aer") {
            simulatorType = 0; // qiskit aer
        } else if (sim_name == "qcsim") {
            simulatorType = 1; // qcsim
        } else if (sim_name == "gpu" && simulationType != 2 && simulationType != 3) { // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
            simulatorType = 4; // gpu
        } else if (sim_name == "composite_qiskit") {
            simulatorType = 2; // p-blocks qiskit aer
            simulationType = 0; // statevector
        } else if (sim_name == "composite_qcsim") {
            simulatorType = 3; // p-blocks qcsim
            simulationType = 0; // statevector
        }

        if (simulatorType != -1 || simulationType != -1) { // if both unspecified, leave the default
            if (simulatorType == -1 && simulationType != -1) { // simulator type not specified
                // both qiskit aer and qcsim
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, 0, simulationType);
                AddOptimizationSimulator(simulatorHandle, 1, simulationType);
            } else if (simulationType == -1) {
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 0); // statevector
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 1); // mps
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, 2); // stabilizer
            } else {
                RemoveAllOptimizationSimulatorsAndAdd(simulatorHandle, simulatorType, simulationType);
            }
        }

        char* result = SimpleExecute(simulatorHandle, circuit_json.dump().c_str(), run_config_json.dump().c_str());
        
        if (result)
        {
            JSON maestro_result = JSON::parse(result);
            FreeResult(result);

            JSON result_json = {
            {"counts", maestro_result.at("counts").get<JSON>()},
            {"time_taken", maestro_result.at("time_taken").get<JSON>()}
            };

            reverse_bitstring_keys_json(result_json);
            return result_json;
        }
        else
        {
            LOGGER_ERROR("Error executing the circuit in the Maestro simulator.");
            return {{"ERROR", "Unable to execute the circuit in the Maestro simulator."}};
        }
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Maestro simulator.\n\tTry checking the format of the circuit sent.");
        return {{"ERROR", std::string(e.what())}};
    }

    return {};
}

} // End of sim namespace
} // End of cunqa namespace
