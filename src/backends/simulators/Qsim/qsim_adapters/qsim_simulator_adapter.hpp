#pragma once

#include <vector>

#include "qsim_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "classical_channel/classical_channel.hpp"
#include "backends/backend.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QsimSimulatorAdapter
{
public:
    QsimSimulatorAdapter() = default;
    QsimSimulatorAdapter(QsimComputationAdapter& qc) : qc{qc} {}

    JSON simulate(const Backend* backend);
    JSON simulate(comm::ClassicalChannel* classical_channel = nullptr, const bool allows_qc = false);

    QsimComputationAdapter qc;

};


} // End of sim namespace
} // End of cunqa namespace