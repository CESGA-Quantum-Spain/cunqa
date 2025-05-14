#pragma once

#include <vector>

#include "backend.hpp"
#include "quantum_task.hpp"
#include "backends/simple_backend.hpp"
#include "simulators/simulator_strategy.hpp"

#include "utils/json.hpp"
#include "logger.hpp"


namespace cunqa {
namespace sim {

struct SimpleConfig {
    std::string name = "SimpleSimulator";
    std::string version = "0.0.1";
    int n_qubits = 32;
    std::string description = "Simple backend with no communications.";
    std::vector<std::vector<int>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::vector<std::string> gates;
    JSON noise_model;

    friend void from_json(const JSON& j, SimpleConfig &obj)
    {
        j.at("name").get_to(obj.name);
        j.at("version").get_to(obj.version);
        j.at("n_qubits").get_to(obj.n_qubits);
        j.at("description").get_to(obj.description);
        j.at("description").get_to(obj.description);
        j.at("coupling_map").get_to(obj.coupling_map);
        j.at("basis_gates").get_to(obj.basis_gates);
        j.at("custom_instructions").get_to(obj.custom_instructions);
        j.at("gates").get_to(obj.gates);
        j.at("noise_model").get_to(obj.noise_model);
    }

    friend void to_json(JSON& j, const SimpleConfig& obj) 
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
            {"noise_model", obj.noise_model}
        };
    }
    
};

class SimpleBackend final : public Backend {
public:
    SimpleConfig config;
    
    SimpleBackend(const SimpleConfig& config, std::unique_ptr<SimulatorStrategy<SimpleBackend>> simulator) : 
        config{config},
        simulator_{std::move(simulator)}
    { }

    SimpleBackend(SimpleBackend& simple_backend) = default;

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
    std::unique_ptr<SimulatorStrategy<SimpleBackend>> simulator_;
};

} // End of sim namespace
} // End of cunqa namespace