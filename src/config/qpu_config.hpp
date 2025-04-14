#pragma once

#include "backend_config.hpp"
#include "net_config.hpp"
#include <nlohmann/json.hpp>
#include "../utils/helpers.hpp"

using json = nlohmann::json;

namespace config {

template <SimType sim_type = SimType::Aer>
class QPUConfig {
public:
    std::string family_name;
    BackendConfig<sim_type> backend_config;
    NetConfig net_config;
    std::string filepath;

    QPUConfig(const json& config, const std::string& filepath) :
        filepath{filepath}, family_name(config.at("family_name"))
    {
        if (config.contains("backend"))
            this->backend_config = config.contains("noise") ? 
                           BackendConfig<sim_type>(config.at("backend"), config.at("noise")) : 
                           BackendConfig<sim_type>(config.at("backend"), json());
        else
            this->backend_config = BackendConfig<sim_type>();
        
        if (config.contains("net"))
            this->net_config = NetConfig(config.at("net"));
        else {
            this->net_config = NetConfig::myNetConfig();
        }
    }

    void set_backendconfig(json backend_conf_json){
        this->backend_config = BackendConfig<sim_type>(backend_conf_json);
    }

    void set_netconfig(json net_conf_json){
        this->net_config = NetConfig(net_conf_json);
    }
};

// ------------------------------------------------
// ----- Conversion functions for json format -----
// ------------------------------------------------
template <SimType sim_type>
void to_json(json& j, const QPUConfig<sim_type>& qpu_config)
{
    std::string family_name = qpu_config.family_name;
    json net = qpu_config.net_config;
    json backend = qpu_config.backend_config;

    j = {   
            {"family_name", family_name},
            {"net", net}, 
            {"backend", backend}
        };
}

template <SimType sim_type>
void from_json(const json& j, QPUConfig<sim_type>& qpu_config) 
{
    qpu_config.fanmily_name = j.at("family_name");
    qpu_config.net_config = j.at("net").template get<NetConfig>();
    qpu_config.backend_config = j.at("backend").template get<BackendConfig<sim_type>>();
}; 

}