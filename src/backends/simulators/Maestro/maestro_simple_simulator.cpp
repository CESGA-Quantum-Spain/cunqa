#include "maestro_simple_simulator.hpp"
#include "maestro_adapters/maestro_computation_adapter.hpp"
#include "maestro_adapters/maestro_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

// Simple MaestroSimulator
MaestroSimpleSimulator::~MaestroSimpleSimulator() = default;

JSON MaestroSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task)
{
    MaestroComputationAdapter maestro_ca(quantum_task);
    MaestroSimulatorAdapter maestro_sa(maestro_ca);

    if (quantum_task.is_dynamic) {
        JSON result = maestro_sa.simulate();
        return {
            {"counts", result.at("id_counts").at(quantum_task.id)},
            {"time_taken", result.at("time_taken")}
        };
    } else {
        return maestro_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa