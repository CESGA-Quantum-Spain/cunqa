#pragma once

#include <string>
#include "sim/simulator.hpp"
#include "comm/classical_channel.hpp"
#include "quantum_task/quantum_task.hpp"

namespace cunqa {
namespace sim {

class QCExecutor {
public:
    QCExecutor(
        std::shared_ptr<Simulator> simulator, 
        const std::size_t& n_qpus
    );

    void run();
private:
    std::shared_ptr<Simulator> simulator_;
    comm::ClassicalChannel classical_channel_;
    std::vector<std::string> qpus_ids_;
    std::vector<std::vector<std::size_t>> communication_qubits_; 
    std::vector<std::size_t> zero_qubits_;
};

} // End of sim namespace
} // End of cunqa namespace