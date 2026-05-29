#pragma once

#include <utility>
#include <vector>

#include "sim/backend.hpp"
#include "sim/simulator.hpp"
#include "comm/classical_channel.hpp"

#include "utils/helpers/environment.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QCBackend final : public Backend {
public:
    std::string name = "QCBackend";
    std::string version = "0.0.1";
    std::pair<std::size_t, std::size_t> num_qubits = {2, 1};
    std::string description = "Backend with quantum communications.";
    std::vector<std::vector<std::size_t>> coupling_map;
    std::vector<std::string> basis_gates;
    std::string custom_instructions;
    std::string simulator_name;

    
    QCBackend(std::unique_ptr<Simulator> simulator,  const JSON& backend_json)
        : classical_channel_{get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_TASK_PID")}
        , executor_id_{get_env_variable("SLURM_JOB_ID") + "_executor"}
    {
        if (!backend_json.empty()) {
            name = backend_json.at("name");
            version = backend_json.at("version");
            num_qubits = backend_json.at("num_qubits");
            description = backend_json.at("description");
            coupling_map = backend_json.at("coupling_map");
            basis_gates = backend_json.at("basis_gates");
            custom_instructions = backend_json.at("custom_instructions");
        } else {
            auto gates = simulator->get_basis_gates();
            basis_gates = std::vector<std::string>{gates.begin(),gates.end()};
        }
        simulator_name = simulator->get_name();

        classical_channel_.publish();
        auto ready = classical_channel_.recv_info(executor_id_);
        classical_channel_.connect(executor_id_);
    }

    inline JSON execute(const std::string& quantum_task_str) override
    {
        classical_channel_.send_info(quantum_task_str, executor_id_);
        auto message = classical_channel_.recv_info(executor_id_);
        return message != "" ? JSON::parse(message) : JSON();
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
            {"custom_instructions", custom_instructions}
        };
    }

private:
    std::string executor_id_;
    comm::ClassicalChannel classical_channel_;
};

} // End of sim namespace
} // End of cunqa namespace