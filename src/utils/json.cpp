#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "json.hpp"

namespace {

    int open_file(const std::string& filename)
    {
        int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("open");
            throw std::runtime_error("Failed to open file: " + filename);
        }

        return fd;
    }

    struct flock lock_write(const int& fd)
    {
        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0; // Lock entire file

        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - lock");
            throw std::runtime_error("Failed to acquire file lock");
        }
        return fl;
    }

    struct flock lock_read(const int& fd)
    {
        struct flock fl{};
        fl.l_type = F_RDLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;

        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - lock_read");
            throw std::runtime_error("Failed to acquire read lock");
        }
        return fl;
    }

    void unlock(const int& fd, struct flock& fl)
    {
        if (fsync(fd) == -1) {
            perror("fsync");
        }

        // 8. Unlock and close
        fl.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &fl) == -1)
            perror("fcntl - unlock");
    }

    cunqa::JSON read_json(const int& fd) 
    {
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

        cunqa::JSON j;
        if (!content.empty()) return cunqa::JSON::parse(content);
        return nlohmann::json::object(); 
    }

    void write_json(const int& fd, const cunqa::JSON& j)
    {
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
    }

} // End of anonymous namespace


namespace cunqa {

void write_on_file(JSON local_data, const std::string &filename, const std::string &suffix)
{
    int fd = -1;
    try {
        fd = open_file(filename);
        auto fl = lock_write(fd);
        auto j = read_json(fd);

        // Get key and add data
        const char *pid_env = std::getenv("SLURM_TASK_PID");
        const char *job_env = std::getenv("SLURM_JOB_ID");
        std::string local_id = pid_env ? pid_env : "UNKNOWN";
        std::string job_id = job_env ? job_env : "UNKNOWN";
        std::string task_id =
            (suffix.empty()) ? (job_id + "_" + local_id)
                             : (job_id + "_" + local_id + "_" + suffix);
        j[task_id] = local_data;

        write_json(fd, j);
        unlock(fd, fl);
        close(fd);
    } catch (const std::exception &e) {
        if (fd != -1) close(fd);
        std::string msg =
            "Error writing JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
}

JSON read_from_file(const std::string &filename)
{
    int fd = -1;
    try {
        fd = open_file(filename);
        auto fl = lock_read(fd);
        auto j = read_json(fd);
        
        return j;
    } catch (const std::exception &e) {
        if (fd != -1) close(fd);
        throw;
    }
    
}

void remove_from_file(const std::string &filename, const std::string &rm_key)
{
    int fd = -1;
    try {
        fd = open_file(filename);
        auto fl = lock_write(fd);
        auto j = read_json(fd);

        // Filter: keep entries which JOB_ID is not the one attached 
        JSON out = JSON::object();
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            bool starts_with = key.rfind(rm_key, 0) == 0;
            if (!starts_with) {
                out[it.key()] = it.value();
            }
        }

        write_json(fd, out);
        unlock(fd, fl);
        close(fd);
    } catch (const std::exception &e) {
        if (fd != -1) close(fd);
        std::string msg =
            "Error writing JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
}

} // End of cunqa namespace