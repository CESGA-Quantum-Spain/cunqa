#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "qulacs_computation_adapter.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QulacsSimulatorAdapter
{
public:
    QulacsSimulatorAdapter() = default;
    QulacsSimulatorAdapter(QulacsComputationAdapter& qc) : qc{qc} {}

    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr);

    QulacsComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace