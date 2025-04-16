#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <queue>
#include <chrono>
#include "qpu.hpp"
#include "simulators/simulator.hpp"
#include "config/qpu_config.hpp"
#include "comm/server.hpp"
#include "comm/client.hpp"
#include "logger/logger.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    SPDLOG_LOGGER_DEBUG(logger,"Setup QPUs arguments: argc={} argv= {} {} {} {}", argc, argv[1], argv[2], argv[3], argv[4]);
    std::string mode(argv[1]);
    std::string family_name(argv[2]);
    std::string info_path(argv[3]);
    std::string simulator(argv[4]);
    std::string backend;
    json backend_json;

    json qpu_config_json = {};
    try {
        if (argc == 6) {
            backend = std::string(argv[5]);
            std::cout << backend << "\n";
            backend_json = json::parse(backend);
            if (backend_json.contains("fakeqmio_path")) {
                std::string command("python "s + std::getenv("INSTALL_PATH") + "/cunqa/fakeqmio.py "s + backend_json.at("fakeqmio_path").get<std::string>() + " "s +  std::getenv("SLURM_JOB_ID"));
                std::system(("ml load qmio/hpc gcc/12.3.0 qmio-tools/0.2.0-python-3.9.9 qiskit/1.2.4-python-3.9.9 2> /dev/null\n"s + command).c_str());
                std::string fq_path = std::getenv("STORE") + "/.cunqa/tmp_fakeqmio_backend_"s + std::getenv("SLURM_JOB_ID") + ".json"s;
                std::ifstream f(fq_path);
                qpu_config_json = json::parse(f);
            } else if (backend_json.contains("backend_path")) {
                std::ifstream f(backend_json.at("backend_path").get<std::string>());
                qpu_config_json = json::parse(f);
            } else {
                throw std::runtime_error(std::string("Format not correct. Must be {\"backend_path\" : \"path/to/backend/json\"} or {\"fakeqmio_path\" : \"path/to/qmio/calibration/json\"}"));
            }
            
        } else  if (argc < 4)
            SPDLOG_LOGGER_ERROR(logger, "Not a QPU configuration was given.");
            
        if (family_name == "default"){
            family_name = (std::string)std::getenv("SLURM_JOB_ID");
        }

        qpu_config_json["family_name"] = family_name;
        qpu_config_json["slurm_job_id"] = (std::string)std::getenv("SLURM_JOB_ID");

        if(auto search = SIM_NAMES.find(simulator); search != SIM_NAMES.end()) {
            if (search->second == SimType::Aer) {
                config::QPUConfig<SimType::Aer> qpu_config{mode, qpu_config_json, info_path};
                QPU<SimType::Aer> qpu(qpu_config);
                SPDLOG_LOGGER_DEBUG(logger, "Turning ON the QPUs with the AER simulator.");
                qpu.turn_ON();
            } else if (search->second == SimType::Munich) {
                SPDLOG_LOGGER_DEBUG(logger, "QPU_config: {}", qpu_config_json["noise"].dump(4));
                config::QPUConfig<SimType::Munich> qpu_config{mode, qpu_config_json, info_path};
                SPDLOG_LOGGER_DEBUG(logger, "QPU_config post qpu_config: {}", qpu_config.backend_config.noise_model.dump(4));
                QPU<SimType::Munich> qpu(qpu_config);
                SPDLOG_LOGGER_DEBUG(logger, "Turning ON the QPUs with the Munich simulator.");
                qpu.turn_ON();
            }  
        } else
            throw std::runtime_error(std::string("No simulator named ") + simulator);
    } catch (const std::exception& e) {
        SPDLOG_LOGGER_ERROR(logger, "Failed turning ON the QPUs because: \n\t{}", e.what());
    }
}