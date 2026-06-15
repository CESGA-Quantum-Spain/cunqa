#pragma once

#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <ranges>
#include <stdexcept>

#include "sim/simulator.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class Backend {
public:
    std::string name;
    std::string version;
    std::pair<std::size_t, std::size_t> num_qubits;
    std::string description;
    std::vector<std::vector<std::size_t>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string simulator_name;

    virtual inline JSON execute(const std::string& quantum_task_str) = 0;
    virtual JSON to_json() const = 0;
    virtual ~Backend() = default;

protected:
    void load_common_fields(const JSON& backend_json, Simulator* simulator)
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
            std::size_t total_qubits = num_qubits.first + num_qubits.second;
            for (const auto& edge : coupling_map) {
                for (auto qubit : edge) {
                    if (qubit >= total_qubits)
                        throw std::invalid_argument(
                            "coupling_map contains qubit index " + std::to_string(qubit) +
                            " out of valid range [0, " + std::to_string(total_qubits - 1) + "]");
                }
            }
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

        simulator_name = simulator->get_name();
    }

    JSON base_json() const
    {
        return {
            {"name", name},
            {"version", version},
            {"num_qubits", num_qubits},
            {"description", description},
            {"coupling_map", coupling_map},
            {"basis_gates", basis_gates},
            {"simulator", simulator_name}
        };
    }
};

} // End of sim namespace
} // End of cunqa namespace
