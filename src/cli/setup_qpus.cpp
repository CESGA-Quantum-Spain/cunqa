#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <sys/file.h>
#include <fcntl.h> 
#include <unistd.h> 

#include "qpu.hpp"
#include "sim/simulators/simulator_factory.hpp"
#include "sim/backends/backend_factory.hpp"

#include "utils/helpers/environment.hpp"
#include "utils/constants.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

using namespace std::string_literals;
using namespace cunqa;
using namespace cunqa::sim;

std::string generate_noise_instructions(const cunqa::JSON& back_path_json, const std::string& family)
{
    std::string backend_path;

    if (back_path_json.contains("backend_path"))
        backend_path=back_path_json.at("backend_path").get<std::string>();
    else 
        backend_path = "default";

    std::string command("python "s + std::string(INSTALL_PATH) + "/cunqa/qiskit_deps/noise_instructions.py "s
                                   + back_path_json.at("noise_properties_path").get<std::string>() + " "s
                                   + backend_path + " "s
                                   + back_path_json.at("thermal_relaxation").get<std::string>() + " "s
                                   + back_path_json.at("readout_error").get<std::string>() + " "s
                                   + back_path_json.at("gate_error").get<std::string>() + " "s
                                   + family + " "s
                                   + back_path_json.at("fakeqmio").get<std::string>());       
    std::system(command.c_str());
    return "";
}

cunqa::JSON get_backend_json(int argc, char *argv[], std::string sim_arg, std::string family)
{
    cunqa::JSON backend_json;
    auto back_path_json = (argc == 6 ? cunqa::JSON::parse(std::string(argv[5])) : cunqa::JSON());
    if (back_path_json.contains("noise_properties_path")) {
        if (sim_arg != "Aer")
            throw std::runtime_error("Noise is only available with AER at the moment.");
        std::string fpath = get_cunqa_path()
            + "/tmp_noisy_backend_" 
            + get_env_variable("SLURM_JOB_ID") 
            + ".json";

        if (get_env_variable("SLURM_PROCID") == "0")
            generate_noise_instructions(back_path_json, family);
        else {
            int fd = open(fpath.c_str(), O_RDONLY);
            while (fd == -1 || flock(fd, LOCK_SH) != 0) {
                if (fd != -1) close(fd);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                fd = open(fpath.c_str(), O_RDONLY);
            }
            close(fd);
        }

        std::ifstream f(fpath);
        backend_json = cunqa::JSON::parse(f);
    } else if (back_path_json.contains("backend_path")) {
        std::ifstream f(back_path_json.at("backend_path").get<std::string>());
        backend_json = cunqa::JSON::parse(f);
    }
    return backend_json;
}

int main(int argc, char *argv[])
{
    std::string mode(argv[1]);
    std::string communications(argv[2]);
    std::string family(argv[3]);
    std::string sim_arg(argv[4]);

    if (family == "default") family = get_env_variable("SLURM_JOB_ID");
    auto name = get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_TASK_PID");
    auto backend_json = get_backend_json(argc, argv, sim_arg, family);

    QPU qpu(
        make_backend(make_simulator(sim_arg), communications, backend_json), 
        mode, 
        name, 
        family
    );
    qpu.turn_ON();

    return EXIT_SUCCESS;
}