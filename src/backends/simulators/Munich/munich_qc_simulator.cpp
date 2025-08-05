#include "munich_qc_simulator.hpp"

namespace cunqa {
namespace sim {

MunichQCSimulator::MunichQCSimulator()
{ 
    classical_channel.publish();
    auto executor_endpoint = classical_channel.recv_info("executor");
    classical_channel.connect(executor_endpoint, "executor");
};


JSON MunichQCSimulator::execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& quantum_task)
{
    auto circuit = to_string(quantum_task);
    classical_channel.send_info(circuit, "executor");
    if (circuit != "") {
        auto results = classical_channel.recv_info("executor");
        return JSON::parse(results);
    }
    return JSON();
}

} // End namespace sim
} // End namespace cunqa

