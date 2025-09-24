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
    return maestro_sa.simulate(&backend);
}

} // End namespace sim
} // End namespace cunqa