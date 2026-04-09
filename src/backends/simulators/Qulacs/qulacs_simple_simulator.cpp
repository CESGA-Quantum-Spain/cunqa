#include "qulacs_simple_simulator.hpp"
#include "qulacs_adapters/qulacs_computation_adapter.hpp"
#include "qulacs_adapters/qulacs_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON QulacsSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    QulacsComputationAdapter qulacs_ca(quantum_task);
    QulacsSimulatorAdapter qulacs_sa(qulacs_ca);

    if (quantum_task.is_dynamic) {
        JSON result = qulacs_sa.simulate();
        return {
            {"counts", result.at("id_counts").at(quantum_task.id)},
            {"time_taken", result.at("time_taken")}
        };
    } else {
        return qulacs_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa