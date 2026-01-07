#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "maestro_computation_adapter.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class MaestroSimulatorAdapter
{
public:
    MaestroSimulatorAdapter();
    MaestroSimulatorAdapter(MaestroComputationAdapter& qc);

    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr);

    MaestroComputationAdapter qc;
private:
    void* maestroInstance = nullptr;
};


} // End of sim namespace
} // End of cunqa namespace