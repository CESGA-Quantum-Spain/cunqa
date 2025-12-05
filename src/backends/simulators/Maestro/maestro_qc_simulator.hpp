#pragma once

#include "quantum_task.hpp"
#include "backends/qc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class MaestroQCSimulator final : public SimulatorStrategy<QCBackend> {
public:
    MaestroQCSimulator();
    MaestroQCSimulator(const std::string& group_id);
    ~MaestroQCSimulator() = default;

    inline std::string get_name() const override {return "MaestroQCSimulator";}

    // TODO: The [[maybe_unused]] annotation is a temporary approach while CunqaSimulator does not take into account the backend info
    JSON execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& circuit) override;

private:
    void write_executor_endpoint(const std::string endpoint, const std::string& group_id = "");
    
    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa