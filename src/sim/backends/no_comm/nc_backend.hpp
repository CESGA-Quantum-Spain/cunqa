#pragma once

#include <utility>
#include <vector>
#include <ranges>
#include <algorithm>

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
    std::string simulator_name;
    JSON noise_model;
    
    NCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json)
    {
        if (backend_json.contains("name"))
            name = backend_json.at("name");
        
        if (backend_json.contains("version"))
            version = backend_json.at("version");
        
        if (backend_json.contains("num_qubits"))
            num_qubits = backend_json.at("num_qubits");
        simulator->set_num_qubits(num_qubits);

        if (backend_json.contains("description"))
            description = backend_json.at("description");

        if (backend_json.contains("coupling_map")) {
            coupling_map = backend_json.at("coupling_map");
            // TODO: Check if the coupling map is correctly defined
        }
        
        if (backend_json.contains("basis_gates")) {
            std::vector<std::string> defined_basis_gates = backend_json.at("basis_gates");
            auto sim_supported_gates = simulator->get_basis_gates();

            auto well_defined = std::ranges::all_of(defined_basis_gates, 
                [&sim_supported_gates](const std::string& gate) {
                    return std::ranges::find(sim_supported_gates, std::string_view{gate}) 
                        != sim_supported_gates.end();
                }
            );

            if (well_defined)
                basis_gates = defined_basis_gates;
            else
                throw std::invalid_argument("The defined basis gates are not supported by the simulator!");
        }

        if (backend_json.contains("noise_model")) {
            noise_model = backend_json.at("noise_model");
            const std::string noise_properties_path = noise_model.at("noise_properties_path");

            auto noise_properties = read_file(noise_properties_path);
            for (const auto& [key, value] : noise_model.items()) {
                if (key != "noise_properties_path")
                    noise_properties[key] = value;
            }

            simulator->set_noise_model(noise_properties);
        }

        simulator_name = simulator->get_name();
        executor_.set_simulator(std::move(simulator));
    }

    inline JSON execute(const std::string& quantum_task_str) override
    {
        auto quantum_task = JSON::parse(quantum_task_str);

        // TODO: Use ID to Qjob unordered get
        if (quantum_task.contains("id")) {
            auto id = quantum_task.at("id").get<std::string>();
            quantum_task.erase("id");
        }

        return executor_.execute(quantum_task);
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
            {"simulator", simulator_name},
            {"noise_model", noise_model}
        };
    }

private:
    NCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace