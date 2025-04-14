#include <iostream>
#include <fstream>
#include <mpi.h>
#include <exception>
#include <sys/file.h>
#include <unistd.h>
#include "logger/logger.hpp"

#include "custom_json.hpp"

#include "logger/logger.hpp"

CustomJson::CustomJson()
{
    custom_json = json::object();
}

CustomJson::~CustomJson() = default;

void CustomJson::write(json local_data, const std::string &filename)
{
    //int flag;
    //MPI_Initialized(&flag);
    this->filename = filename;
    //flag ? CustomJson::_write_MPI(local_data) : CustomJson::_write_locks(local_data);
    CustomJson::_write_locks(local_data);
}

const std::string CustomJson::dump()
{
    return custom_json.dump(4);
}

void CustomJson::_write_MPI(json local_data)
{
    try {
        int world_rank, world_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);

        // Prepare all the sending data
        std::string local_data_str;
        int local_length;
        const char *send_data;
        local_data_str = local_data.dump();
        local_length = local_data_str.size();
        send_data = local_data_str.c_str();
        SPDLOG_LOGGER_DEBUG(logger, "JSON processed to write the data.");

        // Gather lengths to calculate the displacements
        int *recv_lengths;
        if (world_rank == 0)
            recv_lengths = new int[world_size];
        MPI_Gather(&local_length, 1, MPI_INT, recv_lengths, 1, MPI_INT, 0, MPI_COMM_WORLD);
        SPDLOG_LOGGER_DEBUG(logger, "Lengths gathered in order to calculate the displacements.");

        // Calculate displacements at root
        int *displs;
        int total_length = 0;
        if (world_rank == 0)
        {
            displs = new int[world_size];
            displs[0] = 0;
            for (int i = 0; i < world_size; ++i)
            {
                if (i > 0)
                    displs[i] = displs[i - 1] + recv_lengths[i - 1];
                total_length += recv_lengths[i];
            }
        }
        SPDLOG_LOGGER_DEBUG(logger, "Calculate displacements at root.");

        // Gather variable-length data at root
        char *recv_data;
        if (world_rank == 0)
            recv_data = new char[total_length];

        MPI_Gatherv(send_data, local_length, MPI_CHAR,
                    recv_data, recv_lengths, displs, MPI_CHAR,
                    0, MPI_COMM_WORLD);
        SPDLOG_LOGGER_DEBUG(logger, "Gather variable-length data at root.");

        // Reconstruct strings at root
        if (world_rank == 0) {
            std::vector<json> all_json_data(world_size);
            for (int i = 0; i < world_size; ++i) {
                std::string data_str(recv_data + displs[i], recv_lengths[i]);
                all_json_data[i] = json::parse(data_str);
            }

            for (const auto &obj : all_json_data)
                custom_json.update(obj);

            std::fstream file(filename, std::ios::out);
            file << custom_json.dump(4) << "\n";
        }
        SPDLOG_LOGGER_DEBUG(logger, "Succesfully resconstruction of the strings at root.");
    } catch(const std::exception& e) {
        std::string msg("Error writing the JSON simultaneously using MPI.\nError message thrown by the system: "); 
        throw std::runtime_error(msg + e.what());
    }
}

void CustomJson::_write_locks(json local_data)
{
    try {
        int file = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (file == -1) {
            std::cerr << "Error al abrir el archivo" << std::endl;
            return;
        }
        flock(file, LOCK_EX);

        json j;
        std::ifstream file_in(filename);

        if (file_in.peek() != std::ifstream::traits_type::eof())
            file_in >> j;
        file_in.close();

        // This is done because the task epilog does not 
        // have access to SLURM_TASK_PID variable
        //std::string local_id = std::getenv("SLURM_LOCALID");
        std::string local_id = std::getenv("SLURM_TASK_PID");
        std::string job_id = std::getenv("SLURM_JOB_ID");
        
        auto task_id = job_id + "_" + local_id;
        
        //j[std::to_string(j.size())] = local_data;
        j[task_id] = local_data;

        std::ofstream file_out(filename, std::ios::trunc);
        file_out << j.dump(4);
        file_out.close();
        SPDLOG_LOGGER_DEBUG(logger, "Succesfully written this process JSON into the file.");

        flock(file, LOCK_UN);
        close(file);
    } catch(const std::exception& e) {
        std::string msg("Error writing the JSON simultaneously using locks.\nError message thrown by the system: "); 
        throw std::runtime_error(msg + e.what());
    }
}
