#pragma once

#include "quantum_task.hpp"
#include "backends/simple_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

class AerSimpleSimulator final : public SimulatorStrategy<SimpleBackend> {
public:
    AerSimpleSimulator(const std::string& group_id) {};
    ~AerSimpleSimulator() override;

    inline std::string get_name() const override {return "AerSimulator";} 
    JSON execute(const SimpleBackend& backend, const QuantumTask& circuit) override;
};

} // End of sim namespace
} // End of cunqa namespace