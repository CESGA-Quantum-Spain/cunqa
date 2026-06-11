#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "json.hpp"

namespace {

    std::string lock_path_of(const std::string& filename)
    {
        return filename + ".lock";
    }

    std::string directory_of(const std::string& path)
    {
        auto pos = path.find_last_of('/');
        if (pos == std::string::npos) return ".";
        if (pos == 0) return "/";
        return path.substr(0, pos);
    }

    int open_lock(const std::string& filename)
    {
        const std::string lp = lock_path_of(filename);
        int fd = open(lp.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            perror("open - lock file");
            throw std::runtime_error("Failed to open lock file: " + lp);
        }

        return fd;
    }

    enum class LockMode { Read, Write };

    struct flock lock(const int& fd, const LockMode& mode)
    {
        struct flock fl;
        fl.l_type = (mode == LockMode::Write) ? F_WRLCK : F_RDLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0; // Lock entire file

        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - lock");
            throw std::runtime_error("Failed to acquire file lock");
        }
        return fl;
    }

    void unlock(const int& fd, struct flock& fl)
    {
        fl.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &fl) == -1)
            perror("fcntl - unlock");
    }

    cunqa::JSON read_data(const std::string& filename)
    {
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            if (errno == ENOENT) return cunqa::JSON{}; // missing file -> null
            perror("open");
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::string content;
        {
            constexpr size_t BUF_SIZE = 4096;
            char buf[BUF_SIZE];
            ssize_t n;
            while ((n = read(fd, buf, BUF_SIZE)) > 0) {
                content.append(buf, n);
            }
            if (n == -1) {
                const int saved_errno = errno;
                close(fd);
                errno = saved_errno;
                perror("read");
                throw std::runtime_error("Failed reading file");
            }
        }
        close(fd);

        cunqa::JSON j;
        if (!content.empty()) {
            try {
                j = cunqa::JSON::parse(content);
            } catch (...) {
                j = cunqa::JSON::object(); // fallback to empty object if file corrupted
            }
        }
        return j;
    }

    void write_data_atomic(const std::string& filename, const cunqa::JSON& j)
    {
        std::string output = j.dump(4);
        output.push_back('\n');

        const std::string dir = directory_of(filename);
        std::string tmpl = dir + "/.tmp_XXXXXX";
        std::vector<char> tmpl_buf(tmpl.begin(), tmpl.end());
        tmpl_buf.push_back('\0');

        int tfd = mkstemp(tmpl_buf.data());
        if (tfd == -1) {
            perror("mkstemp");
            throw std::runtime_error("Failed to create temporary file in: " + dir);
        }
        const std::string tmp_path(tmpl_buf.data());

        try {
            size_t total = 0;
            while (total < output.size()) {
                ssize_t w = write(tfd, output.data() + total, output.size() - total);
                if (w < 0) {
                    perror("write");
                    throw std::runtime_error("Failed to write complete JSON");
                }
                total += static_cast<size_t>(w);
            }

            if (fsync(tfd) == -1) {
                perror("fsync");
                throw std::runtime_error("Failed to fsync temporary file");
            }
            if (close(tfd) == -1) {
                tfd = -1;
                perror("close");
                throw std::runtime_error("Failed to close temporary file");
            }
            tfd = -1;

            if (chmod(tmp_path.c_str(), 0644) == -1) {
                perror("chmod");
            }

            if (rename(tmp_path.c_str(), filename.c_str()) == -1) {
                perror("rename");
                throw std::runtime_error("Failed to rename temporary file onto: " + filename);
            }

            int dir_fd = open(dir.c_str(), O_RDONLY | O_DIRECTORY);
            if (dir_fd != -1) {
                if (fsync(dir_fd) == -1) perror("fsync - directory");
                close(dir_fd);
            }
        } catch (...) {
            if (tfd != -1) close(tfd);
            unlink(tmp_path.c_str());
            throw;
        }
    }

} // End of anonymous namespace


namespace cunqa {

JSON read_file(const std::string &filename)
{
    int lock_fd = -1;
    try {
        lock_fd = open_lock(filename);
        auto fl = lock(lock_fd, LockMode::Read);
        auto j = read_data(filename);
        unlock(lock_fd, fl);
        close(lock_fd);
        return j;
    } catch (const std::exception &e) {
        if (lock_fd != -1) close(lock_fd);
        std::string msg =
            "Error reading JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }

    return {};
}

void write_on_file(JSON local_data, const std::string &filename, const std::string &id)
{
    int lock_fd = -1;
    try {
        lock_fd = open_lock(filename);
        auto fl = lock(lock_fd, LockMode::Write);
        auto j = read_data(filename);

        j[id] = local_data;

        write_data_atomic(filename, j);
        unlock(lock_fd, fl);
        close(lock_fd);
    } catch (const std::exception &e) {
        if (lock_fd != -1) close(lock_fd);
        std::string msg =
            "Error writing JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
}

void remove_from_file(const std::string &filename, const std::string &rm_key)
{
    int lock_fd = -1;
    try {
        lock_fd = open_lock(filename);
        auto fl = lock(lock_fd, LockMode::Write);
        auto j = read_data(filename);

        JSON out = JSON::object();
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            bool starts_with = key.rfind(rm_key, 0) == 0;
            if (!starts_with) {
                out[it.key()] = it.value();
            }
        }

        write_data_atomic(filename, out);
        unlock(lock_fd, fl);
        close(lock_fd);
    } catch (const std::exception &e) {
        if (lock_fd != -1) close(lock_fd);
        std::string msg =
            "Error writing JSON safely using POSIX (fcntl) locks.\nSystem message: ";
        throw std::runtime_error(msg + e.what());
    }
}

} // End of cunqa namespace
