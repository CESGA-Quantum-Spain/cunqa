#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MunichSimulator {
public:
    json execute(json circuit_json, const config::run::RunConfig& run_config) {
        std::cerr << "Yet to be implemented\n";
        return json();
    }
};