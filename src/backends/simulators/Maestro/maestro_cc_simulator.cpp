#include "maestro_cc_simulator.hpp"
#include "maestro_adapters/maestro_computation_adapter.hpp"
#include "maestro_adapters/maestro_simulator_adapter.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

MaestroCCSimulator::MaestroCCSimulator() : 
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

// Distributed MaestroSimulator
JSON MaestroCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);

    MaestroComputationAdapter maestro_ca(quantum_task);
    MaestroSimulatorAdapter maestro_sa(maestro_ca);
    if (quantum_task.is_dynamic) {
        return maestro_sa.simulate(&classical_channel);
    } else {
        return maestro_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa