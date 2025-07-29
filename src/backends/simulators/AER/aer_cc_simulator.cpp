#include "aer_cc_simulator.hpp"
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
    // Add the classical channel
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with);

    return dynamic_execution_(quantum_task, &classical_channel); 
}

} // End namespace sim
} // End namespace cunqa