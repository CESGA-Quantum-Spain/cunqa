#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"


namespace{
using namespace cunqa;


bool write_qc_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#SBATCH --ntasks=" << std::to_string(args.n_qpus + 1) << "\n";
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
        int total_mem = args.mem_per_qpu.value() * args.n_qpus + args.n_qpus;
        sbatchFile << "#SBATCH --mem=" << std::to_string(total_mem) << "G\n";
    }


    return true;
}

bool write_qc_gpu_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
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

    sbatchFile << "#SBATCH --ntasks=" << std::to_string(args.n_qpus + 1) << "\n";

#if GPU_ARCH == 75
    sbatchFile << "#SBATCH --gres=gpu:t4\n";
    if(args.partition.has_value()) {
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    } else {
        sbatchFile << "#SBATCH -p viz\n";
    }
#elif GPU_ARCH == 80
    sbatchFile << "#SBATCH --gres=gpu:a100:1" << "\n";
    if(args.partition.has_value()) {
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    }
#endif // GPU_ARCH

    sbatchFile << "#SBATCH -c " << std::to_string(args.cores_per_qpu) << "\n";
    if (args.mem_per_qpu.has_value()) {
        int total_mem = args.mem_per_qpu.value() + args.n_qpus;
        sbatchFile << "#SBATCH --mem=" << std::to_string(total_mem) << "G\n";
    }
#endif //COMPILATION_FOR_GPU
    return true;
}

bool write_qc_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    LOGGER_DEBUG("Inside write_qc_sbatch_header");
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    bool success_writing_resources;
    if (args.gpu) {
        if(!write_qc_gpu_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_qc_gpu_resources failed");
            return false;
        }
    } else {
        if (!write_qc_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_qc_resources failed");
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


bool write_qc_run_command(std::ofstream& sbatchFile,const CunqaArgs& args)
{
    LOGGER_DEBUG("Inside write_qc_run_command");
    #ifdef USE_MPI_BTW_QPU
        LOGGER_ERROR("Quantum Communications are not supported with MPI.");
        return false;
    #endif

    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;
    std::string mode = args.co_located ? "co_located" : "hpc";

    subcommand = mode + " qc " + args.family_name + " " + args.simulator;
    LOGGER_DEBUG("Qraise with quantum communications and default backend. \n");

    
#ifdef USE_ZMQ_BTW_QPU
    if (!args.gpu) {
        int simulator_n_cores = args.cores_per_qpu * args.n_qpus; 
        int simulator_memory;
        if (args.mem_per_qpu.has_value()) {
            simulator_memory = args.mem_per_qpu.value() * args.n_qpus;
        }

        run_command =  "srun --exclusive  -n " + std::to_string(args.n_qpus) + " -c 1 --mem-per-cpu=1G --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand + " &\n";
        // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
        run_command +=  "srun --exclusive  -n 1 -c " + std::to_string(simulator_n_cores) + " --mem=" + std::to_string(simulator_memory) + "G setup_executor " + args.simulator + " " + std::to_string(args.n_qpus) + "\n";
    } else {
#if !COMPILATION_FOR_GPU
        LOGGER_ERROR("CUNQA was not compiled with GPU support.");
        return false;
#endif
        int simulator_n_cores = args.cores_per_qpu - args.n_qpus; 
        int simulator_memory;
        if (args.mem_per_qpu.has_value()) {
            simulator_memory = args.mem_per_qpu.value() * args.n_qpus;
        }

        run_command =  "srun --exclusive  -n " + std::to_string(args.n_qpus) + " -c 1 --mem-per-cpu=1G --gres=gpu:0 --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand + " &\n";
        // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
        run_command += "sleep 1\n";
        run_command +=  "srun --exclusive -n 1 -c " + std::to_string(simulator_n_cores) + " --mem=" + std::to_string(simulator_memory) + "G --gres=gpu:1 setup_executor " + args.simulator + " " + std::to_string(args.n_qpus) + "\n";
    }
#endif //USE_ZMQ_BTW_QPU

    sbatchFile << run_command;

    return true;
}


void write_qc_sbatch(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    if (args.n_qpus == 0 || args.time == "") {
        LOGGER_ERROR("qraise needs two mandatory arguments:\n \t -n: number of vQPUs to be raised\n\t -t: maximum time vQPUs will be raised (hh:mm:ss)\n");
        throw std::runtime_error("Bad arguments.");

    } else if (std::find(constants::SUPPORTED_QC_SIMULATORS.begin(), constants::SUPPORTED_QC_SIMULATORS.end(), std::string(args.simulator)) == constants::SUPPORTED_QC_SIMULATORS.end()) {
        LOGGER_ERROR("Simulator {} is not available for quantum communications simulation. Aborting. ", std::string(args.simulator));
        throw std::runtime_error("Error.");

    } else if (exists_family_name(args.family_name, constants::QPUS_FILEPATH)) {
        LOGGER_ERROR("There are QPUs with the same family name as the provided: {}.", args.family_name.c_str());
        throw std::runtime_error("Bad family name.");

    } else if (!write_qc_sbatch_header(sbatchFile, args) || !write_qc_run_command(sbatchFile, args)) {
        LOGGER_ERROR("Error writing QC sbatch file.");
        throw std::runtime_error("Error.");
    }  
}

} // End namespace 