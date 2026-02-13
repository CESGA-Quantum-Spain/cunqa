#pragma once

#include "quantum_task.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

class MaestroCCSimulator final : public SimulatorStrategy<CCBackend> {
public:
    MaestroCCSimulator();
    ~MaestroCCSimulator() = default;

    inline std::string get_name() const override {return "MaestroSimulator";}
    JSON execute(const CCBackend& backend, const QuantumTask& circuit) override;

private:
    comm::ClassicalChannel classical_channel;
};


} // End namespace sim
} // End namespace cunqa