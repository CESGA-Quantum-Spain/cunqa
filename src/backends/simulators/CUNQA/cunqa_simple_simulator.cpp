#include "cunqa_simple_simulator.hpp"

#include "cunqa_adapters/cunqa_computation_adapter.hpp"
#include "cunqa_adapters/cunqa_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON CunqaSimpleSimulator::execute([[maybe_unused]] const SimpleBackend& backend, const QuantumTask& quantum_task)
{
    CunqaComputationAdapter cunqa_ca(quantum_task);
    CunqaSimulatorAdapter cunqa_sa(cunqa_ca);
    
    if (quantum_task.is_dynamic) 
        return cunqa_sa.simulate();
    else
        return cunqa_sa.simulate(&backend);
}

} // End namespace sim
} // End namespace cunqa