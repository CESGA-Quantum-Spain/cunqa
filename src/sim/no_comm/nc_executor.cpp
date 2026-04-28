
#include <chrono>

#include "nc_executor.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

JSON NCExecutor::custom_execute(const QuantumTask& quantum_task) const
{
    std::map<std::string, std::size_t> meas_counter;
    //stream declaration

    simulator_->config = quantum_task.config;

    
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < quantum_task.config.shots; i++) {
        simulator_->initialize();
        for (int pc = 0; pc < quantum_task.circuit.instructions.size(); ++pc) {
            
            std::visit([&](const auto& instr) {
                using T = std::decay_t<decltype(instr)>;

                if constexpr (std::is_same_v<T, ClassicalIf>) {
                    // If the clbit is 0, we skip all the gates till ENDCIF arrives.
                    if (instr.tag == InstructionTag::CIF && !simulator_->creg[instr.clbits[0]])
                        while (pc < quantum_task.circuit.instructions.size() && instr.tag != InstructionTag::ENDCIF)
                            ++pc;
                    // We always avoid ENDCIF cause it does not possess semantic meaning
                    if (instr.tag == InstructionTag::ENDCIF)
                        return;
                } else if constexpr (std::is_same_v<T, ClassicalComm> ||std::is_same_v<T, QuantumComm>)
                    throw std::runtime_error("No communications allowed in the no communication scheme!");
                else if constexpr (std::is_same_v<T, std::monostate>)
                    throw std::runtime_error("Empty circuit received.");
                else
                    simulator_->apply_gate(instr);
            }, quantum_task.circuit.instructions[pc]);

        }

        meas_counter[simulator_->get_measures()]++;
        simulator_->clear();
    }
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