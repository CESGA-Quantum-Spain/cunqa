#include "aer_simple_simulator.hpp"
#include "aer_adapters/aer_computation_adapter.hpp"
#include "aer_adapters/aer_simulator_adapter.hpp"
#include "aer_adapters/aer_internal_fwd.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

JSON AerSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    AerComputationAdapter aer_ca(quantum_task);
    AerSimulatorAdapter aer_sa(aer_ca);

    if (quantum_task.is_dynamic) {
        JSON result = aer_sa.simulate();
        return {
            {"counts", result.at("id_counts").at(quantum_task.id)},
            {"time_taken", result.at("time_taken")}
        };
    } else {
        return aer_sa.simulate(&backend, *noise_model);
    }
}

AerSimpleSimulator::AerSimpleSimulator() = default;
AerSimpleSimulator::AerSimpleSimulator(const JSON& backend_json)
{
    if (backend_json.contains("noise_model")) {
        noise_model = aer_detail::make_noise_model(backend_json.at("noise_model").get<JSON>());
    } else {
         noise_model = aer_detail::make_noise_model();
    }
}

} // End namespace sim
} // End namespace cunqa