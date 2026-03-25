#pragma once

#include <vector>
#include <string>
#include "utils/json.hpp"
#include "utils/constants.hpp"

namespace cunqa {
using namespace constants;

class QuantumTask {
    public:
    std::string id;
    std::vector<JSON> circuit;
    JSON config;
    std::vector<std::string> sending_to;
    bool is_dynamic = false; // C_IF gates & Communications

    QuantumTask() = default;
    QuantumTask(const std::string& quantum_task);

    void update_circuit(const std::string& quantum_task);
    
private:
    void update_params_(const std::vector<double> params);
};

std::string to_string(const QuantumTask& data);
StructuredQuantumTask from_quantum_task_to_structuredqtask(const QuantumTask& quantum_task);

} // End of cunqa namespace