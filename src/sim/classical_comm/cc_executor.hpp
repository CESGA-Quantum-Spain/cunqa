#pragma once

#include <string>
#include "simulator.hpp"
#include "classical_channel.hpp"
#include "quantum_task.hpp"

namespace cunqa {
namespace sim {

class CCExecutor {
public:
    CCExecutor(std::unique_ptr<Simulator> simulator);
    ~CCExecutor() = default;

    JSON execute(const QuantumTask& quantum_task);
private:
    std::unique_ptr<Simulator> simulator_;
    comm::ClassicalChannel classical_channel_;
};

} // End of sim namespace
} // End of cunqa namespace