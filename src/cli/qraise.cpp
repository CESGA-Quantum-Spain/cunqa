#include <string>
#include <fstream>
#include <regex>
#include <any>
#include <filesystem> //debug

#include <iostream>
#include <cstdlib>

#include "argparse/argparse.hpp"

#include "utils/constants.hpp"
#include "qraise/utils_qraise.hpp"
#include "qraise/args_qraise.hpp"
#include "qraise/noise_model_conf_qraise.hpp"
#include "qraise/simple_conf_qraise.hpp"
#include "qraise/cc_conf_qraise.hpp"
#include "qraise/qc_conf_qraise.hpp"
#include "qraise/infrastructure_conf_qraise.hpp"

#include "logger.hpp"

using namespace std::literals;
using namespace cunqa;

namespace {

void write_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args) 
{
    // Escribir el contenido del script SBATCH
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    int n_tasks = args.qc ? args.n_qpus * args.cores_per_qpu + args.n_qpus : args.n_qpus;    
    sbatchFile << "#SBATCH --ntasks=" << n_tasks << "\n";

    if (!args.qc) 
        sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";

    sbatchFile << "#SBATCH -N " << args.number_of_nodes.value() << "\n";
    
    if(args.partition.has_value())
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";

    if (args.qpus_per_node.has_value()) {
        if (args.n_qpus < args.qpus_per_node) {
            LOGGER_ERROR("Less qpus than selected qpus_per_node.");
            LOGGER_ERROR("\tNumber of QPUs: {}\n\t QPUs per node: {}", args.n_qpus, args.qpus_per_node.value());
            LOGGER_ERROR("Aborted.");
            std::system("rm qraise_sbatch_tmp.sbatch");
            return;
        } else {
            sbatchFile << "#SBATCH --ntasks-per-node=" << args.qpus_per_node.value() << "\n";
        }
    }

    if (args.node_list.has_value()) {
        if (args.number_of_nodes.value() != args.node_list.value().size()) {
            LOGGER_ERROR("Different number of node names than total nodes.");
            LOGGER_ERROR("\tNumber of nodes: {}\n\t Number of node names: {}", args.number_of_nodes.value(), args.node_list.value().size());
            LOGGER_ERROR("Aborted.");
            return;
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

    if (args.mem_per_qpu.has_value() && (args.mem_per_qpu.value()/args.cores_per_qpu > DEFAULT_MEM_PER_CORE)) {
        LOGGER_ERROR("Too much memory per QPU. Please, decrease the mem-per-qpu or increase the cores-per-qpu.");
        return;
    }

    if (!args.qc) {
        if (args.mem_per_qpu.has_value() && check_mem_format(args.mem_per_qpu.value())) {
            sbatchFile << "#SBATCH --mem-per-cpu=" << args.mem_per_qpu.value()/args.cores_per_qpu << "G\n";
        } else if (args.mem_per_qpu.has_value() && !check_mem_format(args.mem_per_qpu.value())) {
            LOGGER_ERROR("Memory format is incorrect, must be: xG (where x is the number of Gigabytes).");
            return;
        } else if (!args.mem_per_qpu.has_value()) {
            int mem_per_core = DEFAULT_MEM_PER_CORE;
            sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_core << "G\n";
        } 
    } else {
        if (args.mem_per_qpu.has_value() && check_mem_format(args.mem_per_qpu.value())) {
            sbatchFile << "#SBATCH --mem=" << args.mem_per_qpu.value() * args.n_qpus + args.n_qpus << "G\n";
        } else {
            int mem_per_core = DEFAULT_MEM_PER_CORE;
            sbatchFile << "#SBATCH --mem=" << mem_per_core * args.cores_per_qpu * args.n_qpus + args.n_qpus << "G\n";
        }
    }


    if (check_time_format(args.time))
        sbatchFile << "#SBATCH --time=" << args.time << "\n";
    else {
        LOGGER_ERROR("Time format is incorrect, must be: xx:xx:xx.");
        return;
    }

    if (!check_simulator_name(args.simulator)){
        LOGGER_ERROR("Incorrect simulator name ({}).", args.simulator);
        return;
    }

    sbatchFile << "#SBATCH --output=qraise_%j\n\n";
    sbatchFile << "unset SLURM_MEM_PER_CPU SLURM_CPU_BIND_LIST SLURM_CPU_BIND\n";
    sbatchFile << "EPILOG_PATH=" << std::string(constants::CUNQA_PATH) << "/epilog.sh\n";
}

void write_run_command(std::ofstream& sbatchFile, const CunqaArgs& args, const std::string& mode)
{
    std::string run_command;
    if (args.noise_properties.has_value() || args.fakeqmio.has_value()){
        LOGGER_DEBUG("noise_properties json path provided");
        if (args.simulator == "Munich" or args.simulator == "Cunqa"){
            LOGGER_WARN("Personalized noise models only supported for AerSimulator, switching simulator setting from {} to Aer.", args.simulator.c_str());
        }
        if (args.cc || args.qc){
            LOGGER_ERROR("Personalized noise models not supported for classical/quantum communications schemes.");
            return;
        }

        if (args.backend.has_value()){
            LOGGER_WARN("Because noise properties were provided backend will be redefined according to them.");
        }

        run_command = get_noise_model_run_command(args, mode);


    } else if ((!args.noise_properties.has_value() || !args.fakeqmio.has_value()) && (args.no_thermal_relaxation || args.no_gate_error || args.no_readout_error)){
        LOGGER_ERROR("noise_properties flags where provided but --noise_properties nor --fakeqmio args were not included.");
        return;

    } else {
        if (args.cc) {
            LOGGER_DEBUG("Classical communications");
            run_command = get_cc_run_command(args, mode);
        } else if (args.qc) {
            LOGGER_DEBUG("Quantum communications");
            run_command = get_qc_run_command(args, mode);
        } else {
            LOGGER_DEBUG("No communications");
            run_command = get_simple_run_command(args, mode);
        }
    }

    LOGGER_DEBUG("Run command: {}", run_command);
    sbatchFile << run_command;
}

}
namespace fs = std::filesystem;

int main(int argc, char* argv[]) 
{
    auto args = argparse::parse<CunqaArgs>(argc, argv, true); //true ensures an error is raised if we feed qraise an unrecognized flag

    if (args.infrastructure.has_value()) {
            LOGGER_DEBUG("Raising infrastructure with path: {}", args.infrastructure.value());
            fs::path current_dir = fs::current_path();
            LOGGER_DEBUG("Current dir: {}", current_dir.string());

            std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");
            write_sbatch_file_from_infrastructure(sbatchFile, args);
            sbatchFile.close();
    } else {
        // Setting and checking mode and family name, respectively
        std::string mode = args.co_located ? "co_located" : "hpc";
        std::string family = args.family_name;
        if (exists_family_name(family, constants::QPUS_FILEPATH)) { //Check if there exists other QPUs with same family name
            LOGGER_ERROR("There are QPUs with the same family name as the provided: {}.", family);
            std::system("rm qraise_sbatch_tmp.sbatch");
            return -1;
        }

        // Writing the sbatch file
        std::ofstream sbatchFile("qraise_sbatch_tmp.sbatch");
        write_sbatch_header(sbatchFile, args);
        write_run_command(sbatchFile, args, mode);
        sbatchFile.close();

    }

    // Executing and deleting the file
    std::system("sbatch qraise_sbatch_tmp.sbatch");
    //std::system("rm qraise_sbatch_tmp.sbatch");
    
    
    return 0;
}