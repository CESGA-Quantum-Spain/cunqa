#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

#include "cunqa_adapters/cunqa_simulator_adapter.hpp"
#include "cunqa_adapters/cunqa_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "cunqa_executor.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

CunqaExecutor::CunqaExecutor(const std::size_t& n_qpus) : 
    classical_channel{std::getenv("SLURM_JOB_ID") + "_executor"s}
{
    JSON ids;
    do {
        JSON whole_ids = read_file(constants::COMM_FILEPATH);
        for (const auto& [key, value] : whole_ids.items()) {
            if(std::string(std::getenv("SLURM_JOB_ID")) == key.substr(0, key.find('_')))
                ids[key] = value;
        }
    } while (ids.size() != n_qpus);

    for (const auto& [key, _]: ids.items()) {
        qpu_ids.push_back(key);
        classical_channel.publish();
        classical_channel.connect(key);
        classical_channel.send_info("ready", key);
    }
};

void CunqaExecutor::run()
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

        CunqaComputationAdapter qc(quantum_tasks);
        CunqaSimulatorAdapter cunqa_sa(qc);
        auto result = cunqa_sa.simulate(&classical_channel);
        
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