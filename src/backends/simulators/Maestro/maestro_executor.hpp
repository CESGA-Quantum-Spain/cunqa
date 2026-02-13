#pragma once

#include <string>
#include "classical_channel/classical_channel.hpp"

namespace cunqa {
namespace sim {

class MaestroExecutor {
public:
    MaestroExecutor(const std::size_t& n_qpus);
    ~MaestroExecutor() = default;

    void run();
private:
    comm::ClassicalChannel classical_channel;
    std::vector<std::string> qpu_ids;
};

} // End of sim namespace
} // End of cunqa namespace