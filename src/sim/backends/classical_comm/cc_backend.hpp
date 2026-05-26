#pragma once

#include <utility>
#include <vector>

#include "sim/backend.hpp"
#include "sim/simulator.hpp"
#include "quantum_task/quantum_task.hpp"
#include "cc_executor.hpp"

#include "utils/json.hpp"


namespace cunqa {
namespace sim {

class CCBackend final : public Backend {
public:
    std::string name = "CCBackend";
    std::string version = "0.0.1";
    std::pair<std::size_t, std::size_t> num_qubits = {32, 0};
    std::string description = "Backend with classical communications.";
    std::vector<std::vector<std::size_t>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::string simulator_name;
    
    CCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json): 
        executor_{std::move(simulator)}
    {
        auto& sim = executor_.simulator();

        if (!backend_json.empty()) {
            name = backend_json.at("name");
            version = backend_json.at("version");
            num_qubits = backend_json.at("num_qubits");
            description = backend_json.at("description");
            coupling_map = backend_json.at("coupling_map");
            basis_gates = backend_json.at("basis_gates");
            custom_instructions = backend_json.at("custom_instructions");
            simulator_name = sim.get_name();
        } else {
            simulator_name = sim.get_name();
            
            auto gates = sim.get_basis_gates();
            basis_gates.clear();
            basis_gates.reserve(gates.size());

            for (std::string_view gate : gates)
                basis_gates.emplace_back(gate);
        }
        sim.set_num_qubits(num_qubits);
    }

    inline JSON execute(const std::string& quantum_task_str) override
    {
        auto quantum_task_json = JSON::parse(quantum_task_str);
        auto id = quantum_task_json.at("id").get<std::string>();
        RunConfig run_config(quantum_task_json.at("config"));

        if (auto it = quantum_task_json.find("instructions"); it != quantum_task_json.end()) {
            auto& instructions = *it;
            last_quantum_task_ = QuantumTask(id, run_config, Circuit::from_json(instructions));
        }
        else if (auto it = quantum_task_json.find("params"); it != quantum_task_json.end())
            last_quantum_task_.update_params(it->get<std::vector<double>>());
        
        return executor_.execute(last_quantum_task_);
    }

    JSON to_json() const
    {
        return {  
            {"name", name}, 
            {"version", version},
            {"num_qubits", num_qubits}, 
            {"description", description},
            {"coupling_map", coupling_map},
            {"basis_gates", basis_gates}, 
            {"custom_instructions", custom_instructions},
            {"simulator", simulator_name}
        };
    }

private:
    CCExecutor executor_;
    QuantumTask last_quantum_task_;
};

} // End of sim namespace
} // End of cunqa namespace