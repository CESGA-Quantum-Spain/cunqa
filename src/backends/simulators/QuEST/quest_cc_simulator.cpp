#include "quest_cc_simulator.hpp"
#include "quest_adapters/quest_computation_adapter.hpp"
#include "quest_adapters/quest_simulator_adapter.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

QuestCCSimulator::QuestCCSimulator() :
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

// Distributed QuestSimulator
JSON QuestCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);

    QuestComputationAdapter quest_ca(quantum_task);
    QuestSimulatorAdapter quest_sa(quest_ca);
    if (quantum_task.is_dynamic) {
        JSON result = quest_sa.simulate(&classical_channel);
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