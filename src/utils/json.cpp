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
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "json.hpp"

namespace fs = std::filesystem;

namespace {

    std::string directory_of(const std::string& path)
    {
        auto pos = path.find_last_of('/');
        if (pos == std::string::npos) return ".";
        if (pos == 0) return "/";
        return path.substr(0, pos);
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
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
            throw std::runtime_error("Failed to create directory " + dir + ": " + ec.message());

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
    try {
        return read_data(filename);
    } catch (const std::exception &e) {
        throw std::runtime_error("Error reading JSON file " + filename + ".\nSystem message: " + e.what());
    }
}

void write_file(const JSON& data, const std::string &filename)
{
    try {
        write_data_atomic(filename, data);
    } catch (const std::exception &e) {
        throw std::runtime_error("Error writing JSON file " + filename + ".\nSystem message: " + e.what());
    }
}

JSON read_dir(const std::string &dirname)
{
    JSON out = JSON::object();
    std::error_code ec;
    fs::directory_iterator it(dirname, ec), end;

    // The directory may not exist yet, or be removed while iterating (e.g. by the epilog)
    for (; !ec && it != end; it.increment(ec)) {
        const auto& path = it->path();
        const std::string name = path.filename().string();
        if (name.empty() || name[0] == '.' || path.extension() != ".json")
            continue;

        auto content = read_file(path.string());
        if (!content.is_null()) // Removed between listing and reading
            out[path.stem().string()] = std::move(content);
    }
    return out;
}

} // End of cunqa namespace
