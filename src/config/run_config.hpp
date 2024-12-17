#pragma once

#include <nlohmann/json.hpp>
#include <iostream>

using namespace std::literals;
using json = nlohmann::json;

namespace config{

class RunConfig {
public:
    int shots;
    std::string method;
    int memory_slots;

    RunConfig();
    RunConfig(json config);
};

void to_json(json& j, const RunConfig& run_conf);
void from_json(const json& j, RunConfig& run_conf);

RunConfig::RunConfig() :
    shots{1024},
    method{"statevector"},
    memory_slots{1}
{ }

RunConfig::RunConfig(json config)
{ 
    from_json(config, *this);
}

void to_json(json& j, const RunConfig& run_conf)
{
    j = {   
            {"shots", run_conf.shots}, 
            {"method", run_conf.method},
            {"memory_slots", run_conf.memory_slots}
        };
}

void from_json(const json& j, RunConfig& run_conf) 
{
    j.at("shots").get_to(run_conf.shots);
    j.at("method").get_to(run_conf.method);
    j.at("memory_slots").get_to(run_conf.memory_slots);
}; 

};