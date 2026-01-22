#include "munich_simple_simulator.hpp"
#include "munich_adapters/munich_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"

#include "munich_helpers.hpp"

#include <chrono>

#include "CircuitSimulator.hpp"
#include "StochasticNoiseSimulator.hpp"
#include "ir/QuantumComputation.hpp"

namespace cunqa {
namespace sim {

JSON MunichSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task)
{
    auto p_qca = std::make_unique<QuantumComputationAdapter>(quantum_task);
    MunichSimulatorAdapter csa(std::move(p_qca));

    if (quantum_task.is_dynamic) 
        return csa.simulate();
    else
        return csa.simulate(&backend);
} 

} // End of sim namespace
} // End of cunqa namespace


