#pragma once

#include <string>
#include "classical_channel/classical_channel.hpp"

namespace cunqa {
namespace sim {

class QsimExecutor {
public:
    QsimExecutor(const std::size_t& n_qpus);
    ~QsimExecutor() = default;

    void run();
private:
    comm::ClassicalChannel classical_channel;
    std::vector<std::string> qpu_ids;
    std::unordered_map<std::string, std::string> qpu_quantumtask_map;
};

} // End of sim namespace
} // End of cunqa namespace