#pragma once

#include <optional>
#include <string>

#include "sim/simulator.hpp"
#include "sim/run_config.hpp"
#include "sim/timing.hpp"
#include "circuit.hpp"

namespace cunqa {
namespace sim {

class NCExecutor {
public:

    NCExecutor() = default;

    void set_simulator(std::unique_ptr<Simulator> simulator)
    {
        simulator_ = std::move(simulator);
    }

    void set_timing(const std::optional<TimingModel>& timing)
    {
        if (timing)
            clock_.emplace(*timing, simulator_->num_qubits);
    }

    JSON execute(const JSON& quantum_task)
    {
        if (auto it = quantum_task.find("config"); it != quantum_task.end())
            simulator_->config = RunConfig(*it);
            
        if (simulator_->config.is_dynamic) {
            // Either create a new circuit or update the previous one
            if (auto it = quantum_task.find("instructions"); it != quantum_task.end())
                last_circuit_ = std::make_unique<DynamicCircuit>(*it);
            else if (auto it = quantum_task.find("params"); it != quantum_task.end())
                last_circuit_->update_params(it->get<std::vector<double>>());
            
            return custom_execute_();
        } else {
            // Either create a new circuit or update the previous one
            if (auto it = quantum_task.find("instructions"); it != quantum_task.end()) {
                last_circuit_ = simulator_->create_circuit(*it);
                if (clock_)
                    static_timing_ = static_timing_of_(*it);
            } else if (auto it = quantum_task.find("params"); it != quantum_task.end())
                last_circuit_->update_params(it->get<std::vector<double>>());
            
            auto result = simulator_->native_execute(*last_circuit_);
            if (clock_)
                result["simulated_time"] = static_timing_;
            return result;
        }

    }

private:
    std::unique_ptr<Simulator> simulator_;
    std::unique_ptr<Circuit> last_circuit_;
    std::optional<SimClock> clock_;
    // Without mid-circuit control flow the schedule is the same for every
    // shot, so it is computed once per circuit (parameters do not change it).
    JSON static_timing_;

    JSON custom_execute_();
    JSON static_timing_of_(const JSON& instructions);
};

} // End of sim namespace
} // End of cunqa namespace