#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

#include "munich_adapters/circuit_simulator_adapter.hpp"
#include "munich_adapters/quantum_computation_adapter.hpp"
#include "quantum_task.hpp"
#include "munich_executor.hpp"

#include "utils/json.hpp"
#include "logger.hpp"

namespace cunqa {
namespace sim {

MunichExecutor::MunichExecutor() : classical_channel{"executor"}
{
    std::string filename = std::string(std::getenv("STORE")) + "/.cunqa/communications.json";
    std::ifstream in(filename);

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
}

void MunichExecutor::run()
{
    std::vector<QuantumTask> quantum_tasks;
    JSON quantum_task_json;
    std::vector<std::string> qpus_working;
    std::string message;
    while (true) {
        for(const auto& qpu_id: qpu_ids) {
            message = classical_channel.recv_info(qpu_id);
            if(message != "") {
                qpus_working.push_back(qpu_id);
                quantum_task_json = JSON::parse(message);
                
                quantum_tasks.push_back(QuantumTask(message));
            }
        }

        auto qc = std::make_unique<QuantumComputationAdapter>(quantum_tasks);
        CircuitSimulatorAdapter simulator(std::move(qc));

        auto start = std::chrono::high_resolution_clock::now();
        auto result = simulator.simulate(1);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();
        
        // TODO: transformar los circuitos
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