#pragma once

#include "quantum_task.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "classical_channel/classical_channel.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

class CunqaCCSimulator final : public SimulatorStrategy<CCBackend>
{
public:
    CunqaCCSimulator();
    ~CunqaCCSimulator() = default;

    inline std::string get_name() const override {return "CunqaSimulator";}

    // TODO: The [[maybe_unused]] annotation is a temporary approach while CunqaSimulator does not take into account the backend info
    JSON execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantumtask) override; 

private:
    comm::ClassicalChannel classical_channel;
};

} // End namespace sim
} // End namespace cunqa
