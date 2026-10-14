#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <thread>
#include <chrono>

#include "qc_executor.hpp"
#include "comm_model.hpp"
#include "sim/run_config.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "utils/helpers/environment.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

namespace {

template <typename Predicate>
void poll_until_(Predicate&& ready,
                 const char* what,
                 std::size_t max_attempts = 600,
                 std::chrono::milliseconds interval = std::chrono::milliseconds{100})
{
    for (std::size_t attempt = 0; attempt < max_attempts; ++attempt) {
        if (ready())
            return;
        std::this_thread::sleep_for(interval);
    }
    throw std::runtime_error(std::string("Timed out waiting for ") + what);
}

} // End of anonymous namespace


namespace cunqa {
namespace sim {

QCExecutor::QCExecutor(
    std::shared_ptr<Simulator> simulator,
    const std::size_t& n_qpus
)
    : simulator_{simulator}
    , classical_channel_{get_env_variable("SLURM_JOB_ID") + "_executor"}
{
    auto slurm_job_id = get_env_variable("SLURM_JOB_ID");

    poll_until_([&] {
        qpus_ids_.clear();
        const auto communications = read_file(COMM_FILEPATH);
        for (const auto& [qpu_id, value] : communications.items())
            if (slurm_job_id == qpu_id.substr(0, qpu_id.find('_')))
                qpus_ids_.push_back(qpu_id);
        return qpus_ids_.size() == n_qpus;
    }, "QPU communication entries");

    classical_channel_.publish();
    for (const auto& qpu_id: qpus_ids_) {
        classical_channel_.connect(qpu_id);
        classical_channel_.send_info(std::string("ready"), qpu_id);
    }

    JSON backends;
    poll_until_([&] {
        backends.clear();
        const auto all_backends = read_file(QPUS_FILEPATH);
        for (const auto& [qpu_id, value] : all_backends.items()) {
            if (std::find(qpus_ids_.begin(), qpus_ids_.end(), qpu_id) != qpus_ids_.end())
                backends[qpu_id] = value;
        }
        return backends.size() == qpus_ids_.size();
    }, "QPUs backend definition");

    // Every vQPU contributes its qubits (data first, then communication) to the simulator
    // shared by all of them, together with its timing and noise description.
    std::vector<std::pair<std::string, JSON>> vqpu_backends;
    for (const auto& qpu_id : qpus_ids_)
        vqpu_backends.emplace_back(qpu_id, backends.at(qpu_id).at("backend"));
    auto model = CommModel::from_backends(vqpu_backends);

    std::size_t accumulated_qubits{0};
    for (const auto& vqpu : model.vqpus)
        accumulated_qubits += vqpu.num_data_qubits + vqpu.num_comm_qubits;
    simulator->num_qubits = accumulated_qubits;

    if (model.enabled)
        LOGGER_INFO("Communication noise model enabled for {} vQPUs.", model.vqpus.size());

    engine_ = std::make_unique<QCEngine>(simulator, std::move(model));
};

void QCExecutor::run()
{
    while (true) {
        std::vector<QuantumTask> quantum_tasks;
        std::vector<std::string> waiting_qpus;
        std::vector<JSON> results;
        try {
            for(const auto& qpu_id : qpus_ids_) {
                auto message = JSON::parse(classical_channel_.recv_info(qpu_id));
                if(!message.empty()) {
                    waiting_qpus.push_back(qpu_id);
                    RunConfig run_config(message.at("config"));
                    auto dynamic_circuit = std::make_unique<cunqa::DynamicCircuit>(message.at("instructions"));
                    quantum_tasks.emplace_back(
                        message.at("id").get<std::string>(),
                        run_config,
                        std::move(dynamic_circuit)
                    );
                }
            }

            if (quantum_tasks.empty()) {
                LOGGER_INFO("Nothing to run: empty circuits sent to executor.");
                continue;   // nothing to run this round
            }

            std::vector<RunConfig> run_configs;
            for (const auto& quantum_task: quantum_tasks)
                run_configs.push_back(quantum_task.config);
            simulator_->config = RunConfig::combine_configs(run_configs);

            results = engine_->run(quantum_tasks);
        } catch (const std::exception& e) {
            // The vQPUs that sent a task are waiting for its result, so they get the error
            // instead and the executor stays alive for the next round.
            LOGGER_ERROR("Error simulating the quantum-communications round: {}", e.what());
            results.assign(waiting_qpus.size(), JSON{{"ERROR", std::string(e.what())}});
            for (std::size_t t = 0; t < quantum_tasks.size(); ++t)
                results[t]["id"] = quantum_tasks[t].id;
            for (std::size_t t = 0; t < waiting_qpus.size(); ++t)
                classical_channel_.send_info(results[t].dump(), waiting_qpus[t]);
            continue;
        }

        for (std::size_t t = 0; t < quantum_tasks.size(); ++t) {
            results[t]["id"] = quantum_tasks[t].id;
            classical_channel_.send_info(results[t].dump(), quantum_tasks[t].config.qpu_id);
        }
    }
}

} // End of sim namespace
} // End of cunqa namespace
