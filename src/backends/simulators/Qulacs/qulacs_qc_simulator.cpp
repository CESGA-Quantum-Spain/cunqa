#include "qulacs_qc_simulator.hpp"

#include <string>
#include <cstdlib>
#include <sys/file.h>
#include <unistd.h>

using namespace std::string_literals;

namespace cunqa {
namespace sim {

QulacsQCSimulator::QulacsQCSimulator() : 
    classical_channel{std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID")},
    executor_id{std::getenv("SLURM_JOB_ID") + "_executor"s}
{
    classical_channel.publish();
    auto ready = classical_channel.recv_info(executor_id);
    classical_channel.connect(executor_id);
};

JSON QulacsQCSimulator::execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& quantum_task)
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