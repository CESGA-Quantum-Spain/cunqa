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
    CunqaComputationAdapter(const std::vector<QuantumTask> quantum_tasks, const int& n_comm_qubits) : 
        quantum_tasks{quantum_tasks}, num_comm_qubits{get_num_comm_qubits_(n_comm_qubits)}
    { }

    std::vector<QuantumTask> quantum_tasks;
    int num_comm_qubits = 0;

    int get_num_comm_qubits_(const int& ncq)
    {
        int tmp = (ncq % 2 != 0) ? ncq + 1 : ncq; // Ensure communication qubits always in pairs
        return tmp; 
    }
};


} // End of sim namespace
} // End of cunqa namespace