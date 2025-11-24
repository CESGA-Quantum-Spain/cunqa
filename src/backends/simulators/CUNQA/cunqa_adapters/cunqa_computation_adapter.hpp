#pragma once

#include "quantum_task.hpp"

namespace cunqa {
namespace sim {

class CunqaComputationAdapter
{
public:
    CunqaComputationAdapter() = default;
    CunqaComputationAdapter(const QuantumTask quantum_task) : 
        quantum_tasks{quantum_task}
    { }
    CunqaComputationAdapter(const std::vector<QuantumTask> quantum_tasks) : 
        quantum_tasks{quantum_tasks}
    { }

    std::vector<QuantumTask> quantum_tasks;
};


} // End of sim namespace
} // End of cunqa namespace