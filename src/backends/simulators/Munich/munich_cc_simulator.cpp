#include <chrono>

#include "munich_cc_simulator.hpp"
#include "munich_adapters/munich_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"

#include "utils/constants.hpp"

namespace cunqa {
namespace sim {

MunichCCSimulator::MunichCCSimulator()
{
    classical_channel.publish();
};

MunichCCSimulator::MunichCCSimulator(const std::string& group_id)
{
    classical_channel.publish(group_id);
};

JSON MunichCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with, false);
    
    auto p_qca = std::make_unique<QuantumComputationAdapter>(quantum_task);
    CircuitSimulatorAdapter csa(std::move(p_qca));

    if (quantum_task.is_dynamic) {
        return csa.simulate(&classical_channel);
    } else {
        return csa.simulate(&backend);
    }
}

} // End namespace sim
} // End namespace cunqa

