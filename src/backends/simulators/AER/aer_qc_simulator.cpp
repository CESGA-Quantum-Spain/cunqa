#include "aer_qc_simulator.hpp"

#include <string>
#include <cstdlib>
#include <sys/file.h>
#include <unistd.h>

namespace cunqa {
namespace sim {

AerQCSimulator::AerQCSimulator()
{ 
    classical_channel.publish();
    auto executor_endpoint = classical_channel.recv_info("executor");
    std::string id_ = "executor";
    classical_channel.connect(executor_endpoint, id_);
    write_executor_endpoint(executor_endpoint); 
};

AerQCSimulator::AerQCSimulator(const std::string& group_id)
{
    classical_channel.publish(group_id);
    auto executor_endpoint = classical_channel.recv_info("executor");
    std::string id_ = "executor";
    classical_channel.connect(executor_endpoint, id_);
    write_executor_endpoint(executor_endpoint, group_id);
};

JSON AerQCSimulator::execute([[maybe_unused]] const QCBackend& backend, const QuantumTask& quantum_task)
{
    auto circuit = to_string(quantum_task);
    classical_channel.send_info(circuit, "executor");
    if (circuit != "") {
        auto results = classical_channel.recv_info("executor");
        return JSON::parse(results);
    }
    return JSON();
}

void AerQCSimulator::write_executor_endpoint(const std::string endpoint, const std::string& group_id)
{
    try {
        int file = open(constants::COMM_FILEPATH.c_str(), O_RDWR | O_CREAT, 0666);
        if (file == -1) {
            std::cerr << "Error al abrir el archivo" << std::endl;
            return;
        }
        flock(file, LOCK_EX);

        JSON j;
        std::ifstream file_in(constants::COMM_FILEPATH);

        if (file_in.peek() != std::ifstream::traits_type::eof())
            file_in >> j;
        file_in.close();

        // This two SLURM variables conform the ID of the process
        std::string local_id = std::getenv("SLURM_TASK_PID");
        std::string job_id = std::getenv("SLURM_JOB_ID");
        auto task_id = (group_id == "") ? job_id + "_" + local_id : job_id + "_" + local_id + "_" + group_id;
        
        j[task_id]["executor_endpoint"] = endpoint;

        std::ofstream file_out(constants::COMM_FILEPATH, std::ios::trunc);
        file_out << j.dump(4);
        file_out.close();

        flock(file, LOCK_UN);
        close(file);
    } catch(const std::exception& e) {
        std::string msg("Error writing the JSON simultaneously using locks.\nError message thrown by the system: "); 
        throw std::runtime_error(msg + e.what());
    }
}

} // End namespace sim
} // End namespace cunqa
