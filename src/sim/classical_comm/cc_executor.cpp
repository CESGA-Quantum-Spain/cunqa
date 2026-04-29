#include <string>

#include "cc_executor.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

using namespace std::string_literals;

namespace {

using namespace cunqa;

void execute_shot_(
    sim::Simulator* simulator,
    const Circuit& circuit,
    comm::ClassicalChannel& classical_channel
)
{

    for (int pc = 0; pc < circuit.instructions.size(); pc++) {
        std::visit([&](const auto& instr) {
            using T = std::decay_t<decltype(instr)>;

            /* if constexpr (!std::is_same_v<T, std::monostate>)
                LOGGER_DEBUG("Instruccion {}: {}", std::to_string(pc), INVERTED_INSTRUCTIONS_MAP.at(instr.type));
            else
                LOGGER_DEBUG("Instruccion {}: Aquí hay monostate", std::to_string(pc)); */
            auto type = circuit.instructions[pc].type;

            if constexpr (std::is_same_v<T, ClassicalIf>) {
                // If the clbit is 0, we skip all the gates till ENDCIF arrives.
                if (type == InstructionType::CIF && !simulator->creg[instr.clbits[0]]) {
                    while (pc < circuit.instructions.size() && circuit.instructions[pc].type != InstructionType::ENDCIF)
                        ++pc;
                }
                // We always avoid ENDCIF cause it does not possess semantic meaning
                if (type == InstructionType::ENDCIF)
                    return;
            } else if constexpr (std::is_same_v<T, ClassicalComm>){
                if (type == InstructionType::SEND) {
                    for (int i=0; i<instr.clbits.size(); i++) 
                        classical_channel.send_measure(simulator->creg[instr.clbits[i]], instr.qpus[i]);
                } else {
                    for (int i=0; i<instr.clbits.size(); i++)
                        simulator->creg[instr.clbits[i]] = classical_channel.recv_measure(instr.qpus[i]);;
                }
            } else if constexpr (std::is_same_v<T, QuantumComm>)
                throw std::runtime_error("No communications allowed in the no communication scheme!");
            else if constexpr (std::is_same_v<T, std::monostate>)
                throw std::runtime_error("Empty circuit received.");
            else
                simulator->apply_gate(type, instr);
        }, circuit.instructions[pc].payload);
    }
}
} // End of anonymous namespace


namespace cunqa {
namespace sim {

CCExecutor::CCExecutor(std::unique_ptr<Simulator> simulator) : 
    simulator_{std::move(simulator)},
    classical_channel_{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel_.publish();
};

JSON CCExecutor::execute(const QuantumTask& quantum_task)
{
    std::map<std::string, std::size_t> meas_counter;
    
    for(const std::string& qpu_id: quantum_task.config.sending_to)
        classical_channel_.connect(qpu_id);
        
    simulator_->config = quantum_task.config;
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < quantum_task.config.shots; i++) {
        simulator_->initialize();
        execute_shot_(simulator_.get(), quantum_task.circuit, classical_channel_);
        LOGGER_DEBUG("Aquí llego");
        meas_counter[simulator_->get_measures()]++;
        LOGGER_DEBUG("Actualizo el mesaures");
        simulator_->clear();
    } // End all shots
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