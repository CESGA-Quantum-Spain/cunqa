#include <iostream>
#include <fstream>
#include <mpi.h>
#include <exception>
#include <sys/file.h>
#include <unistd.h>
//#include <fcntl.h>

#include "json.hpp"
#include "logger.hpp"

namespace cunqa {

void write_on_file(JSON local_data, const std::string &filename, const std::string& suffix) 
{
    try {
        int file = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (file == -1) {
            std::cerr << "Error al abrir el archivo" << std::endl;
            return;
        }
        flock(file, LOCK_EX);

        JSON j;
        std::ifstream file_in(filename);

        if (file_in.peek() != std::ifstream::traits_type::eof())
            file_in >> j;
        file_in.close();

        // This two SLURM variables conform the ID of the process
        std::string local_id = std::getenv("SLURM_TASK_PID");
        std::string job_id = std::getenv("SLURM_JOB_ID");
        auto task_id = (suffix == "") ? job_id + "_" + local_id : job_id + "_" + local_id + "_" + suffix;
        
        j[task_id] = local_data;

        std::ofstream file_out(filename, std::ios::trunc);
        file_out << j.dump(4);
        file_out.close();

        flock(file, LOCK_UN);
        close(file);
    } catch(const std::exception& e) {
        std::string msg("Error writing the JSON simultaneously using locks.\nError message thrown by the system: "); 
        throw std::runtime_error(msg + e.what());
    }
}

/* void write_on_file(JSON local_data, const std::string &filename, const std::string &suffix)
{
    try {
        int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("open");
            LOGGER_ERROR("Error opening file {}", filename);
            return;
        }

        // Define a write lock that covers the whole file
        struct flock fl;
        fl.l_type = F_WRLCK;   // Exclusive write lock
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;          // 0 = lock the whole file

        // Block until the lock is acquired
        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - lock");
            close(fd);
            throw std::runtime_error("Failed to acquire file lock with fcntl");
        }

        // --- Critical section ---
        JSON j;

        std::ifstream file_in(filename);
        if (file_in.peek() != std::ifstream::traits_type::eof())
            file_in >> j;
        file_in.close();

        // Retrieve SLURM variables for unique ID
        const char *pid_env = std::getenv("SLURM_TASK_PID");
        const char *job_env = std::getenv("SLURM_JOB_ID");
        std::string local_id = pid_env ? pid_env : "UNKNOWN";
        std::string job_id = job_env ? job_env : "UNKNOWN";

        std::string task_id = (suffix.empty()) ?
            job_id + "_" + local_id :
            job_id + "_" + local_id + "_" + suffix;

        j[task_id] = local_data;

        std::ofstream file_out(filename, std::ios::trunc);
        file_out << j.dump(4);
        file_out.close();
        // --- End of critical section ---

        // Unlock the file
        fl.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &fl) == -1) {
            perror("fcntl - unlock");
        }

        close(fd);
    } catch (const std::exception &e) {
        std::string msg = "Error writing the JSON simultaneously using fcntl locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
} */

}