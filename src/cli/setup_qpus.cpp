
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "qpu.hpp"
#include "sim/no_comm/nc_backend.hpp"
#include "sim/simulators/simulator_factory.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"

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

    std::string command("python "s + INSTALL_PATH + "/cunqa/qiskit_deps/noise_instructions.py "s
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

int main(int argc, char *argv[])
{
    std::string mode(argv[1]);
    std::string communications(argv[2]);
    std::string family(argv[3]);
    std::string sim_arg(argv[4]);

    LOGGER_DEBUG("Aquí bien.");

    if (family == "default")
        family = std::getenv("SLURM_JOB_ID");
    std::string name = std::getenv("SLURM_JOB_ID") 
                     + "_"s 
                     + std::getenv("SLURM_TASK_PID");
    
    cunqa::JSON backend_json;
    auto back_path_json = (argc == 6 ? cunqa::JSON::parse(std::string(argv[5])) : cunqa::JSON());
    if (back_path_json.contains("noise_properties_path")) {
        if (sim_arg != "Aer")
            throw std::runtime_error("Noise is only available with AER at the moment.");
        std::string fpath = std::string(CUNQA_PATH) 
            + "/tmp_noisy_backend_" 
            + std::getenv("SLURM_JOB_ID") 
            + ".json";

        if (std::getenv("SLURM_PROCID") && std::string(std::getenv("SLURM_PROCID")) == "0")
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

    switch(murmur::hash(communications)) {
        case murmur::hash("nc"): 
        {
            LOGGER_DEBUG("Sim Arg {}.", sim_arg);
            auto backend = std::make_unique<NCBackend>(make_simulator(sim_arg), backend_json);
            QPU qpu(std::move(backend), mode, name, family);
            qpu.turn_ON();
            break;
        }
        case murmur::hash("cc"): 
            // TODO
            break;
        case murmur::hash("qc"):
            // TODO
            break;
        default:
            //LOGGER_ERROR("No {} communication method available.", communications);
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}