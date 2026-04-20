#include "quest_simple_simulator.hpp"
#include "quest_adapters/quest_computation_adapter.hpp"
#include "quest_adapters/quest_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

JSON QuestSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) 
{
    QuestComputationAdapter quest_ca(quantum_task);
    QuestSimulatorAdapter quest_sa(quest_ca);

    // Dynamic simulation always
    JSON result = quest_sa.simulate();
    return {
        {"counts", result.at("id_counts").at(quantum_task.id)},
        {"time_taken", result.at("time_taken")}
    };
    
}

} // End namespace sim
} // End namespace cunqa