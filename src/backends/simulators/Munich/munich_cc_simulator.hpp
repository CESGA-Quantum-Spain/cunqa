#pragma once

#include <chrono>

#include "quantum_task.hpp"
#include "classical_channel.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"

#include "munich_helpers.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class MunichCCSimulator final : public SimulatorStrategy<CCBackend> {
public:
    MunichCCSimulator();
    ~MunichCCSimulator() = default;

    inline std::string get_name() const override {return "MunichSimulator";}

    // TODO: The [[maybe_unused]] annotation is a temporary approach while CunqaSimulator does not take into account the backend info
    JSON execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& circuit) override;

private:
    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa