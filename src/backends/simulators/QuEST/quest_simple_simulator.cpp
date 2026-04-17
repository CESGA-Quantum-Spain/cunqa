#include "quest_simple_simulator.hpp"
#include "quest_adapters/quest_computation_adapter.hpp"
#include "quest_adapters/quest_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON QuestSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    QuestComputationAdapter quest_ca(quantum_task);
    QuestSimulatorAdapter quest_sa(quest_ca);

    if (quantum_task.is_dynamic) {
        JSON result = quest_sa.simulate();
        return {
            {"counts", result.at("id_counts").at(quantum_task.id)},
            {"time_taken", result.at("time_taken")}
        };
    } else {
        return quest_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa