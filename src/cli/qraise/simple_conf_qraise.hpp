#pragma once

#include <string>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"


namespace{
using namespace cunqa;


bool write_simple_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#SBATCH --ntasks=" << std::to_string(args.n_qpus) << "\n";
    sbatchFile << "#SBATCH -c " << std::to_string(args.cores_per_qpu) << "\n";
    sbatchFile << "#SBATCH -N " << std::to_string(args.number_of_nodes.value()) << "\n";
    
    if(args.partition.has_value())
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    
    if (args.qpus_per_node.has_value()) {
            sbatchFile << "#SBATCH --ntasks-per-node=" << std::to_string(args.qpus_per_node.value()) << "\n";
    }
    
    if (args.node_list.has_value()) {
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

    if (args.mem_per_qpu.has_value()) {
        int mem_per_core = (args.mem_per_qpu.value()/args.cores_per_qpu != 0) ? args.mem_per_qpu.value()/args.cores_per_qpu : 1;
        sbatchFile << "#SBATCH --mem-per-cpu=" << std::to_string(mem_per_core) << "G\n";
    }
    
    return true;
}

bool write_simple_gpu_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
{
#if !COMPILATION_FOR_GPU
    LOGGER_ERROR("CUNQA was not compiled with GPU support.");
    return false;
#else

    std::vector<std::string> simulators_with_gpu_support = {"Aer"};
    if (std::find(simulators_with_gpu_support.begin(), simulators_with_gpu_support.end(), std::string(args.simulator)) == simulators_with_gpu_support.end()) {
        LOGGER_ERROR("At this moment, only Aer supports GPU simulation");
        return false;
    }

    sbatchFile << "#SBATCH --ntasks=" << std::to_string(args.n_qpus) << "\n";

#if GPU_ARCH == 75
    sbatchFile << "#SBATCH --gres=gpu:t4\n";
    if(args.partition.has_value()) {
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    } else {
        sbatchFile << "#SBATCH -p viz\n";
    }    
#elif GPU_ARCH == 80
    sbatchFile << "#SBATCH --gres=gpu:a100:" << std::to_string(args.n_qpus) << "\n";
    if(args.partition.has_value()) {
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    }    
#endif // GPU_ARCH

    sbatchFile << "#SBATCH -c " << std::to_string(args.cores_per_qpu) << "\n";
    if (args.mem_per_qpu.has_value()) {
        int mem_per_core = (args.mem_per_qpu.value()/args.cores_per_qpu != 0) ? args.mem_per_qpu.value()/args.cores_per_qpu : 1;
        int total_mem = args.n_qpus * args.cores_per_qpu * mem_per_core;
        sbatchFile << "#SBATCH --mem=" << std::to_string(total_mem) << "G\n";
    }
#endif //COMPILATION_FOR_GPU

    return true;
}

bool write_simple_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    bool success_writing_resources;
    if (args.gpu) {
        if(!write_simple_gpu_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_simple_gpu_resources failed");
            return false;
        }
    } else {
        if (!write_simple_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_simple_resources failed");
            return false;
        }
    }

    sbatchFile << "#SBATCH --time=" << args.time << "\n";

    //sbatchFile << "#SBATCH --profile=all\n";   // Enable comprehensive profiling
    sbatchFile << "#SBATCH --output=qraise_%j\n\n";
    sbatchFile << "unset SLURM_MEM_PER_CPU SLURM_CPU_BIND_LIST SLURM_CPU_BIND\n";
    sbatchFile << "EPILOG_PATH=" << std::string(constants::CUNQA_PATH) << "/epilog.sh\n";

    return true;
}

bool write_simple_run_command(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;
    std::string mode = args.co_located ? "co_located" : "hpc";

    if (args.backend.has_value()) {
        LOGGER_DEBUG("Backend provided.");
        if(args.backend.value() == "etiopia_computer.json") {
            LOGGER_ERROR("Terrible mistake. Possible solution: {}", cafe);
            return false;
        } else {
            LOGGER_DEBUG("Qraise with no communications and personalized backend. \n");
            backend_path = std::string(args.backend.value());
            backend = R"({"backend_path":")" + backend_path + R"("})" ;
            subcommand = mode + " no_comm " + args.family_name + " " + args.simulator + " \'" + backend + "\'" "\n";
            run_command = "srun --task-epilog=$EPILOG_PATH setup_qpus " + subcommand;
        }
    } else {
        subcommand = mode + " no_comm " + args.family_name + " " + args.simulator + "\n";
        run_command = "srun --task-epilog=$EPILOG_PATH setup_qpus " + subcommand;
    }

    sbatchFile << run_command;

    return true;
}

void write_simple_sbatch(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    if (args.n_qpus == 0 || args.time == "") {
        LOGGER_ERROR("qraise needs two mandatory arguments:\n \t -n: number of vQPUs to be raised\n\t -t: maximum time vQPUs will be raised (hh:mm:ss)\n");
        throw std::runtime_error("Bad arguments.");

    } else if (std::find(constants::SUPPORTED_SIMPLE_SIMULATORS.begin(), constants::SUPPORTED_SIMPLE_SIMULATORS.end(), std::string(args.simulator)) == constants::SUPPORTED_SIMPLE_SIMULATORS.end()) {
        LOGGER_ERROR("Simulator {} is not available for simple simulation. Aborting. ", std::string(args.simulator));
        throw std::runtime_error("Error.");

    } else if (exists_family_name(args.family_name, constants::QPUS_FILEPATH)) {
        LOGGER_ERROR("There are QPUs with the same family name as the provided: {}.", args.family_name.c_str());
        throw std::runtime_error("Bad family name.");
        
    } else if (!write_simple_sbatch_header(sbatchFile, args) || !write_simple_run_command(sbatchFile, args)) {
        LOGGER_ERROR("Error writing simple sbatch file.");
        throw std::runtime_error("Error.");
    }  
}

} // End namespace