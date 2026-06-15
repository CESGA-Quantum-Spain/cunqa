#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "cunqa_simulator_adapter.hpp"
#include "cunqa_circuit_adapter.hpp"

#include "result_cunqasim.hpp"
#include "executor.hpp"
#include "utils/types_cunqasim.hpp"

#include "utils/constants.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

struct CunqaSimulatorAdapter::State {
    Executor executor;

    State(const int& n_qubits) : executor(n_qubits) {}
};

CunqaSimulatorAdapter::CunqaSimulatorAdapter() = default;
CunqaSimulatorAdapter::~CunqaSimulatorAdapter() = default;

std::unique_ptr<Circuit> CunqaSimulatorAdapter::create_circuit(const JSON& instructions_json) const
{
    return std::make_unique<CunqaCircuit>(instructions_json);
}

void CunqaSimulatorAdapter::initialize()
{
    if (state_ == nullptr)
        state_ = std::make_unique<State>(num_qubits);
    state_->executor.restart_statevector();
}

void CunqaSimulatorAdapter::clear()
{
    state_->executor.restart_statevector();
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
        case InstructionType::X:
        case InstructionType::Y:
        case InstructionType::Z:
        case InstructionType::H:
        case InstructionType::SX:
        {
            std::string_view inst_name = instruction_type_name(type);
            state_->executor.apply_gate(std::string(inst_name), {payload.qubit});
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::RX:
        case InstructionType::RY:
        case InstructionType::RZ:
        {
            std::string_view inst_name = instruction_type_name(type);
            std::vector<double> params = {*payload.param};
            state_->executor.apply_parametric_gate(std::string(inst_name), {payload.qubit}, params);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CX:
        case InstructionType::CY:
        case InstructionType::CZ:
        case InstructionType::SWAP:
        {
            std::string_view inst_name = instruction_type_name(type);
            state_->executor.apply_gate(std::string(inst_name), {payload.qubits[0], payload.qubits[1]});
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::CRX:
        case InstructionType::CRY:
        case InstructionType::CRZ:
        {
            std::string_view inst_name = instruction_type_name(type);
            std::vector<double> params = {*payload.param};
            state_->executor.apply_parametric_gate(std::string(inst_name), {payload.qubits[0], payload.qubits[1]}, params);
            break;
        }
            
        default:
            unsupported_gate(type, payload);
    }
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
        {
            if(payload.clbit < config.num_clbits) {
                int measurement = state_->executor.apply_measure({payload.qubit});
                creg[payload.clbit] =
                        (measurement == 1);
                save_clbit[payload.clbit] = payload.save;
            } else {
                throw std::runtime_error("Cannot store measurement: classical bit "
                                            "index exceeds the available range.");
            }    
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void CunqaSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
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

JSON CunqaSimulatorAdapter::native_execute(const Circuit& circuit) {

    LOGGER_DEBUG("Cunqa native execution");
    try
    {
        auto& cunqa_adapter_circuit = dynamic_cast<const CunqaCircuit&>(circuit);
        auto n_qubits = num_qubits;
        auto shots = config.shots;

        Executor executor(n_qubits);
        QuantumCircuit cunqa_circuit = cunqa_adapter_circuit.instructions;
        for (auto instruction : cunqa_circuit)
            LOGGER_DEBUG("{}", instruction.dump(4));
            
        JSON result = executor.run(cunqa_circuit, shots);

        return result;
    } 
    catch (const std::exception &e)
    {
        LOGGER_ERROR("Error executing the circuit in the Cunqa simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return {};

}

} // End of sim namespace
} // End of cunqa namespace