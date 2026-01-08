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

AerExecutor::AerExecutor(const std::string& group_id) : classical_channel{"executor_" + group_id}
{
    JSON ids = read_file(constants::COMM_FILEPATH);
    for (const auto& [key, _]: ids.items()) {
        if (key.rfind(group_id) == key.size() - group_id.size()) {
            qpu_ids.push_back(key);
            classical_channel.publish();
            classical_channel.connect(key);
            classical_channel.send_info("ready", key);
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
            message = classical_channel.recv_info(qpu_id);

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