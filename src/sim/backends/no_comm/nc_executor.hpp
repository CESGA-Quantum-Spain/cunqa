#pragma once

#include <string>

#include "sim/simulator.hpp"
#include "sim/run_config.hpp"
#include "circuit.hpp"

namespace cunqa {
namespace sim {

class NCExecutor {
public:
    NCExecutor(std::unique_ptr<Simulator> simulator) : 
        simulator_{std::move(simulator)}
    { }

    inline Simulator& simulator() noexcept {
        return *simulator_;
    }

    inline JSON native_execute(const JSON& quantum_task, const JSON& noise_model)
    {
        if (auto it = quantum_task.find("config"); it != quantum_task.end())
            simulator_->config = RunConfig(*it);

        // Either create a new circuit or update the previous one
        if (auto it = quantum_task.find("instructions"); it != quantum_task.end())
            last_circuit_ = simulator_->create_circuit(*it);
        else if (auto it = quantum_task.find("params"); it != quantum_task.end())
            last_circuit_->update_params(it->get<std::vector<double>>());
        
        return simulator_->native_execute(*last_circuit_, noise_model);
    }

    JSON custom_execute(const JSON& quantum_task);
private:
    std::unique_ptr<Simulator> simulator_;
    std::unique_ptr<Circuit> last_circuit_;
};

} // End of sim namespace
} // End of cunqa namespace