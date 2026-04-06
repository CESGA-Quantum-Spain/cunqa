#pragma once

#include "quantum_task.hpp"

namespace cunqa {
namespace sim {

class QsimComputationAdapter
{
public:
    QsimComputationAdapter() = default;
    QsimComputationAdapter(const QuantumTask quantum_task) : 
        quantum_tasks{quantum_task}
    { }
    QsimComputationAdapter(const std::vector<QuantumTask> quantum_tasks) : 
        quantum_tasks{quantum_tasks}
    { }

    std::vector<QuantumTask> quantum_tasks;
};


} // End of sim namespace
} // End of cunqa namespace