#pragma once

#include <cstdlib>
#include <string>
#include <stdexcept>

inline std::string get_env_variable(const char* env_var) {
    const char* value = std::getenv(env_var);
    if (!value) throw std::runtime_error(std::string(env_var) + " not set");
    return value;
}