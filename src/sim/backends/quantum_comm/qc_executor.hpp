#pragma once

#include <memory>
#include <string>

#include "sim/simulator.hpp"
#include "comm/classical_channel.hpp"

#include "qc_engine.hpp"

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
    std::unique_ptr<QCEngine> engine_;
};

} // End of sim namespace
} // End of cunqa namespace
