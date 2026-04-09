#include "qsim_simple_simulator.hpp"
#include "qsim_adapters/qsim_computation_adapter.hpp"
#include "qsim_adapters/qsim_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON QsimSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    QsimComputationAdapter qsim_ca(quantum_task);
    QsimSimulatorAdapter qsim_sa(qsim_ca);

    if (quantum_task.is_dynamic) {
        JSON result = qsim_sa.simulate();
        return {
            {"counts", result.at("id_counts").at(quantum_task.id)},
            {"time_taken", result.at("time_taken")}
        };
    } else {
        return qsim_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa