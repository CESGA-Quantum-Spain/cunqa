#include <chrono>

#include "munich_cc_simulator.hpp"
#include "munich_adapters/circuit_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"

#include "utils/constants.hpp"

namespace cunqa {
namespace sim {

MunichCCSimulator::MunichCCSimulator()
{
    classical_channel.publish();
};

JSON MunichCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    std::vector<std::string> connect_with = quantum_task.sending_to;
    classical_channel.connect(connect_with);

    auto p_qca = std::make_unique<QuantumComputationAdapter>(quantum_task);
    CircuitSimulatorAdapter csa(std::move(p_qca));

    return csa.simulate(&classical_channel);
}

} // End namespace sim
} // End namespace cunqa

