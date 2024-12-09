#pragma once

#include "backend_config.hpp"
#include "net_config.hpp"
#include <nlohmann/json.hpp>
#include "../utils/helpers.hpp"

using json = nlohmann::json;

namespace config {

template <SimType sim_type = SimType::Aer>
class QPUConfig : Config {
public:
    backend::BackendConfig<sim_type> backend_config;
    net::NetConfig net_config;
    std::string filepath;

    QPUConfig(const json& config, const std::string& filepath) :
        filepath{filepath}
    {
        if (config.contains("backend"))
            this->backend_config = backend::BackendConfig<sim_type>(config.at("backend"));
        else
            this->backend_config = backend::BackendConfig<sim_type>();
        
        // If no net is specified in the config file, then the process net is defined
        if (config.contains("net"))
            this->net_config = net::NetConfig(config.at("net"));
        else {
            this->net_config = net::NetConfig::myNetConfig();
        }
    }

    void set_backendconfig(json backend_conf_json){
        this->backend_config = backend::BackendConfig<sim_type>(backend_conf_json);
    }

    void set_netconfig(json net_conf_json){
        this->net_config = net::NetConfig(net_conf_json);
    }

    inline void load(const json& config) override {

    }

    inline json dump() const override {
        return {};
    }
};

}