#include "aer_qc_simulator.hpp"

#include <string>
#include <cstdlib>
#include <sys/file.h>
#include <unistd.h>

namespace cunqa {
namespace sim {

AerQCSimulator::AerQCSimulator(const std::string& group_id) : 
    executor_id{"executor_" + group_id},
    classical_channel{executor_id}
{
    classical_channel.publish();
    auto ready = classical_channel.recv_info(executor_id);
    classical_channel.connect(executor_id);
};

JSON AerQCSimulator::execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& quantum_task)
{
    auto circuit = to_string(quantum_task);
    
    classical_channel.send_info(circuit, executor_id);
    if (circuit != "") {
        auto results = classical_channel.recv_info(executor_id);
        return JSON::parse(results);
    }
    return JSON();
}

} // End namespace sim
} // End namespace cunqa
