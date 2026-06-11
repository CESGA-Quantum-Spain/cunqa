#pragma once

#include <string>

#include "circuit.hpp"
#include "sim/simulator.hpp"
#include "sim/run_config.hpp"
#include "comm/classical_channel.hpp"

namespace cunqa {
namespace sim {

class CCExecutor {
public:
    CCExecutor();

    void set_simulator(std::unique_ptr<Simulator> simulator)
    {
        simulator_ = std::move(simulator);
    }

    JSON execute(const JSON& quantum_task);
private:
    std::unique_ptr<Simulator> simulator_;
    comm::ClassicalChannel classical_channel_;
    std::unique_ptr<Circuit> last_circuit_;
};

} // End of sim namespace
} // End of cunqa namespace