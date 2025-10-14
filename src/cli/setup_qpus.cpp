
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

#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"
#include "logger.hpp"

using namespace std::string_literals;

using namespace cunqa;
using namespace cunqa::sim;

namespace {

std::string transform_noise_properties(const JSON& qubits_json, const std::string& output_path)
{
    JSON output_json;
    if (qubits_json.contains("single_qubits")) {
        output_json["Qubits"] = {};
        output_json["Q1Gates"] = {};
        output_json["Q2Gates(RB)"] = {};
        int i = 0;
        for (auto it = qubits_json["single_qubits"].begin(); it != qubits_json["single_qubits"].end(); ++it) {
            const auto& qubit_info = it.value();
            output_json["Qubits"]["q[" + std::to_string(i) + "]"] = {{"T1 (s)", qubit_info["t1"]},
                                                                        {"T2 (s)", qubit_info["t2"]},
                                                                        {"Drive Frequency (Hz)", qubit_info["drive_frequency"]},
                                                                        {"Measuring frequency (Hz)", qubit_info["measuring_frequency"]},
                                                                        {"Readout duration (s)", qubit_info["readout_duration"]},
                                                                        {"Readout fidelity (RB)", qubit_info["readout_fidelity"]}
                                                                    };
            output_json["Q1Gates"][it.key()] = {};
            for (auto gate_it = qubit_info["gates"].begin(); gate_it != qubit_info["gates"].end(); ++gate_it) {
                const std::string& gate_name = gate_it.key();
                const auto& gate_info = gate_it.value();
                output_json["Q1Gates"]["q[" + std::to_string(i) + "]"][gate_name] = {{"Gate duration (s)", gate_info["gate_duration"]},
                                                                                        {"Fidelity(RB)", gate_info["fidelity"]}
                                                                                    };
            }
            i++;
        }

        for (auto it = qubits_json["pared_qubits"].begin(); it != qubits_json["pared_qubits"].end(); ++it) {
            const auto& qubit_info = it.value();
            const std::string& key = it.key();
            int n, m;
            sscanf(key.c_str(), "q%d-q%d>", &n, &m);
            std::string new_key = std::to_string(n) + "-" + std::to_string(m);

            auto& q2gates = output_json["Q2Gates(RB)"][new_key];
            for (const auto& gate_it : qubit_info["gates"].items()) {
                const std::string& gate_name = gate_it.key();
                const auto& gate_info = gate_it.value();
                q2gates[gate_name] = {
                    {"Duration (s)", gate_info["gate_duration"]},
                    {"Fidelity(RB)", gate_info["fidelity"]}
                };
            } 
        }
    }
    std::ofstream outfile(output_path);
    if (!outfile.is_open()) {
        LOGGER_ERROR("Cannot open output file: {}", output_path);
        throw std::runtime_error("Output file open failed");
    }
    outfile << output_json.dump(4);
    outfile.close();
    return output_path;
}


JSON convert_to_backend(const JSON& backend_paths)
{
    JSON qpu_properties;
    if (backend_paths.size() == 1) {
        auto it = backend_paths.begin();
        std::string path = it.value().get<std::string>();
        std::ifstream f(path); // try-catch?
        qpu_properties = JSON::parse(f);
    } else if (backend_paths.size() > 1) {
        std::string str_local_id = std::getenv("SLURM_PROCID");
        int local_id = std::stoi(str_local_id);
        auto qpu = backend_paths.begin();
        std::advance(qpu, local_id);
        std::string path = qpu.value().get<std::string>();
        std::ifstream f(path); // try-catch?
        qpu_properties = JSON::parse(f);
    } else {
        LOGGER_ERROR("No backends provided");
        throw;
    }

    if ((qpu_properties.contains("single_qubits") && !qpu_properties.at("single_qubits").empty()) ||
        (qpu_properties.contains("paired_qubits") && !qpu_properties.at("paired_qubits").empty())) {
        std::string noise_properties_path = std::getenv("STORE") + std::string("/.cunqa/tmp_noise_properties_") + std::getenv("SLURM_JOB_ID") + ".json";
        transform_noise_properties(qpu_properties, noise_properties_path);

    } else {
        std::string noise_properties_path = ""
    }

    JSON backend;
    backend = {
        {"name", qpu_properties.at("name")}, 
        {"version", ""},
        {"description", "QPU from infrastructure"},
        {"n_qubits", qpu_properties.at("n_qubits")},
        {"coupling_map", qpu_properties.at("coupling_map")},
        {"basis_gates", qpu_properties.at("basis_gates")}, 
        {"custom_instructions", ""}, // What's this?
        {"gates", JSON::array()}, // gates vs basis_gates?
        {"noise_model", JSON()},
        {"noise_properties", noise_properties_path},
        {"noise_path", ""}
    };
    
    return backend;
}

std::string get_qpu_name(const JSON& backend_paths) 
{
    std::string qpu_name;
    if (backend_paths.size() == 1) {
        qpu_name = backend_paths.begin().key();
    } else {
        std::string str_local_id = std::getenv("SLURM_PROCID");
        int local_id = std::stoi(str_local_id);
        auto qpu = backend_paths.begin();
        std::advance(qpu, local_id);
        qpu_name = qpu.key();
    } 

    return qpu_name;
}

template<typename Simulator, typename Config, typename BackendType>
void turn_ON_QPU(const JSON& backend_json, const std::string& mode, const std::string& name, const std::string& family)
{
    std::unique_ptr<Simulator> simulator = std::make_unique<Simulator>(family);
    LOGGER_DEBUG("Simulator instantiated");
    Config config;
    if (!backend_json.empty())
        config = backend_json;
    QPU qpu(std::make_unique<BackendType>(config, std::move(simulator)), mode, name, family);
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

    if (SYSTEM_NAME == "QMIO") {
        std::system(("ml load qmio/hpc gcc/12.3.0 qiskit/1.2.4-python-3.11.9 2> /dev/null\n"s + command).c_str());
    } else if (SYSTEM_NAME == "FT3") {
        std::system(("ml load cesga/2022 gcc/system qiskit/1.2.4 2> /dev/null\n"s + command).c_str());
    } else if (SYSTEM_NAME == "MY_CLUSTER") {
        std::system(("ml load NEEDED_MODULES 2> /dev/null\n"s + command).c_str());
    } else {
        LOGGER_ERROR("SYSTEM_NAME MACRO is not defined.");
        throw;
    }

    return "";
}

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

