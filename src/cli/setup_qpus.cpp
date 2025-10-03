
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "qpu.hpp"
#include "backends/simple_backend.hpp"
#include "backends/cc_backend.hpp"
#include "backends/simulators/AER/aer_simple_simulator.hpp"
#include "backends/simulators/AER/aer_cc_simulator.hpp"
#include "backends/simulators/AER/aer_qc_simulator.hpp"
#include "backends/simulators/Munich/munich_simple_simulator.hpp"
#include "backends/simulators/Munich/munich_cc_simulator.hpp"
#include "backends/simulators/Munich/munich_qc_simulator.hpp"
#include "backends/simulators/CUNQA/cunqa_simple_simulator.hpp"
#include "backends/simulators/CUNQA/cunqa_cc_simulator.hpp"
#include "backends/simulators/CUNQA/cunqa_qc_simulator.hpp"

#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"
#include "logger.hpp"

using namespace std::string_literals;

using namespace cunqa;
using namespace cunqa::sim;

template<typename Simulator, typename Config, typename BackendType>
void turn_ON_QPU(const JSON& backend_json, const std::string& mode, const std::string& family)
{
    std::unique_ptr<Simulator> simulator = std::make_unique<Simulator>();
    Config config;
    if (!backend_json.empty())
        config = backend_json;
    JSON config_json = config;
    QPU qpu(std::make_unique<BackendType>(config, std::move(simulator)), mode, family);
    LOGGER_DEBUG("QPU instantiated.");
    qpu.turn_ON();
}

std::string generate_noise_instructions(JSON back_path_json, std::string& family)
{
    std::string backend_path;

    if (back_path_json.contains("backend_path")){
        LOGGER_DEBUG("backend_path provided");
        backend_path=back_path_json.at("backend_path").get<std::string>();
    } else {
        LOGGER_DEBUG("No backend_path provided, defining backend from noise_properties.");
        backend_path = "default";
    }
    std::string command("python "s + std::getenv("STORE") + "/.cunqa/noise_model/noise_instructions.py "s
                                   + back_path_json.at("noise_properties_path").get<std::string>() + " "s
                                   + backend_path.c_str() + " "s
                                   + back_path_json.at("thermal_relaxation").get<std::string>() + " "s
                                   + back_path_json.at("readout_error").get<std::string>() + " "s
                                   + back_path_json.at("gate_error").get<std::string>() + " "s
                                   + family.c_str() + " "s
                                   + back_path_json.at("fakeqmio").get<std::string>());
                                   

    LOGGER_DEBUG("Command: {}", command);
    std::system(("ml load qmio/hpc gcc/12.3.0 qiskit/1.2.4-python-3.9.9 2> /dev/null\n"s + command).c_str());
    try {
        // Try to open the generated noisy backend file to check if it exists and is readable
        std::ifstream infile(std::getenv("STORE") + std::string("/.cunqa/tmp_noisy_backend_") + std::getenv("SLURM_JOB_ID") + ".json");
        if (!infile.good()) {
            throw std::runtime_error("Failed to open noise model JSON file.");
        }
    } catch (const std::exception& e) {
        LOGGER_ERROR("Exception in generate_noise_instructions: {}", e.what());
        throw;
    }
    return std::getenv("STORE") + "/.cunqa/tmp_noisy_backend_"s + std::getenv("SLURM_JOB_ID") + ".json"s;
}

int main(int argc, char *argv[])
{
    std::string info_path(argv[1]);
    std::string mode(argv[2]);
    std::string communications(argv[3]);
    std::string family(argv[4]);
    std::string sim_arg(argv[5]);

    if (family == "default")
        family = std::getenv("SLURM_JOB_ID");

    auto back_path_json = (argc == 7 ? JSON::parse(std::string(argv[6]))
                                     : JSON());

    JSON backend_json;
    if (back_path_json.contains("noise_properties_path")) {
        std::ifstream f(generate_noise_instructions(back_path_json, family));
        backend_json = JSON::parse(f);
    } else if (back_path_json.contains("backend_path")) {
        std::ifstream f(back_path_json.at("backend_path").get<std::string>());
        backend_json = JSON::parse(f);
    } else {
        LOGGER_DEBUG("No backend_path nor noise_properties_path were provided.");
    }


    switch(murmur::hash(communications)) {
        case murmur::hash("no_comm"): 
            LOGGER_DEBUG("Raising QPU without communications.");
            switch(murmur::hash(sim_arg)) {
                case murmur::hash("Aer"): 
                    LOGGER_DEBUG("QPU going to turn on with AerSimpleSimulator.");
                    turn_ON_QPU<AerSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Munich"):
                    LOGGER_DEBUG("QPU going to turn on with MunichSimpleSimulator.");
                    turn_ON_QPU<MunichSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Cunqa"):
                    LOGGER_DEBUG("QPU going to turn on with CunqaSimpleSimulator.");
                    turn_ON_QPU<CunqaSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, family);
                    break;
                default:
                    LOGGER_ERROR("Simulator {} do not support simple simulation or does not exist.", sim_arg);
                    return EXIT_FAILURE;
            }
            break;
        case murmur::hash("cc"): 
            LOGGER_DEBUG("Raising QPU with classical communications.");
            switch(murmur::hash(sim_arg)) {
                case murmur::hash("Aer"): 
                    LOGGER_DEBUG("QPU going to turn on with AerCCSimulator.");
                    turn_ON_QPU<AerCCSimulator, CCConfig, CCBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Munich"): 
                    LOGGER_DEBUG("QPU going to turn on with MunichCCSimulator.");
                    turn_ON_QPU<MunichCCSimulator, CCConfig, CCBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Cunqa"): 
                    LOGGER_DEBUG("QPU going to turn on with CunqaCCSimulator.");
                    turn_ON_QPU<CunqaCCSimulator, CCConfig, CCBackend>(backend_json, mode, family);
                    break;
                default:
                    LOGGER_ERROR("Simulator {} do not support classical communication simulation or does not exist.", sim_arg);
                    return EXIT_FAILURE;
            }
            break;
        case murmur::hash("qc"):
            LOGGER_DEBUG("Raising QPU with quantum communications.");
            switch(murmur::hash(sim_arg)) {
                case murmur::hash("Aer"): 
                    LOGGER_DEBUG("QPU going to turn on with AerQCSimulator.");
                    turn_ON_QPU<AerQCSimulator, QCConfig, QCBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Munich"): 
                    LOGGER_DEBUG("QPU going to turn on with MunichQCSimulator.");
                    turn_ON_QPU<MunichQCSimulator, QCConfig, QCBackend>(backend_json, mode, family);
                    break;
                case murmur::hash("Cunqa"): 
                    LOGGER_DEBUG("QPU going to turn on with CunqaQCSimulator.");
                    turn_ON_QPU<AerQCSimulator, QCConfig, QCBackend>(backend_json, mode, family);
                    break;
                default:
                    LOGGER_ERROR("Simulator {} do not support quantum communication simulation or does not exist.", sim_arg);
                    return EXIT_FAILURE;
            }
            break;
        default:
            LOGGER_ERROR("No {} communication method available.", communications);
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}