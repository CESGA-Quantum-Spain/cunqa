#include "aer_simple_simulator.hpp"
#include "aer_adapters/aer_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

// Simple AerSimulator
AerSimpleSimulator::~AerSimpleSimulator() = default;

JSON AerSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    if (!quantum_task.is_dynamic) {
        return usual_execution_(backend, quantum_task);
    } else {
        return dynamic_execution_(quantum_task);
    } 
}

} // End namespace sim
} // End namespace cunqa