#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/json.hpp"

namespace cunqa {

inline constexpr int NO_SEED = -1;

struct RunConfig {
    std::string qpu_id;
    int shots;
    std::string method;
    bool avoid_parallelization;
    bool is_dynamic;
    int num_clbits;
    int seed;
    std::vector<std::string> sending_to;
    JSON device;
    JSON simulator_specifics;

    RunConfig() = default;

    RunConfig(const JSON& config)
    {
        qpu_id = config.at("qpu_id"); 
        shots = config.at("shots");
        method = config.at("method");
        avoid_parallelization = config.at("avoid_parallelization");
        is_dynamic = config.at("is_dynamic");
        num_clbits = config.at("num_clbits");
        seed = config.contains("seed") ? config.at("seed").get<int>() : NO_SEED;
        if (config.contains("sending_to"))
            sending_to = config.at("sending_to").get<std::vector<std::string>>();

        device = config.at("device");

        simulator_specifics = config;
        simulator_specifics.erase("shots");
        simulator_specifics.erase("method");
        simulator_specifics.erase("avoid_parallelization");
        simulator_specifics.erase("num_clbits");
        simulator_specifics.erase("seed");
        simulator_specifics.erase("sending_to");
        simulator_specifics.erase("device");
    }

    JSON to_json() const {
        JSON json;
        json["qpu_id"] = qpu_id;
        json["shots"] = shots;
        json["method"] = method;
        json["avoid_parallelization"] = avoid_parallelization;
        json["is_dynamic"] = is_dynamic;
        json["num_clbits"] = num_clbits;
        json["seed"] = seed;
        json["sending_to"] = sending_to;
        json["device"] = device;
        json["simulator_specifics"] = simulator_specifics;
        return json;
    }

    operator JSON() const {
        return to_json();
    }

    static RunConfig combine_configs(std::vector<RunConfig> configs)
    {
        RunConfig combined_config;
        auto base_config = configs[0];

        {
            auto same_method = std::all_of(
                configs.begin(), 
                configs.end(), 
                [base_config](RunConfig config) { 
                    return base_config.method == config.method; 
                });
            if (!same_method)
                throw std::runtime_error("Every quantum task with quantum communications has "
                                        "to have the same simulation method.");

            auto same_shots = std::all_of(
                configs.begin(), 
                configs.end(), 
                [base_config](RunConfig config) { 
                    return base_config.shots == config.shots; 
                });
            if (!same_shots)
                throw std::runtime_error("Every QPU with quantum communications has to have "
                                        "the same number of shots assigned.");

            auto same_parallelization = std::all_of(
                configs.begin(), 
                configs.end(), 
                [base_config](RunConfig config) { 
                    return base_config.avoid_parallelization == config.avoid_parallelization; 
                });
            if (!same_parallelization)
                throw std::runtime_error("Every quantum task with quantum communications " 
                                        "has to have activated or desactivated the parallelization.");
            
            auto same_seed = std::all_of(
                configs.begin(), 
                configs.end(), 
                [base_config](RunConfig config) { 
                    return base_config.seed == config.seed; 
                });
            if (!same_seed)
                combined_config.seed = NO_SEED; 
            else 
                combined_config.seed = base_config.seed;
                
            auto same_device = std::all_of(
                configs.begin(), 
                configs.end(), 
                [base_config](RunConfig config) { 
                    return base_config.device.at("device_name") == config.device.at("device_name"); 
                });
            if (!same_device)
                throw std::runtime_error("Every quantum task with quantum communications " 
                                        "has to be simulated in the same type of device.");
        }

        combined_config.qpu_id = "NONE";
        combined_config.shots = base_config.shots;
        combined_config.method = base_config.method;
        combined_config.avoid_parallelization = base_config.avoid_parallelization;
        combined_config.is_dynamic = true;
        combined_config.device = base_config.device;
        combined_config.simulator_specifics = base_config.simulator_specifics;
        combined_config.num_clbits = 0;

        for (const auto& config: configs) {
            combined_config.num_clbits += config.num_clbits;
        }

        return combined_config;
    }
};
 
} // End of cunqa namespace