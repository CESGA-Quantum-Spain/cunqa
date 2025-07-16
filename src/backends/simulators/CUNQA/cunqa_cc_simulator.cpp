#include "cunqa_cc_simulator.hpp"
#include "cunqa_executors.hpp"


using namespace std::string_literals;
namespace {
    const auto store = getenv("STORE");
    const std::string filepath = store + "/.cunqa/communications.json"s;
}

namespace cunqa {
namespace sim {

CunqaCCSimulator::CunqaCCSimulator()
{
    classical_channel.publish();
}

JSON CunqaCCSimulator::execute(const CCBackend& backend, const QuantumTask& quantum_task)
{
    LOGGER_DEBUG("We are in the execute() method of CCCunqa.");
    return cunqa_execution_(quantum_task, &classical_channel);
}


} // End namespace sim
} // End namespace cunqa