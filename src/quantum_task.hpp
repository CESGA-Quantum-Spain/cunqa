#pragma once

#include <vector>
#include <string>
#include "utils/json.hpp"

namespace cunqa {

class QuantumTask {
public:
    JSON circuit;
    JSON config;
    std::vector<std::string> sending_to;
    bool is_dynamic = false; // C_IF gates & Communications
    std::string id;

    QuantumTask() = default;
    QuantumTask(const std::string& quantum_task);
    QuantumTask(const JSON& circuit, const JSON& config): circuit(circuit), config(config) {};

    void update_circuit(const std::string& quantum_task);
    
private:
    void update_params_(const std::vector<double> params);
};

std::string to_string(const QuantumTask& data);

} // End of cunqa namespace