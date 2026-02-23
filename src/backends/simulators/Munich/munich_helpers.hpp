#pragma once

#include <iostream>
#include <vector>
#include "utils/constants.hpp"
#include "quantum_task.hpp"
#include "utils/json.hpp"
#include "utils/helpers/json_to_qasm2.hpp"

#include "logger.hpp"

namespace cunqa {

inline std::string quantum_task_to_Munich(const QuantumTask& quantum_task) 
{ 
    return json_to_qasm2(quantum_task.circuit, quantum_task.config);
}

} // End of cunqa namespace