    if (back_path_json.contains("backend_from_infrastructure")) {
         auto backend_paths = back_path_json.at("backend_from_infrastructure").get<JSON>();
        convert_to_backend(backend_paths);

    }

    // back_path_json = {"informacion del backend y las noise_properties!!!!"} ---> si recivo algún tipo de indicio de ruido traduzco a lo
    // que me deberia haber dado el get_noise_model_run_command


    JSON backend_json;
    std::string name = family + "_" + std::getenv("SLURM_PROCID");
    if (back_path_json.contains("noise_properties_path")) {
        std::string fpath = std::getenv("STORE") + std::string("/.cunqa/tmp_noisy_backend_") + std::getenv("SLURM_JOB_ID") + ".json";

        if (std::getenv("SLURM_PROCID") && std::string(std::getenv("SLURM_PROCID")) == "0") {
            generate_noise_instructions(back_path_json, family);
            LOGGER_DEBUG("Correctly created tmp noise intructions file.");
        } else {
            int fd = open(fpath.c_str(), O_RDONLY);
            while (fd == -1 || flock(fd, LOCK_SH) != 0) {
                if (fd != -1) close(fd);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                fd = open(fpath.c_str(), O_RDONLY);
            }
            close(fd);
        }

        std::ifstream f(fpath);
        backend_json = JSON::parse(f);
    } else if (back_path_json.contains("backend_path")) {
        std::ifstream f(back_path_json.at("backend_path").get<std::string>());
        backend_json = JSON::parse(f);
    } else if (back_path_json.contains("backend_from_infrastructure")) {
        auto backend_paths = back_path_json.at("backend_from_infrastructure").get<JSON>();
        backend_json = convert_to_backend(backend_paths);
        name = get_qpu_name(backend_paths);
    } else {    
        LOGGER_DEBUG("No backend_path nor noise_properties_path were provided.");
    }


    switch(murmur::hash(communications)) {
        case murmur::hash("no_comm"): 
            LOGGER_DEBUG("Raising QPU without communications.");
            switch(murmur::hash(sim_arg)) {
                case murmur::hash("Aer"): 
                    LOGGER_DEBUG("QPU going to turn on with AerSimpleSimulator.");
                    turn_ON_QPU<AerSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Munich"):
                    LOGGER_DEBUG("QPU going to turn on with MunichSimpleSimulator.");
                    turn_ON_QPU<MunichSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
                    break;
                case murmur::hash("Cunqa"):
                    LOGGER_DEBUG("QPU going to turn on with CunqaSimpleSimulator.");
                    turn_ON_QPU<CunqaSimpleSimulator, SimpleConfig, SimpleBackend>(backend_json, mode, name, family);
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