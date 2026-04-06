#include "qsim_cc_simulator.hpp"
#include "qsim_adapters/qsim_computation_adapter.hpp"
#include "qsim_adapters/qsim_simulator_adapter.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

QsimCCSimulator::QsimCCSimulator() :
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

// Distributed QsimSimulator
JSON QsimCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);

    QsimComputationAdapter qsim_ca(quantum_task);
    QsimSimulatorAdapter qsim_sa(qsim_ca);
    if (quantum_task.is_dynamic) {
        return qsim_sa.simulate(&classical_channel);
    } else {
        return qsim_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa