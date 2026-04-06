#include "qsim_simple_simulator.hpp"
#include "qsim_adapters/qsim_computation_adapter.hpp"
#include "qsim_adapters/qsim_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON QsimSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    QsimComputationAdapter qsim_ca(quantum_task);
    QsimSimulatorAdapter qsim_sa(qsim_ca);

    if (quantum_task.is_dynamic) 
        return qsim_sa.simulate();
    else
        return qsim_sa.simulate(&backend);
}

} // End namespace sim
} // End namespace cunqa