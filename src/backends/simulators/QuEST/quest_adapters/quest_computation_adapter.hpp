#pragma once

#include "quantum_task.hpp"

namespace cunqa {
namespace sim {

class QuestComputationAdapter
{
public:
    QuestComputationAdapter() = default;
    QuestComputationAdapter(const QuantumTask quantum_task) : 
        quantum_tasks{quantum_task}
    { }
    QuestComputationAdapter(const std::vector<QuantumTask> quantum_tasks) : 
        quantum_tasks{quantum_tasks}
    { }

    std::vector<QuantumTask> quantum_tasks;
};


} // End of sim namespace
} // End of cunqa namespace