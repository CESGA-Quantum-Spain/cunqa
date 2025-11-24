#pragma once

#include "quantum_task.hpp"
#include "backends/simple_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

class CunqaSimpleSimulator final : public SimulatorStrategy<SimpleBackend>
{
public:
    CunqaSimpleSimulator() = default;
    CunqaSimpleSimulator(const std::string& group_id) {};
    ~CunqaSimpleSimulator() = default;

    inline std::string get_name() const override {return "CunqaSimulator";}

    // TODO: The [[maybe_unused]] annotation is a temporary approach while CunqaSimulator does not take into account the backend info
    JSON execute([[maybe_unused]] const SimpleBackend& backend, const QuantumTask& quantum_task) override;

};

} // End namespace sim
} // End namespace cunqa
