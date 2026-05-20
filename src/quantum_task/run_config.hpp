#pragma once

#include <optional>

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
    std::vector<std::string> sending_to;
    std::optional<std::string> maestro_simulator;
    JSON device;
    JSON simulator_specifics;

    RunConfig() = default;

    RunConfig(const JSON& config) {
        shots = config.at("shots");
        method = config.at("method");
        avoid_parallelization = config.at("avoid_parallelization");
        is_dynamic = config.at("is_dynamic");
        num_clbits = config.at("num_clbits");
        num_qubits = config.at("num_qubits");
        seed = config.contains("seed") ? config.at("seed").get<int>() : NO_SEED;
        if (config.contains("sending_to"))
            sending_to = config.at("sending_to").get<std::vector<std::string>>();
        if (config.contains("maestro_simulator"))
            maestro_simulator = config.at("maestro_simulator").get<std::string>();

        device = config.at("device");

        simulator_specifics = config;
        simulator_specifics.erase("shots");
        simulator_specifics.erase("method");
        simulator_specifics.erase("avoid_parallelization");
        simulator_specifics.erase("num_clbits");
        simulator_specifics.erase("num_qubits");
        simulator_specifics.erase("seed");
        simulator_specifics.erase("sending_to");
        simulator_specifics.erase("maestro_simulator");
        simulator_specifics.erase("device");
    }

    JSON to_json() const {
        JSON config_json;
        config_json["shots"] = shots;
        config_json["method"] = method;
        config_json["avoid_parallelization"] = avoid_parallelization;
        config_json["is_dynamic"] = is_dynamic;
        config_json["num_clbits"] = num_clbits;
        config_json["num_qubits"] = num_qubits;
        config_json["seed"] = seed;
        config_json["sending_to"] = sending_to;
        
        // Handle optional maestro_simulator
        if (maestro_simulator.has_value()) {
            config_json["maestro_simulator"] = maestro_simulator.value();
        }
        
        config_json["device"] = device;
        config_json["simulator_specifics"] = simulator_specifics;
        
        return config_json;
    }
};
 
} // End of cunqa namespace