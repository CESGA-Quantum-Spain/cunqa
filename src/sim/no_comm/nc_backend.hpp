#pragma once

#include <vector>

#include "sim/simulator.hpp"
#include "sim/backend.hpp"
#include "quantum_task/quantum_task.hpp"
#include "nc_executor.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class NCBackend final : public Backend {
public:
    std::string name = "NCBackend";
    std::string version = "0.0.1";
    int n_qubits = 32;
    std::string description = "Simple backend with no communications.";
    std::vector<std::vector<int>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::vector<std::string> gates;
    std::string simulator_name;
    JSON noise_model = {};
    std::string noise_properties_path;
    std::string noise_path;
    
    NCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json) : 
        executor_{std::move(simulator)}
    { 
        if (!backend_json.empty()) {
            name = backend_json.at("name");
            version = backend_json.at("version");
            n_qubits = backend_json.at("n_qubits");
            description = backend_json.at("description");
            coupling_map = backend_json.at("coupling_map");
            basis_gates = backend_json.at("basis_gates");
            custom_instructions = backend_json.at("custom_instructions");
            gates = backend_json.at("gates");
            simulator_name = simulator->get_name();
            noise_model = backend_json.at("noise_model");
            noise_properties_path = backend_json.at("noise_properties_path");
            noise_path = backend_json.at("noise_path");
        }
    }

    inline JSON execute(const QuantumTask& quantum_task) override
    {
        return quantum_task.config.is_dynamic ? executor_.custom_execute(quantum_task)
                                              : executor_.native_execute(quantum_task, noise_model);
    }

    JSON to_json() const
    {
        return {{   
            {"name", name}, 
            {"version", version},
            {"n_qubits", n_qubits}, 
            {"description", description},
            {"coupling_map", coupling_map},
            {"basis_gates", basis_gates}, 
            {"custom_instructions", custom_instructions},
            {"gates", gates},
            {"simulator", simulator_name},
            {"noise_model", noise_path},
            {"noise_properties_path", noise_properties_path}
        }};
    }

private:
    NCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace