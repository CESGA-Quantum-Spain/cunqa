#pragma once

#include <chrono>

#include "quantum_task.hpp"
#include "backends/qc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"


namespace cunqa {
namespace sim {

class MunichQCSimulator final : public SimulatorStrategy<QCBackend> {
public:
    MunichQCSimulator();
    ~MunichQCSimulator() = default;

    inline std::string get_name() const override {return "MunichQCSimulator";}
    JSON execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& circuit) override;

private:
    std::string executor_id;
    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa