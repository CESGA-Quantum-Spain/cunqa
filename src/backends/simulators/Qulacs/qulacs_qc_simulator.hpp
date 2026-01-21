#pragma once

#include "quantum_task.hpp"
#include "backends/qc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QulacsQCSimulator final : public SimulatorStrategy<QCBackend> {
public:
    QulacsQCSimulator();
    QulacsQCSimulator(const std::string& group_id);
    ~QulacsQCSimulator() = default;

    inline std::string get_name() const override {return "QulacsQCSimulator";}

    JSON execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& circuit) override;

private:
    void write_executor_endpoint(const std::string endpoint, const std::string& group_id = "");

    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa