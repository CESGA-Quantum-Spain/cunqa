#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <optional>

#include "quantum_task.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class Backend {
public:
    virtual ~Backend() = default;
    virtual inline JSON execute(const QuantumTask& quantum_task) const = 0;
    virtual JSON to_json() const = 0;
};

} // End of sim namespace
} // End of cunqa namespace