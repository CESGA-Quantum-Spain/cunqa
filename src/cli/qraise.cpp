#include <string>
#include <fstream>
#include <regex>
#include <any>
#include <nlohmann/json.hpp>
#include <iostream>
#include <cstdlib>

#include "argparse.hpp"
#include "logger/logger.hpp"
#include "constants.hpp"

using json = nlohmann::json;


using namespace std::literals;

struct MyArgs : public argparse::Args 
{
    int& n_qpus                          = kwarg("n,num_qpus", "Number of QPUs to be raised.");
    std::string& time                    = kwarg("t,time", "Time for the QPUs to be raised.");
    int& cores_per_qpu                   = kwarg("c,cores", "Number of cores per QPU.").set_default(2);
    int& mem_per_qpu                     = kwarg("mem,mem-per-qpu", "Memory given to each QPU in GB.").set_default(24);
    std::optional<int>& number_of_nodes  = kwarg("N,n_nodes", "Number of nodes.").set_default(1);
    std::optional<int>& qpus_per_node    = kwarg("qpuN,qpus_per_node", "Number of qpus in each node.");
    std::optional<std::string>& backend  = kwarg("b,backend", "Path to the backend config file.");
    std::string& simulator               = kwarg("sim,simulator", "Simulator reponsible of running the simulations.").set_default("Aer");
    std::optional<std::string>& fakeqmio = kwarg("fq,fakeqmio", "Raise FakeQmio backend from calibration file", /*implicit*/"last_calibrations");
    std::string& family_name             = kwarg("fam,family_name", "Name that identifies which QPUs were raised together").set_default("default");
    std::optional<std::vector<std::string>>& node_list = kwarg("node_list", "List of nodes where the QPUs will be deployed.").multi_argument(); 
    std::string& mode                    = kwarg("mode", "Infraestructure mode: HPC or CLOUD").set_default("hpc");

    void welcome() {
        std::cout << "Welcome to qraise command, a command responsible for turn on the required QPUs.\n" << std::endl;
    }
};

bool check_time_format(const std::string& time)
{
    std::regex format("^(\\d{2}):(\\d{2}):(\\d{2})$");
    return std::regex_match(time, format);   
}

bool check_mem_format(const int& mem) 
{
    std::string mem_str = std::to_string(mem) + "G";
    std::regex format("^(\\d{1,2})G$");
    return std::regex_match(mem_str, format);
}

int check_memory_specs(int& mem_per_qpu, int& cores_per_qpu)
{
    int mem_per_cpu = mem_per_qpu/cores_per_qpu;
    const char* system_var = std::getenv("LMOD_SYSTEM_NAME");
    if ((std::string(system_var) == "QMIO" && mem_per_cpu > 15)) {
        return 1;
    } else if ((std::string(system_var) == "FT3" && mem_per_cpu > 4)){
        return 2;
    }

    SPDLOG_LOGGER_DEBUG(logger, "Correct memory per core.");

    return 0;
}



