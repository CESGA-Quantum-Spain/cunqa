#include "aer_cc_simulator.hpp"
#include "aer_adapters/aer_computation_adapter.hpp"
#include "aer_adapters/aer_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

AerCCSimulator::AerCCSimulator()
{
    classical_channel.publish();
};

// Distributed AerSimulator
JSON AerCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with);

    AerComputationAdapter aer_ca(quantum_task);
    AerSimulatorAdapter aer_sa(aer_ca);
    if (quantum_task.is_dynamic) {
        return aer_sa.simulate(&classical_channel);
    } else {
        return aer_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa