#pragma once

#include <vector>

#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "aer_computation_adapter.hpp"

#include "utils/json.hpp"

namespace AER {
namespace Noise {
class NoiseModel;
}
}

namespace cunqa {
namespace sim {

class AerSimulatorAdapter
{
public:
    AerSimulatorAdapter();
    AerSimulatorAdapter(AerComputationAdapter& qc);
    ~AerSimulatorAdapter();
    
    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr, const bool allows_qc = false);

    AerComputationAdapter qc;
    std::unique_ptr<AER::Noise::NoiseModel> noise_model;
    bool is_noise_model_constructed = false;

};


} // End of sim namespace
} // End of cunqa namespace