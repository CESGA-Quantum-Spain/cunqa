#pragma once

#include <vector>

#include "backend.hpp"
#include "quantum_task.hpp"
#include "simulators/simulator_strategy.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "logger.hpp"


namespace cunqa {
namespace sim {

struct CCConfig {
    std::string name = "CCBackend";
    std::string version = "0.0.1";
    int n_qubits = 32;
    std::string description = "Simple backend with classical communications.";
    std::vector<std::vector<int>> coupling_map;
    std::vector<std::string> basis_gates = constants::BASIS_GATES;
    std::string custom_instructions;
    std::vector<std::string> gates;
    JSON noise_model = {};
    std::string noise_properties_path;
    std::string noise_path;

    friend void from_json(const JSON& j, CCConfig &obj)
    {
        j.at("name").get_to(obj.name);
        j.at("version").get_to(obj.version);
        j.at("n_qubits").get_to(obj.n_qubits);
        j.at("description").get_to(obj.description);
        j.at("coupling_map").get_to(obj.coupling_map);
        j.at("basis_gates").get_to(obj.basis_gates);
        j.at("custom_instructions").get_to(obj.custom_instructions);
        j.at("gates").get_to(obj.gates);
        j.at("noise_model").get_to(obj.noise_model);
        j.at("noise_properties_path").get_to(obj.noise_properties_path);
        j.at("noise_path").get_to(obj.noise_path);
    }

    friend void to_json(JSON& j, const CCConfig& obj)
    {
        j = {   
            {"name", obj.name}, 
            {"version", obj.version},
            {"n_qubits", obj.n_qubits}, 
            {"description", obj.description},
            {"coupling_map", obj.coupling_map},
            {"basis_gates", obj.basis_gates}, 
            {"custom_instructions", obj.custom_instructions},
            {"gates", obj.gates},
            {"noise_model", obj.noise_path},
            {"noise_properties_path", obj.noise_properties_path}
        };
    }
};

class CCBackend final : public Backend {
public:
    CCConfig cc_config;
    
    CCBackend(const CCConfig& cc_config, std::unique_ptr<SimulatorStrategy<CCBackend>> simulator): 
        cc_config{cc_config},
        simulator_{std::move(simulator)}
    {
        config = cc_config;
        config["noise_model"] = cc_config.noise_model; // Not in to_json() to avoid the writing on qpus.json
    }

    CCBackend(CCBackend& cc_backend) = default;

    inline JSON execute(const QuantumTask& quantum_task) const override
    {
        return simulator_->execute(*this, quantum_task);
    }

    // TODO: Achieve this using the JSON adl serializer
    JSON to_json() const override 
    {
        JSON config_json = config;
        const auto simulator_name = simulator_->get_name();
        config_json["simulator"] = simulator_name;
        return config_json;
    }

private:
    std::unique_ptr<SimulatorStrategy<CCBackend>> simulator_;
};

} // End of sim namespace
} // End of cunqa namespace