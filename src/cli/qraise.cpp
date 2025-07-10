#include <string>
#include <fstream>
#include <regex>
#include <any>

#include <iostream>
#include <cstdlib>

#include "argparse.hpp"

#include "utils/constants.hpp"
#include "utils/qraise/utils_qraise.hpp"
#include "utils/qraise/args_qraise.hpp"
#include "utils/qraise/fakeqmio_conf_qraise.hpp"
#include "utils/qraise/no_comm_conf_qraise.hpp"
#include "utils/qraise/class_comm_conf_qraise.hpp"
#include "utils/qraise/quantum_comm_conf_qraise.hpp"

#include "logger.hpp"

namespace {
    const int CORES_PER_NODE = 64; // For both QMIO and FT3
}

using namespace std::literals;

int main(int argc, char* argv[]) 
{
    srand((unsigned int)time(NULL));
    int intSEED = rand() % 1000;
    std::string SEED = std::to_string(intSEED);

    auto args = argparse::parse<MyArgs>(argc, argv, true); //true ensures an error is raised if we feed qraise an unrecognized flag

    const char* store = std::getenv("STORE");
    std::string info_path = std::string(store) + "/.cunqa/qpus.json";

    std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");
    LOGGER_DEBUG("Temporal file qraise_sbatch_tmp.sbatch created.");
    std::string run_command;

    // Escribir el contenido del script SBATCH
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";
    sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";
    sbatchFile << "#SBATCH --ntasks=" << args.n_qpus << "\n";
    int n_nodes = number_of_nodes(args.n_qpus, args.cores_per_qpu, args.number_of_nodes.value(), CORES_PER_NODE);
    sbatchFile << "#SBATCH -N " << n_nodes << "\n";
    

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

    if (check_mem_format(args.mem_per_qpu)){
        int mem_per_qpu = args.mem_per_qpu;
        int cores_per_qpu = args.cores_per_qpu;
        sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_qpu/cores_per_qpu << "G\n";
    } else {
        LOGGER_ERROR("Memory format is incorrect, must be: xG (where x is the number of Gigabytes).");
        return -1;
    }

    if (check_time_format(args.time))
        sbatchFile << "#SBATCH --time=" << args.time << "\n";
    else {
        LOGGER_ERROR("Time format is incorrect, must be: xx:xx:xx.");
        return -1;
    }

    if (!check_simulator_name(args.simulator)){
        LOGGER_ERROR("Incorrect simulator name ({}).", args.simulator);
        return -1;
    }

    int memory_specs = check_memory_specs(args.mem_per_qpu, args.cores_per_qpu);

    if (memory_specs == 1) {
        LOGGER_ERROR("Too much memory per QPU in QMIO. Please, decrease the mem-per-qpu or increase the cores-per-qpu. (Max mem-per-cpu = 16)");
        return -1;
    } else if (memory_specs == 2) {
        LOGGER_ERROR("Too much memory per QPU in FT3. Please, decrease the mem-per-qpu or increase the cores-per-qpu. Max mem-per-cpu = 4");
        return -1;
    }

    sbatchFile << "#SBATCH --output=qraise_%j\n";

    sbatchFile << "\n";
    sbatchFile << "if [ ! -d \"$STORE/.cunqa\" ]; then\n";
    sbatchFile << "mkdir $STORE/.cunqa\n";
    sbatchFile << "fi\n";

    sbatchFile << "BINARIES_DIR=" << std::getenv("STORE") << "/.cunqa\n";
    sbatchFile << "export INFO_PATH=" << info_path + "\n";

    //Checking duplicate family name
    std::string family = std::any_cast<std::string>(args.family);
    if (exists_family_name(family, info_path)) { //Check if there exists other QPUs with same family name
        std::system("rm qraise_sbatch_tmp.sbatch");
        std::cerr << "\033[1;31m" << "Error: There are QPUs with the same family name as the one provided: " << family << "\033[0m" << std::endl;
        return -1;
        /* try{
            throw std::runtime_error("There are QPUs with the same family name");
        } catch (const std::exception& e) {
            std::cerr << "\033[1;31m" << "Error: " << e.what() << " as the one provided: " << family << "\033[0m" << std::endl;
            return -1;  
        } */
    }

    //Get mode: hpc or cloud
    std::string mode;
    if(args.cloud) {
        mode = "cloud";
    } else {
        mode = "hpc";
    }

    //Get srun command
    if (args.fakeqmio.has_value()) {
        LOGGER_DEBUG("Fakeqmio provided as a FLAG");
        run_command = get_fakeqmio_run_command(args, mode);
    } else {
        if (args.classical_comm) {
            LOGGER_DEBUG("Classical communications");
            LOGGER_ERROR("Classical communications are not implemented yet");
            std::system("rm qraise_sbatch_tmp.sbatch");
            return 0;
        } else if (args.quantum_comm) {
            LOGGER_ERROR("Quantum communications are not implemented yet");
            std::system("rm qraise_sbatch_tmp.sbatch");
            return 0;
        } else {
            LOGGER_DEBUG("No communications");
            run_command = get_no_comm_run_command(args, mode);
            if (run_command == "0") {
                return 0;
            }
        }
    }

    LOGGER_DEBUG("Run command: ", run_command);
    sbatchFile << run_command;

    sbatchFile.close();

    std::system("sbatch qraise_sbatch_tmp.sbatch");
    std::system("rm qraise_sbatch_tmp.sbatch");

    LOGGER_DEBUG("Sbatch launched and qraise_sbatch_tmp.sbatch removed.");

    return 0;
}