#pragma once

#include <vector>

#include "sim/backend.hpp"
#include "quantum_task/quantum_task.hpp"
#include "sim/simulator.hpp"
#include "cc_executor.hpp"

#include "utils/json.hpp"


namespace cunqa {
namespace sim {

class CCBackend final : public Backend {
public:
    std::string name = "CCBackend";
    std::string version = "0.0.1";
    int n_qubits = 32;
    std::string description = "Simple backend with classical communications.";
    std::vector<std::vector<int>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::vector<std::string> gates;
    std::string simulator_name;
    
    CCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json): 
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
        }
    }

    inline JSON execute(const QuantumTask& quantum_task) override
    {
        return executor_.execute(quantum_task);
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
            {"simulator", simulator_name}
        }};
    }

private:
    CCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace