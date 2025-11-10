#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "json.hpp"
#include "logger.hpp"

namespace cunqa {

void write_on_file(JSON local_data, const std::string &filename, const std::string &suffix)
{
    int fd = -1;
    try {
        // 1. Open (or create) the file read/write
        fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("open");
            LOGGER_ERROR("Error opening file {}", filename);
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // 2. Acquire an exclusive lock (blocking)
        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0; // Lock entire file

        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - lock");
            LOGGER_ERROR("Error locking the file {}", filename);
            throw std::runtime_error("Failed to acquire file lock");
        }


        // 3. Read existing JSON content (if any)
        lseek(fd, 0, SEEK_SET);
        std::string content;
        {
            constexpr size_t BUF_SIZE = 4096;
            char buf[BUF_SIZE];
            ssize_t n;
            while ((n = read(fd, buf, BUF_SIZE)) > 0) {
                content.append(buf, n);
            }
            if (n == -1) {
                perror("read");
                throw std::runtime_error("Failed reading file");
            }
        }

        JSON j;
        if (!content.empty()) {
            try {
                j = JSON::parse(content);
            } catch (...) {
                j = JSON::object(); // fallback to empty object if file corrupted
            }
        }

        // 4. Compute unique task ID (SLURM vars)
        const char *pid_env = std::getenv("SLURM_TASK_PID");
        const char *job_env = std::getenv("SLURM_JOB_ID");
        std::string local_id = pid_env ? pid_env : "UNKNOWN";
        std::string job_id = job_env ? job_env : "UNKNOWN";
        std::string task_id =
            (suffix.empty()) ? (job_id + "_" + local_id)
                             : (job_id + "_" + local_id + "_" + suffix);

        // 5. Merge new data
        j[task_id] = local_data;

        // 6. Truncate and write updated JSON atomically
        std::string output = j.dump(4);
        if (ftruncate(fd, 0) == -1) {
            perror("ftruncate");
            throw std::runtime_error("Failed to truncate file");
        }

        lseek(fd, 0, SEEK_SET);
        ssize_t written = write(fd, output.c_str(), output.size());
        if (written < 0 || static_cast<size_t>(written) != output.size()) {
            perror("write");
            throw std::runtime_error("Failed to write complete JSON");
        }

        // 7. Ensure data reaches disk before unlocking
        if (fsync(fd) == -1) {
            perror("fsync");
        }

        // 8. Unlock and close
        fl.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &fl) == -1)
            perror("fcntl - unlock");

        close(fd);
    } catch (const std::exception &e) {
        if (fd != -1) close(fd);
        std::string msg =
            "Error writing JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
}

}