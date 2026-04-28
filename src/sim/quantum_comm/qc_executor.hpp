#pragma once

#include <string>
#include "simulator.hpp"
#include "comm/classical_channel/classical_channel.hpp"

namespace cunqa {
namespace sim {

class QCExecutor {
public:
    QCExecutor(std::unique_ptr<Simulator> simulator);
    ~QCExecutor() = default;

    JSON execute(const std::vector<QuantumTask>& quantum_tasks);
private:
    std::unique_ptr<Simulator> simulator_;
};

} // End of sim namespace
} // End of cunqa namespace