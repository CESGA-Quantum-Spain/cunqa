#pragma once

#include "quantum_task.hpp"
#include "backends/qc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

class CunqaQCSimulator final : public SimulatorStrategy<QCBackend>
{
public:
    CunqaQCSimulator();
    ~CunqaQCSimulator() = default;

    inline std::string get_name() const override {return "CunqaSimulator";}
    JSON execute(const QCBackend& backend, const QuantumTask& quantumtask) override; 

private:
    std::string executor_id;
    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa
