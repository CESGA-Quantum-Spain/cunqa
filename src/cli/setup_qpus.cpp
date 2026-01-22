
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

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
#include "backends/simulators/Qulacs/qulacs_simple_simulator.hpp"
#include "backends/simulators/Qulacs/qulacs_cc_simulator.hpp"
#include "backends/simulators/Qulacs/qulacs_qc_simulator.hpp"

#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"
#include "logger.hpp"

using namespace std::string_literals;
using namespace cunqa;
using namespace cunqa::sim;

template<typename Simulator, typename Config, typename BackendType>
void turn_ON_QPU(
    const JSON& backend_json, const std::string& mode, 
    const std::string& name, const std::string& family
)
{
    std::unique_ptr<Simulator> simulator = std::make_unique<Simulator>();
    Config config;
    if (!backend_json.empty())
        config = backend_json;
    QPU qpu(std::make_unique<BackendType>(config, std::move(simulator)), mode, name, family);
    qpu.turn_ON();
}

int main(int argc, char *argv[])
{
    std::string mode(argv[1]);
    std::string communications(argv[2]);
    std::string family(argv[3]);
    std::string sim_arg(argv[4]);

    if (family == "default")
        family = std::getenv("SLURM_JOB_ID");
    std::string name = std::getenv("SLURM_JOB_ID") + "_"s 
                     + std::getenv("SLURM_TASK_PID");
    
    auto back_path_json = (argc == 6 ? JSON::parse(std::string(argv[5])) : JSON());
    JSON backend_json;
    if (back_path_json.contains("backend_path")) {
        std::ifstream f(back_path_json.at("backend_path").get<std::string>());
        backend_json = JSON::parse(f);
    }

    switch(murmur::hash(communications)) {
        case murmur::hash("no_comm"): 
            LOGGER_DEBUG("Raising QPU without communications.");
            switch(murmur::hash(sim_arg)) {
                case murmur::hash("Aer"): 
                    LOGGER_DEBUG("QPU going to turn on with AerSimpleSimulator.");
                    turn_ON_QPU<AerSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, 
                                                                                 name, family);
                    break;
                case murmur::hash("Munich"):
                    LOGGER_DEBUG("QPU going to turn on with MunichSimpleSimulator.");
                    turn_ON_QPU<MunichSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Cunqa"):
                    LOGGER_DEBUG("QPU going to turn on with CunqaSimpleSimulator.");
                    turn_ON_QPU<CunqaSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Qulacs"):
                    LOGGER_DEBUG("QPU going to turn on with QulacsSimpleSimulator.");
                    turn_ON_QPU<QulacsSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
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
                    turn_ON_QPU<AerCCSimulator, CCConfig, CCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Munich"): 
                    LOGGER_DEBUG("QPU going to turn on with MunichCCSimulator.");
                    turn_ON_QPU<MunichCCSimulator, CCConfig, CCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Cunqa"): 
                    LOGGER_DEBUG("QPU going to turn on with CunqaCCSimulator.");
                    turn_ON_QPU<CunqaCCSimulator, CCConfig, CCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Qulacs"): 
                    LOGGER_DEBUG("QPU going to turn on with QulacsCCSimulator.");
                    turn_ON_QPU<QulacsCCSimulator, CCConfig, CCBackend>(backend_json, mode, name, family);
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
                    turn_ON_QPU<AerQCSimulator, QCConfig, QCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Munich"): 
                    LOGGER_DEBUG("QPU going to turn on with MunichQCSimulator.");
                    turn_ON_QPU<MunichQCSimulator, QCConfig, QCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Cunqa"): 
                    LOGGER_DEBUG("QPU going to turn on with CunqaQCSimulator.");
                    turn_ON_QPU<CunqaQCSimulator, QCConfig, QCBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Qulacs"): 
                    LOGGER_DEBUG("QPU going to turn on with QulacsQCSimulator.");
                    turn_ON_QPU<QulacsQCSimulator, QCConfig, QCBackend>(backend_json, mode, name, family);
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