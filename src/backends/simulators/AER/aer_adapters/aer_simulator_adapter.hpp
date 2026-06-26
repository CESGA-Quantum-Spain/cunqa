#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "aer_computation_adapter.hpp"
#include "aer_internal_fwd.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class AerSimulatorAdapter
{
public:
    AerSimulatorAdapter() = default;
    AerSimulatorAdapter(AerComputationAdapter& qc) : qc{qc} {}
    ~AerSimulatorAdapter() = default;
    
    JSON simulate(const Backend* backend, AER::Noise::NoiseModel& noise_model);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr, const bool allows_qc = false);

    AerComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace