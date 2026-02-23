#include <chrono>

#include "munich_cc_simulator.hpp"
#include "munich_adapters/munich_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"

#include "utils/constants.hpp"

using namespace std::string_literals;


namespace cunqa {
namespace sim {

MunichCCSimulator::MunichCCSimulator() : 
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

JSON MunichCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);
    
    auto p_qca = std::make_unique<QuantumComputationAdapter>(quantum_task);
    MunichSimulatorAdapter csa(std::move(p_qca));
    if (quantum_task.is_dynamic) {
        return csa.simulate(&classical_channel);
    } else {
        return csa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa

