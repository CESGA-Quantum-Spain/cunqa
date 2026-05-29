#pragma once

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class Backend {
public:
    virtual inline JSON execute(const std::string& quantum_task_str) = 0;
    virtual JSON to_json() const = 0;
};

} // End of sim namespace
} // End of cunqa namespace