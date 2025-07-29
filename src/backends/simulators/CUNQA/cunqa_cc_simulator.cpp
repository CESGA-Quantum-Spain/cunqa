#include "cunqa_cc_simulator.hpp"
#include "cunqa_executors.hpp"

namespace cunqa {
namespace sim {

CunqaCCSimulator::CunqaCCSimulator()
{
    classical_channel.publish();
}

JSON CunqaCCSimulator::execute([[maybe_unused]] const CCBackend& backend, const QuantumTask& quantum_task)
{
    LOGGER_DEBUG("We are in the execute() method of CCCunqa.");
    return cunqa_execution_(quantum_task, &classical_channel);
}


} // End namespace sim
} // End namespace cunqa