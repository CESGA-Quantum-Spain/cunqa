
#include <chrono>

#include "nc_executor.hpp"

#include "dynamic_circuit/dynamic_circuit.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

JSON NCExecutor::custom_execute_()
{   
    std::map<std::string, std::size_t> meas_counter;

    auto& circuit = static_cast<DynamicCircuit&>(*last_circuit_);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < simulator_->config.shots; i++) {
        simulator_->initialize();
        for (int pc = 0; pc < circuit.instructions.size(); ++pc) {
            
            std::visit([&](const auto& payload) {
                using T = std::decay_t<decltype(payload)>;

                auto type = circuit.instructions[pc].type;

                if constexpr (std::is_same_v<T, ClassicalIf>) {
                    if (type == InstructionType::CIF){

                        // Negate the clbit value if condition is 0. If there is only one clbit, result = init
                        bool init = (payload.condition) ? simulator_->creg[payload.clbits[0]] : !simulator_->creg[payload.clbits[0]];
                        // Operates on the values provided, with the specified operation. If condition is 0, the operation negates the second operand (check cif_ops)
                        bool result = std::accumulate(payload.clbits.begin() + 1, payload.clbits.end(), 
                                                      init,                       // Starting value
                                                      [&](bool acc, int clbit) { 
                                                          return cif_ops[payload.operation](acc, simulator_->creg[clbit]); 
                                                      });

                        // If condition is not met, we skip all the gates till ENDCIF arrives.
                        if (!result){
                            while (pc < circuit.instructions.size() && circuit.instructions[pc].type != InstructionType::ENDCIF)
                                ++pc;
                        } 
                    }
                    // We always avoid ENDCIF cause it does not possess semantic meaning
                    if (type == InstructionType::ENDCIF)
                        return;
                } else if constexpr (std::is_same_v<T, ClassicalComm> ||std::is_same_v<T, GenEnt>)
                    throw std::runtime_error("No communications allowed in the no communication scheme!");
                else if constexpr (std::is_same_v<T, std::monostate>)
                    throw std::runtime_error("Empty circuit received.");
                else
                    simulator_->apply_gate(type, payload);
            }, circuit.instructions[pc].payload);

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