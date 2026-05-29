#pragma once

#include <utility>
#include <vector>

#include "sim/simulator.hpp"
#include "sim/backend.hpp"
#include "nc_executor.hpp"

#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

class NCBackend final : public Backend {
public:
    std::string name = "NCBackend";
    std::string version = "0.0.1";
    std::pair<std::size_t, std::size_t> num_qubits = {6, 0};
    std::string description = "Simple backend with no communications.";
    std::vector<std::vector<std::size_t>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::string simulator_name;
    JSON noise_model = {};
    std::string noise_properties_path;
    std::string noise_path;
    
    NCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json)
        : executor_{std::move(simulator)}
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

            noise_model = backend_json.at("noise_model");
            noise_properties_path = backend_json.at("noise_properties_path");
            noise_path = backend_json.at("noise_path");
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
        auto quantum_task = JSON::parse(quantum_task_str);

        // TODO: Use ID to Qjob unordered get
        if (quantum_task.contains("id")) {
            auto id = quantum_task.at("id").get<std::string>();
            quantum_task.erase("id");
        }

        bool is_dynamic = quantum_task.at("config").at("is_dynamic");

        return is_dynamic ? executor_.custom_execute(quantum_task)
                          : executor_.native_execute(quantum_task, noise_model);
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
            {"simulator", simulator_name},
            {"noise_model", noise_path},
            {"noise_properties_path", noise_properties_path}
        };
    }

private:
    NCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace