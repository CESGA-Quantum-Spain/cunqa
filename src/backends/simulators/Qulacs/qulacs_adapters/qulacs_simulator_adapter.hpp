#pragma once

#include <vector>

#include <cppsim/state.hpp>
#include <csim/type.hpp>

#include "qulacs_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
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

    UINT get_measurement(QuantumState& state, UINT target_index);

    QulacsComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace