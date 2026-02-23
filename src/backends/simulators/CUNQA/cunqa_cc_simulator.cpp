#include "cunqa_cc_simulator.hpp"
#include "cunqa_adapters/cunqa_computation_adapter.hpp"
#include "cunqa_adapters/cunqa_simulator_adapter.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

CunqaCCSimulator::CunqaCCSimulator() : 
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

JSON CunqaCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);

    CunqaComputationAdapter cunqa_ca(quantum_task);
    CunqaSimulatorAdapter cunqa_sa(cunqa_ca);
    if (quantum_task.is_dynamic) {
        return cunqa_sa.simulate(&classical_channel);
    } else {
        return cunqa_sa.simulate(&backend);
    }
}


} // End namespace sim
} // End namespace cunqa