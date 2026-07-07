#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

#include "write_sbatch.hpp"

#include "logger.hpp"

namespace fs = std::filesystem;

namespace {

class ScopedTempFile {
public:
    ScopedTempFile()
    {
        auto template_path = (fs::temp_directory_path() / "qraise_sbatch_XXXXXX").string();
        const int fd = ::mkstemp(template_path.data());
        if (fd == -1)
            throw std::runtime_error(
                "Could not create temporary sbatch file: " + std::string(std::strerror(errno))
            );

        ::close(fd);
        path_ = std::move(template_path);
    }

    ~ScopedTempFile() noexcept
    {
        std::error_code ec;
        //fs::remove(path_, ec);
    }

    ScopedTempFile(const ScopedTempFile&) = delete;
    ScopedTempFile& operator=(const ScopedTempFile&) = delete;

    const fs::path& path() const noexcept
    {
        return path_;
    }

    void save_as(const fs::path& destination) const
    {
        if (destination.empty())
            throw std::runtime_error("Cannot save temporary file: destination path is empty");

        if (destination.has_parent_path())
            fs::create_directories(destination.parent_path());

        fs::copy_file(
            path_,
            destination,
            fs::copy_options::overwrite_existing
        );
    }

private:
    fs::path path_;
};

std::string shell_quote(const fs::path& path)
{
    std::string result = "'";
    for (const char c : path.string()) {
        if (c == '\'')
            result += "'\\''";
        else
            result += c;
    }
    result += "'";
    return result;
}

int normalize_system_status(const int status)
{
    if (status == -1) return EXIT_FAILURE;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        // true ensures an error is raised if qraise receives an unrecognized flag.
        const auto args = argparse::parse<CunqaArgs>(argc, argv, true);

        const ScopedTempFile sbatch_file;

        {
            std::ofstream output(sbatch_file.path());
            if (!output) {
                throw std::runtime_error(
                    "Could not open temporary sbatch file: " + sbatch_file.path().string()
                );
            }

            write_sbatch(output, args);

            output.close();
            if (!output) {
                throw std::runtime_error(
                    "Failed while writing temporary sbatch file: " + sbatch_file.path().string()
                );
            }

            //sbatch_file.save_as("my_sbatch_script.sh");
        }

        const std::string command = "sbatch --parsable " + shell_quote(sbatch_file.path());
        const int status = std::system(command.c_str());
        const int exit_code = normalize_system_status(status);

        if (exit_code != EXIT_SUCCESS)
            LOGGER_ERROR("sbatch failed with exit code {}", exit_code);

        return exit_code;
    } catch (const std::exception& e) {
        LOGGER_ERROR("qraise failed:\n{}", e.what());
        return EXIT_FAILURE;
    }
}