#include <string>
#include <utility>

#include "cc_executor.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "utils/helpers/environment.hpp"

#include "logger.hpp"

using namespace std::string_literals;

namespace {

using namespace cunqa;

void execute_shot_(
    sim::Simulator* simulator,
    const DynamicCircuit& circuit,
    comm::ClassicalChannel& classical_channel,
    std::optional<sim::SimClock>& clock
)
{
    for (std::size_t pc = 0; pc < circuit.instructions.size(); pc++) {
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            auto type = circuit.instructions[pc].type;

            if constexpr (std::is_same_v<T, ClassicalIf>) {
                if (type == InstructionType::CIF){
                        // Operates on the values provided, with the specified operation. 
                        //If condition is 0, the operation negates the second operand (check cif_ops)
                        bool result = std::accumulate(
                            payload.clbits.begin() + 1, payload.clbits.end(), 
                            simulator->creg[payload.clbits[0]], // Starting value
                            [&](bool acc, std::size_t clbit) { 
                                return cif_ops[payload.operation](acc, simulator->creg[clbit]); 
                            }
                        );

                        // If condition is not met, we skip all the gates till ENDCIF arrives.
                        if (result != payload.condition){
                            const auto cif_pc = pc;
                            while (pc < circuit.instructions.size() && circuit.instructions[pc].type != InstructionType::ENDCIF)
                                ++pc;
                            // The skipped gates still wait for the condition to be known.
                            if (clock) {
                                std::vector<std::size_t> qubits;
                                for (auto skipped = cif_pc + 1; skipped < pc; ++skipped) {
                                    const auto skipped_qubits = sim::instruction_qubits(circuit.instructions[skipped].payload);
                                    qubits.insert(qubits.end(), skipped_qubits.begin(), skipped_qubits.end());
                                }
                                clock->skip_conditional(payload.clbits, qubits);
                            }
                        } else if (clock)
                            clock->begin_conditional(payload.clbits);
                    }  
                // We always avoid ENDCIF cause it does not possess semantic meaning
                if (type == InstructionType::ENDCIF) {
                    if (clock)
                        clock->end_conditional();
                    return;
                }
            } else if constexpr (std::is_same_v<T, ClassicalComm>){
                if (type == InstructionType::SEND) {
                    if (payload.clbits.size() != payload.qpus.size())
                        throw std::invalid_argument("Not the same number of clbits and qpus to send!");
                    for (std::size_t i=0; i<payload.clbits.size(); i++) {
                        if (clock)
                            classical_channel.send_measure(
                                simulator->creg[payload.clbits[i]],
                                clock->classical_time({payload.clbits[i]}),
                                payload.qpus[i]
                            );
                        else
                            classical_channel.send_measure(simulator->creg[payload.clbits[i]], payload.qpus[i]);
                    }
                } else {
                    for (std::size_t i=0; i<payload.clbits.size(); i++) {
                        if (clock) {
                            auto [value, send_time] = classical_channel.recv_timed_measure(payload.qpus[i]);
                            static bool warned = false;
                            if (!send_time && !std::exchange(warned, true))
                                LOGGER_WARN("Message from {} carries no simulated time; assuming it was sent at t=0.", payload.qpus[i]);
                            simulator->creg[payload.clbits[i]] = value;
                            clock->receive_clbit(payload.clbits[i], send_time.value_or(0.0));
                        } else
                            simulator->creg[payload.clbits[i]] = classical_channel.recv_measure(payload.qpus[i]);
                    }
                }
            } else if constexpr (std::is_same_v<T, GenEnt>)
                throw std::runtime_error("No communications allowed in the no communication scheme!");
            else if constexpr (std::is_same_v<T, std::monostate>)
                throw std::runtime_error("Empty circuit received.");
            else {
                simulator->apply_gate(type, payload);
                if (clock)
                    clock->apply(type, payload);
            }
        }, circuit.instructions[pc].payload);
    }
}
} // End of anonymous namespace


namespace cunqa {
namespace sim {

CCExecutor::CCExecutor() : 
    classical_channel_{get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_TASK_PID")}
{
    classical_channel_.publish();
};

JSON CCExecutor::execute(const JSON& quantum_task)
{
    std::map<std::string, std::size_t> meas_counter;
    
    if (auto it = quantum_task.find("config"); it != quantum_task.end())
        simulator_->config = RunConfig(*it);

    // Either create a new circuit or update the previous one
    if (auto it = quantum_task.find("instructions"); it != quantum_task.end())
        last_circuit_ = std::make_unique<DynamicCircuit>(*it);
    else if (auto it = quantum_task.find("params"); it != quantum_task.end())
        last_circuit_->update_params(it->get<std::vector<double>>());

    auto& circuit = static_cast<DynamicCircuit&>(*last_circuit_);

    for(const std::string& qpu_id: simulator_->config.sending_to)
        classical_channel_.connect(qpu_id);
    
    TimingStats timing_stats;

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < simulator_->config.shots; i++) {
        simulator_->initialize();
        if (clock_)
            clock_->reset();
        execute_shot_(simulator_.get(), circuit, classical_channel_, clock_);
        if (clock_)
            timing_stats.add_shot(*clock_);
        meas_counter[simulator_->get_measures()]++;
        simulator_->clear();
    } // End all shots
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result = {
        {"counts", meas_counter},
        {"time_taken", time_taken}
    };
    if (clock_)
        result["simulated_time"] = timing_stats.to_json();
    return result;
}

} // End of sim namespace
} // End of cunqa namespace