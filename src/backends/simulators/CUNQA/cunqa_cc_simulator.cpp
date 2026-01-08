#include "cunqa_cc_simulator.hpp"
#include "cunqa_adapters/cunqa_computation_adapter.hpp"
#include "cunqa_adapters/cunqa_simulator_adapter.hpp"

namespace cunqa {
namespace sim {

CunqaCCSimulator::CunqaCCSimulator(const std::string& group_id)
{
    classical_channel.publish(group_id);
};

JSON CunqaCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with, false);

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