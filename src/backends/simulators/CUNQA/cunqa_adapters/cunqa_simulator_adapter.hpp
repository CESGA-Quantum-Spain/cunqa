#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "cunqa_computation_adapter.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class CunqaSimulatorAdapter
{
public:
    CunqaSimulatorAdapter() = default;
    CunqaSimulatorAdapter(CunqaComputationAdapter& qc) : qc{qc} {}

    JSON simulate([[maybe_unused]] const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr);

    CunqaComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace