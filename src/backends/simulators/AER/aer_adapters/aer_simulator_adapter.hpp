#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "aer_computation_adapter.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class AerSimulatorAdapter
{
public:
    AerSimulatorAdapter() = default;
    AerSimulatorAdapter(AerComputationAdapter& qc) : qc{qc} {}

    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr);

    AerComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace