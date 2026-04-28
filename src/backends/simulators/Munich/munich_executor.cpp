#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

#include "munich_adapters/munich_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "munich_executor.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

using namespace std::string_literals;

namespace cunqa {
namespace sim {

MunichExecutor::MunichExecutor(const std::size_t& n_qpus) : 
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

    classical_channel.publish();
    for (const auto& [key, _]: ids.items()) {
        qpu_ids.push_back(key);
        classical_channel.connect(key);
        classical_channel.send_info("ready", key);
    }
};

void MunichExecutor::run()
{
    std::vector<QuantumTask> quantum_tasks;
    std::vector<std::string> qpus_working;
    JSON quantum_task_json;
    std::string message;
    while (true) {
        int qpu_count = 0;
        for(const auto& qpu_id: qpu_ids) {
            message = classical_channel.recv_info(qpu_id);
            if(!message.empty()) {
                qpus_working.push_back(qpu_id);
                quantum_task_json = JSON::parse(message);
                QuantumTask qtask(message);
                for (const auto& [qpu_id, qtask_id] : qpu_quantumtask_map) {
                    if (qtask.id == qtask_id) {
                        qtask.id += "_" + std::to_string(qpu_count);
                        break;
                    }
                }
                qpu_quantumtask_map[qpu_id] = qtask.id;
                quantum_tasks.push_back(qtask);
            }
            qpu_count++;
        }

        auto qc = std::make_unique<QuantumComputationAdapter>(quantum_tasks);
        MunichSimulatorAdapter simulator(std::move(qc));
        auto result = simulator.simulate(&classical_channel, true);
        
        for(const auto& qpu: qpus_working) {
            JSON qpu_result = {
                {"counts", result.at("id_counts").at(qpu_quantumtask_map[qpu])},
                {"time_taken", result.at("time_taken")}
            };
            std::string qpu_result_str = qpu_result.dump();
            classical_channel.send_info(qpu_result_str, qpu);
        }
        
        qpus_working.clear();
        quantum_tasks.clear();
        qpu_quantumtask_map.clear();
    }
}


} // End of sim namespace
} // End of cunqa namespace