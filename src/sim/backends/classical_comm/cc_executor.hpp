#pragma once

#include <string>
#include "sim/simulator.hpp"
#include "quantum_task/quantum_task.hpp"
#include "comm/classical_channel.hpp"

namespace cunqa {
namespace sim {

class CCExecutor {
public:
    CCExecutor(std::unique_ptr<Simulator> simulator);

    JSON execute(const QuantumTask& quantum_task);
private:
    std::unique_ptr<Simulator> simulator_;
    comm::ClassicalChannel classical_channel_;
};

} // End of sim namespace
} // End of cunqa namespace