#include "maestro_cc_simulator.hpp"
#include "maestro_adapters/maestro_computation_adapter.hpp"
#include "maestro_adapters/maestro_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

MaestroCCSimulator::MaestroCCSimulator()
{
    classical_channel.publish();
};

MaestroCCSimulator::MaestroCCSimulator(const std::string& group_id)
{
    classical_channel.publish(group_id);
};

// Distributed MaestroSimulator
JSON MaestroCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with, false);

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