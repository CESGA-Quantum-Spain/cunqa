#pragma once

#include "utils/json.hpp"

namespace cunqa {

inline constexpr int NO_SEED = -1;

struct RunConfig {
    int shots;
    std::string method;
    bool avoid_parallelization;
    bool is_dynamic;
    int num_clbits;
    int num_qubits;
    int seed;
    JSON device;
    JSON simulator_specifics;

    RunConfig() = default;

    RunConfig(const JSON& config)
    {
        shots = config.at("shots");
        method = config.at("method");
        avoid_parallelization = config.at("avoid_parallelization");
        is_dynamic = config.at("is_dynamic");
        num_clbits = config.at("num_clbits");
        num_qubits = config.at("num_qubits");
        device = config.at("device");
        seed = config.contains("seed") ? config.at("seed").get<int>() : NO_SEED;
        
        simulator_specifics = config;
        simulator_specifics.erase("shots");
        simulator_specifics.erase("method");
        simulator_specifics.erase("avoid_parallelization");
        simulator_specifics.erase("num_clbits");
        simulator_specifics.erase("num_qubits");
        simulator_specifics.erase("device");
    }
};
 
} // End of cunqa namespace