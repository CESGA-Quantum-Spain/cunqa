#pragma once

#include "sim/backend.hpp"
#include "comm/classical_channel.hpp"

#include "utils/helpers/environment.hpp"

#include "comm_model.hpp"

namespace cunqa {
namespace sim {

class QCBackend final : public Backend {
public:
    QCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json)
        : classical_channel_{get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_TASK_PID")}
        , executor_id_{get_env_variable("SLURM_JOB_ID") + "_executor"}
    {
        name = "QCBackend";
        num_qubits = {2, 1};
        description = "Backend with quantum communications.";

        load_common_fields(backend_json, simulator.get());

        // The simulation happens at the executor, which reads these sections from the
        // published backend. Building the model here reports a wrong one at deployment.
        if (backend_json.contains("noise_model"))
            noise_model_ = backend_json.at("noise_model");
        if (backend_json.contains("comm_model"))
            comm_model_ = backend_json.at("comm_model");
        CommModel::from_backends({{"vQPU", to_json()}});

        classical_channel_.publish();
        auto ready = classical_channel_.recv_info(executor_id_);
        classical_channel_.connect(executor_id_);
    }

    JSON execute(const std::string& quantum_task_str) override
    {
        classical_channel_.send_info(quantum_task_str, executor_id_);
        auto message = classical_channel_.recv_info(executor_id_);
        return message != "" ? JSON::parse(message) : JSON();
    }

    JSON to_json() const override
    {
        auto j = base_json();
        if (!noise_model_.is_null())
            j["noise_model"] = noise_model_;
        if (!comm_model_.is_null())
            j["comm_model"] = comm_model_;
        return j;
    }

private:
    std::string executor_id_;
    comm::ClassicalChannel classical_channel_;
    JSON noise_model_;
    JSON comm_model_;
};

} // End of sim namespace
} // End of cunqa namespace
