#pragma once

#include <string>

#include "sim/simulator.hpp"
#include "quantum_task/quantum_task.hpp"

namespace cunqa {
namespace sim {

class NCExecutor {
public:
    NCExecutor(std::unique_ptr<Simulator> simulator) : 
        simulator_{std::move(simulator)}
    { }

    inline JSON native_execute(const QuantumTask& quantum_task, const JSON& noise_model)
    {
        simulator_->config = quantum_task.config;
        simulator_->native_execute(quantum_task.circuit, noise_model);
    }

    JSON custom_execute(const QuantumTask& quantum_task);
private:
    std::unique_ptr<Simulator> simulator_;
};

} // End of sim namespace
} // End of cunqa namespace