#include "qulacs_cc_simulator.hpp"
#include "qulacs_adapters/qulacs_computation_adapter.hpp"
#include "qulacs_adapters/qulacs_simulator_adapter.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

QulacsCCSimulator::QulacsCCSimulator() :
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")}
{
    classical_channel.publish();
};

// Distributed QulacsSimulator
JSON QulacsCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    for(const auto& qpu_id: quantum_task.sending_to)
        classical_channel.connect(qpu_id);

    QulacsComputationAdapter qulacs_ca(quantum_task);
    QulacsSimulatorAdapter qulacs_sa(qulacs_ca);
    if (quantum_task.is_dynamic) {
        return qulacs_sa.simulate(&classical_channel);
    } else {
        return qulacs_sa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa