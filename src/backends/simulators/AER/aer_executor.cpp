#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

#include "aer_adapters/aer_simulator_adapter.hpp"
#include "aer_adapters/aer_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "aer_executor.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

AerExecutor::AerExecutor() : classical_channel{"executor"}
{
    std::ifstream in(constants::COMM_FILEPATH);

    if (!in.is_open()) {
        throw std::runtime_error("Error opening the communications file.");
    }

    JSON j;
    if (in.peek() != std::ifstream::traits_type::eof())
        in >> j;
    in.close();

    std::string job_id = std::getenv("SLURM_JOB_ID");

    for (const auto& [key, value]: j.items()) {
        if (key.rfind(job_id, 0) == 0) {
            auto qpu_endpoint = value["communications_endpoint"].get<std::string>();
            qpu_ids.push_back(qpu_endpoint);
            classical_channel.connect(qpu_endpoint);
            classical_channel.send_info(classical_channel.endpoint, qpu_endpoint);
        }
    }
};

AerExecutor::AerExecutor(const std::string& group_id) : classical_channel{"executor"}
{
    std::ifstream in(constants::COMM_FILEPATH);

    if (!in.is_open()) {
        throw std::runtime_error("Error opening the communications file.");
    }

    JSON j;
    if (in.peek() != std::ifstream::traits_type::eof())
        in >> j;
    in.close();

    for (const auto& [key, value]: j.items()) {
        if (key.rfind(group_id) == key.size() - group_id.size()) {
            auto qpu_endpoint = value["communications_endpoint"].get<std::string>();
            qpu_ids.push_back(qpu_endpoint);
            classical_channel.connect(qpu_endpoint);
            classical_channel.send_info(classical_channel.endpoint, qpu_endpoint);
        }
    }
};

void AerExecutor::run()
{
    std::vector<QuantumTask> quantum_tasks;
    std::vector<std::string> qpus_working;
    JSON quantum_task_json;
    std::string message;
    while (true) {
        for(const auto& qpu_id: qpu_ids) {
            LOGGER_DEBUG("Vamos a recibir el mensaje de: {}", qpu_id);
            message = classical_channel.recv_info(qpu_id);
            LOGGER_DEBUG("Recibimos el mensaje: {}", message);

            if(!message.empty()) {
                qpus_working.push_back(qpu_id);
                quantum_task_json = JSON::parse(message);
                quantum_tasks.push_back(QuantumTask(message));
            }
        }

        AerComputationAdapter qc(quantum_tasks);
        AerSimulatorAdapter aer_sa(qc);
        auto result = aer_sa.simulate(&classical_channel);
        
        // TODO: transform results to give each qpu its results
        std::string result_str = result.dump();

        for(const auto& qpu: qpus_working) {
            classical_channel.send_info(result_str, qpu);
        }

        qpus_working.clear();
        quantum_tasks.clear();
    }
}


} // End of sim namespace
} // End of cunqa namespace