#pragma once

#include <memory>
#include "utils/json.hpp"

#include "aer_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "aer_computation_adapter.hpp"

namespace AER { namespace Noise { class NoiseModel; } }

namespace cunqa {
namespace sim {
namespace aer_detail {

struct NoiseModelDeleter {
    void operator()(AER::Noise::NoiseModel* ptr) const;
};

using NoiseModelPtr = std::unique_ptr<AER::Noise::NoiseModel, NoiseModelDeleter>;

NoiseModelPtr make_noise_model();
NoiseModelPtr make_noise_model(const JSON& j);

JSON run_circuit_with_controller(const JSON& aer_quantum_task_with_config,
                                  AER::Noise::NoiseModel& noise_model);

JSON simulate_standard(AerComputationAdapter& qc, const Backend* backend, AER::Noise::NoiseModel& noise_model);
JSON simulate_dynamic(AerComputationAdapter& qc, comm::ClassicalChannel* classical_channel, bool allows_qc);


} // namespace aer_detail
} // namespace sim
} // namespace cunqa