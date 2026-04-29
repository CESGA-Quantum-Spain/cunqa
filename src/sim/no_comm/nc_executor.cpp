
#include <chrono>

#include "nc_executor.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

JSON NCExecutor::custom_execute(const QuantumTask& quantum_task)
{
    std::map<std::string, std::size_t> meas_counter;

    simulator_->config = quantum_task.config;

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < quantum_task.config.shots; i++) {
        simulator_->initialize();
        for (int pc = 0; pc < quantum_task.circuit.instructions.size(); ++pc) {
            
            std::visit([&](const auto& instr) {
                using T = std::decay_t<decltype(instr)>;

                auto type = quantum_task.circuit.instructions[pc].type;

                if constexpr (std::is_same_v<T, ClassicalIf>) {
                    // If the clbit is 0, we skip all the gates till ENDCIF arrives.
                    if (type == InstructionType::CIF && !simulator_->creg[instr.clbits[0]])
                        while (pc < quantum_task.circuit.instructions.size() && quantum_task.circuit.instructions[pc].type != InstructionType::ENDCIF)
                            ++pc;
                    // We always avoid ENDCIF cause it does not possess semantic meaning
                    if (type == InstructionType::ENDCIF)
                        return;
                } else if constexpr (std::is_same_v<T, ClassicalComm> ||std::is_same_v<T, QuantumComm>)
                    throw std::runtime_error("No communications allowed in the no communication scheme!");
                else if constexpr (std::is_same_v<T, std::monostate>)
                    throw std::runtime_error("Empty circuit received.");
                else
                    simulator_->apply_gate(type, instr);
            }, quantum_task.circuit.instructions[pc].payload);

        }

        meas_counter[simulator_->get_measures()]++;
        simulator_->clear();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    return {
        {"counts", meas_counter},
        {"time_taken", time_taken}
    };
}

} // End of sim namespace
} // End of cunqa namespace