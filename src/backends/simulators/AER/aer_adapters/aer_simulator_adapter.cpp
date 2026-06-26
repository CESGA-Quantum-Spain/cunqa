#include "aer_simulator_adapter.hpp"
#include "aer_internal_fwd.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

JSON AerSimulatorAdapter::simulate(const Backend* backend, AER::Noise::NoiseModel& noise_model)
{
    return aer_detail::simulate_standard(qc, backend, noise_model);
}

JSON AerSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    return aer_detail::simulate_dynamic(qc, classical_channel, allows_qc);
}

} // End of sim namespace
} // End of cunqa namespace