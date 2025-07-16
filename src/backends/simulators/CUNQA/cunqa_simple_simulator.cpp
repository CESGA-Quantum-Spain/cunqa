
#include "cunqa_simple_simulator.hpp"
#include "cunqa_executors.hpp"

#include "quantum_task.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

JSON CunqaSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task)
{
    LOGGER_DEBUG("We are in the execute() method of SimpleCunqa.");
    return cunqa_execution_(quantum_task);
}

} // End namespace sim
} // End namespace cunqa