int main(int argc, char* argv[]) 
{
    srand((unsigned int)time(NULL));
    int intSEED = rand() % 1000;
    std::string SEED = std::to_string(intSEED);

    auto args = argparse::parse<MyArgs>(argc, argv, true); //true ensures an error is raised if we feed qraise an unrecognized flag

    //Checking mode:
    if ((args.mode != "hpc") && (args.mode != "cloud")) {
        std::cerr << "\033[1;31m" << "Error: " << "mode argument different than hpc or cloud. Given: " << args.mode << "\n";
        std::cerr << "Aborted." << "\033[0m \n";
        return 1;
    }

    std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");
    SPDLOG_LOGGER_DEBUG(logger, "Temporal file qraise_sbatch_tmp.sbatch created.");

    // Escribir el contenido del script SBATCH
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";
    sbatchFile << "#SBATCH --ntasks=" << args.n_qpus << "\n";
    sbatchFile << "#SBATCH -N " << args.number_of_nodes.value() << "\n";

    if (args.qpus_per_node.has_value()) {
        if (args.n_qpus < args.qpus_per_node) {
            std::cerr << "\033[1;31m" << "Error: " << "Less qpus than selected qpus_per_node.\n";
            std::cerr << "Number of QPUs: " << args.n_qpus << " QPUs per node: " << args.qpus_per_node.value() << "\n";
            std::cerr << "Aborted." << "\033[0m \n";
            std::system("rm qraise_sbatch_tmp.sbatch");
            return 1;
        } else {
            sbatchFile << "#SBATCH --ntasks-per-node=" << args.qpus_per_node.value() << "\n";
        }
    }

    if (args.node_list.has_value()) {
        if (args.number_of_nodes.value() != args.node_list.value().size()) {
            std::cerr << "\033[1;31m" << "Error: " << "Different number of node names than total nodes.\n";
            std::cerr << "Number of nodes: " << args.number_of_nodes.value() << " Number of node names: " << args.node_list.value().size() << "\n";
            std::cerr << "Aborted." << "\033[0m \n";
            std::system("rm qraise_sbatch_tmp.sbatch");
            return 1;
        } else {
            sbatchFile << "#SBATCH --nodelist=";
            int comma = 0;
            for (auto& node_name : args.node_list.value()) {
                if (comma > 0 ) {
                    sbatchFile << ",";
                }
                sbatchFile << node_name;
                comma++;
            }
            sbatchFile << "\n";
        }
    }
    //  

    if (check_mem_format(args.mem_per_qpu)){
        int mem_per_qpu = args.mem_per_qpu;
        int cores_per_qpu = args.cores_per_qpu;
        sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_qpu/cores_per_qpu << "G\n";
    } else {
        SPDLOG_LOGGER_DEBUG(logger, "Memory format is incorrect, must be: xG (where x is the number of Gigabytes).");
        return -1;
    }

    if (check_time_format(args.time))
        sbatchFile << "#SBATCH --time=" << args.time << "\n";
    else {
        SPDLOG_LOGGER_DEBUG(logger, "Time format is incorrect, must be: xx:xx:xx.");
        return -1;
    }

    int memory_specs = check_memory_specs(args.mem_per_qpu, args.cores_per_qpu);

    if (memory_specs == 1) {
        SPDLOG_LOGGER_ERROR(logger, "Too much memory per QPU in QMIO. Please, decrease the mem-per-QPU or increase the cores-per-qpu. (Max mem-per-cpu = 16)");
        return -1;
    } else if (memory_specs == 2) {
        SPDLOG_LOGGER_ERROR(logger, "Too much memory per QPU in FT3. Please, decrease the mem-per-QPU or increase the cores-per-qpu. Max mem-per-cpu = 4");
        return -1;
    }

    sbatchFile << "#SBATCH --output=qraise_%j\n";

    sbatchFile << "\n";
    sbatchFile << "if [ ! -d \"$STORE/.cunqa\" ]; then\n";
    sbatchFile << "mkdir $STORE/.cunqa\n";
    sbatchFile << "fi\n";

    const char* var_name = "INSTALL_PATH"; // Replace with your variable name
    const char* var_install_path = std::getenv(var_name);

    if (var_install_path) {
        sbatchFile << "BINARIES_DIR=" << var_install_path << "/bin\n";
    } else {
        std::cerr << "Environment variable INSTALL_PATH is not set: aborting.\n"; 
    }

    sbatchFile << "export INFO_PATH=" << std::getenv("STORE") << "/.cunqa/qpus.json\n";

    std::string backend_path;
    std::string backend;
    if (args.fakeqmio.has_value()) {
        backend_path = std::any_cast<std::string>(args.fakeqmio.value());
        backend = R"({"fakeqmio_path":")" + backend_path + R"("})" ;
        sbatchFile << "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus " << args.mode.c_str() << " " << args.family_name.c_str() << " $INFO_PATH " << args.simulator.c_str() << " " << "\'"<< backend << "\'" << "\n";  
        SPDLOG_LOGGER_DEBUG(logger, "FakeQmio. Command: srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus {} {} $INFO_PATH {} {}\n", args.mode.c_str(), args.family_name.c_str(), args.simulator.c_str(), backend);
    }
    
    if (args.backend.has_value()) {
        if(args.backend.value() == "etiopia_computer.json") {
            SPDLOG_LOGGER_ERROR(logger, "Terrible mistake. Possible solution: {}", cafe);
            std::system("rm qraise_sbatch_tmp.sbatch");
            return 0;
        } else {
            backend_path = std::any_cast<std::string>(args.backend.value());
            backend = R"({"backend_path":")" + backend_path + R"("})" ;
            sbatchFile << "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus " << args.mode.c_str() << " " << args.family_name.c_str() << " $INFO_PATH " << args.simulator.c_str() << " " << "\'"<< backend << "\'" << "\n";
            SPDLOG_LOGGER_DEBUG(logger, "Qraise with backend. Command: srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus {} {} $INFO_PATH {} {}\n", args.mode.c_str(), args.family_name.c_str(), args.simulator.c_str(), backend);
        }
    } else {
        sbatchFile << "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus " << args.mode.c_str() << " " << args.family_name.c_str() << " $INFO_PATH " << args.simulator.c_str() << "\n";
        SPDLOG_LOGGER_DEBUG(logger, "Command: srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus {} {} $INFO_PATH {} \n", args.mode.c_str(), args.family_name.c_str(), args.simulator.c_str());
    }

    sbatchFile.close();

    std::system("sbatch qraise_sbatch_tmp.sbatch");
    std::system("rm qraise_sbatch_tmp.sbatch");

    SPDLOG_LOGGER_DEBUG(logger, "Sbatch launched and qraise_sbatch_tmp.sbatch removed.");

    return 0;
}