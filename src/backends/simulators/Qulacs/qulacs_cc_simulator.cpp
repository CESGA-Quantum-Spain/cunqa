#include "qulacs_cc_simulator.hpp"
#include "qulacs_adapters/qulacs_computation_adapter.hpp"
#include "qulacs_adapters/qulacs_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

QulacsCCSimulator::QulacsCCSimulator()
{
    classical_channel.publish();
};

QulacsCCSimulator::QulacsCCSimulator(const std::string& group_id)
{
    classical_channel.publish(group_id);
};

// Distributed QulacsSimulator
JSON QulacsCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with, false);

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