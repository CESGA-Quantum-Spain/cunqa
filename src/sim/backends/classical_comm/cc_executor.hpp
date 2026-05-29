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
    CCExecutor(std::unique_ptr<Simulator> simulator);

    inline Simulator& simulator() noexcept {
        return *simulator_;
    }

    JSON execute(const JSON& instructions, const RunConfig& run_config);
private:
    std::unique_ptr<Simulator> simulator_;
    comm::ClassicalChannel classical_channel_;
    std::unique_ptr<Circuit> last_circuit_;
};

} // End of sim namespace
} // End of cunqa namespace