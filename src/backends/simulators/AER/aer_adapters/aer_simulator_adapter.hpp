#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/simple_backend.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

JSON usual_execution_(const SimpleBackend& backend, const QuantumTask& quantum_task);
JSON dynamic_execution_(const QuantumTask& quantum_task, comm::ClassicalChannel* classical_channel = nullptr);

} // End of sim namespace
} // End of cunqa namespace