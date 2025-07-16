#pragma once 

#include "quantum_task.hpp"

#include "ir/QuantumComputation.hpp"

namespace cunqa {
namespace sim {

// Extension of qc::QuantumComputation for Distributed Classical Communications
class QuantumComputationAdapter : public qc::QuantumComputation
{
public:
    // Constructors
    QuantumComputationAdapter(const QuantumTask& quantum_task) : 
        qc::QuantumComputation(quantum_task.config.at("num_qubits").get<std::size_t>(), quantum_task.config.at("num_clbits").get<std::size_t>()),
        quantum_tasks{quantum_task}
    { }
    QuantumComputationAdapter(const std::vector<QuantumTask>& quantum_tasks) : 
        qc::QuantumComputation(get_num_qubits_(quantum_tasks), get_num_clbits_(quantum_tasks)),
        quantum_tasks{quantum_tasks}
    { }

    std::vector<QuantumTask> quantum_tasks;

private:

    std::size_t get_num_qubits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t num_qubits = 0;
        for(const auto& quantum_task: quantum_tasks) {
            num_qubits += quantum_task.config.at("num_qubits").get<std::size_t>();
        }
        return num_qubits + 2;
    }

    std::size_t get_num_clbits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t num_clbits = 0;
        for(const auto& quantum_task: quantum_tasks) {
            num_clbits += quantum_task.config.at("num_clbits").get<std::size_t>();
        }
        return num_clbits;
    }

};

} // End of sim namespace
} // End of cunqa namespace