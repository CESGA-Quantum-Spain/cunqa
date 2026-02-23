#pragma once

#include <string>
#include <algorithm>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"

using namespace cunqa;


bool write_cc_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#SBATCH --ntasks=" << args.n_qpus << "\n";
    sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";
    sbatchFile << "#SBATCH -N " << args.number_of_nodes.value() << "\n";
    
    if(args.partition.has_value())
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    
    if (args.qpus_per_node.has_value()) {
        if (args.n_qpus < args.qpus_per_node) {
            LOGGER_ERROR("Less QPUs than qpus_per_node");
            return false;
        } else {
            sbatchFile << "#SBATCH --ntasks-per-node=" << args.qpus_per_node.value() << "\n";
        }
    }
    
    if (args.node_list.has_value()) {
        if (args.number_of_nodes.value() != args.node_list.value().size()) {
            LOGGER_ERROR("Different number of node names than total nodes");
            return false;
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
    }

    if (args.mem_per_qpu.has_value() && check_mem_format(args.mem_per_qpu.value())) {
        int mem_per_cpu = (args.mem_per_qpu.value()/args.cores_per_qpu != 0) ? args.mem_per_qpu.value()/args.cores_per_qpu : 1;
        sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_cpu << "G\n";
    } else if (args.mem_per_qpu.has_value() && !check_mem_format(args.mem_per_qpu.value())) {
        LOGGER_ERROR("Memory format is incorrect, must be: xG (where x is the number of Gigabytes).");
        return false;
    } else if (!args.mem_per_qpu.has_value()) {
        int mem_per_core = DEFAULT_MEM_PER_CORE;
        sbatchFile << "#SBATCH --mem-per-cpu=" << mem_per_core << "G\n";
    } 
    
    return true;
}

bool write_cc_gpu_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
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

    if (args.n_qpus > MAX_GPUS_PER_NODE) {
        LOGGER_ERROR("Node with GPU_ARCH = {} only supports {} QPU", std::to_string(GPU_ARCH), std::to_string(MAX_GPUS_PER_NODE));
        return false;
    }
#if GPU_ARCH == 75
    sbatchFile << "#SBATCH --ntasks=" << args.n_qpus << "\n";
    sbatchFile << "#SBATCH --gres=gpu:t4\n";
    if(args.partition.has_value()) {
        sbatchFile << "#SBATCH --partition=" << args.partition.value() << "\n";
    } else {
        sbatchFile << "#SBATCH -p viz\n";
    }
    sbatchFile << "#SBATCH -c " << args.cores_per_qpu << "\n";
    sbatchFile << "#SBATCH --mem=" << args.n_qpus * args.cores_per_qpu * DEFAULT_MEM_PER_CORE << "G\n";
#elif GPU_ARCH == 80
    int mem_per_qpu = args.mem_per_qpu.has_value() ? args.mem_per_qpu.value() : DEFAULT_MEM_PER_CORE * args.cores_per_qpu; 
    int mem = mem_per_qpu * args.n_qpus;
        
    sbatchFile << "#SBATCH --ntasks=" << std::to_string(args.n_qpus) << "\n";
    sbatchFile << "#SBATCH --gres=gpu:a100:" << std::to_string(args.n_qpus) << "\n";
    sbatchFile << "#SBATCH -c " << std::to_string(args.cores_per_qpu) << "\n";
    sbatchFile << "#SBATCH --mem=" << std::to_string(mem) << "G\n";
#endif // GPU_ARCH
#endif //COMPILATION_FOR_GPU

    return true;
}

bool write_cc_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    bool success_writing_resources;
    if (args.gpu) {
        if(!write_cc_gpu_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_cc_gpu_resources failed");
            return false;
        }
    } else {
        if (!write_cc_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_cc_resources failed");
            return false;
        }
    }

    if (check_time_format(args.time))
        sbatchFile << "#SBATCH --time=" << args.time << "\n";
    else {
        LOGGER_ERROR("Incorrect time format");
        return false;
    }

    //sbatchFile << "#SBATCH --profile=all\n";   // Enable comprehensive profiling
    sbatchFile << "#SBATCH --output=qraise_%j\n\n";
    sbatchFile << "unset SLURM_MEM_PER_CPU SLURM_CPU_BIND_LIST SLURM_CPU_BIND\n";
    sbatchFile << "EPILOG_PATH=" << std::string(constants::CUNQA_PATH) << "/epilog.sh\n";

    return true;
}

bool write_cc_run_command(std::ofstream& sbatchFile,const CunqaArgs& args)
{
    std::vector<std::string> simulators_with_cc = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    bool is_available_cc_simulator = std::find(simulators_with_cc.begin(), simulators_with_cc.end(), std::string(args.simulator)) != simulators_with_cc.end();
    if (!is_available_cc_simulator) {
        LOGGER_ERROR("Available CC simulators are \"Aer\", \"Cunqa\", \"Munich\", \"Maestro\" and \"Qulacs\" , but the following was provided: {}", std::string(args.simulator));
        return false;
    } 

    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;
    std::string mode = args.co_located ? "co_located" : "hpc";

    if (args.backend.has_value()) {
        backend_path = std::string(args.backend.value());
        backend = R"({"backend_path":")" + backend_path + R"("})" ;
        subcommand = mode + " cc " + args.family_name + " " + args.simulator + " \'" + backend + "\'\n";
        LOGGER_DEBUG("Qraise with classical communications and personalized CunqaSimulator backend. \n");
    } else {
        subcommand = mode + " cc " + args.family_name + " " + args.simulator + "\n";
        LOGGER_DEBUG("Qraise with classical communications and default CunqaSimulator backend. \n");
    }

#ifdef USE_MPI_BTW_QPU
    run_command =  "srun --mpi=pmix --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with MPI comm: {}", run_command);
#elif defined(USE_ZMQ_BTW_QPU)
    run_command =  "srun --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with ZMQ comm: {}", run_command);
#endif

    sbatchFile << run_command;

    return true;
}


void write_cc_sbatch(std::ofstream& sbatchFile,const CunqaArgs& args)
{
    if (args.n_qpus == 0 || args.time == "") {
        LOGGER_ERROR("qraise needs two mandatory arguments:\n \t -n: number of vQPUs to be raised\n\t -t: maximum time vQPUs will be raised (hh:mm:ss)\n");
        throw std::runtime_error("Bad arguments.");
    } else if (exists_family_name(args.family_name, constants::QPUS_FILEPATH)) {
        LOGGER_ERROR("There are QPUs with the same family name as the provided: {}.", args.family_name.c_str());
        throw std::runtime_error("Bad family name.");
    } else if (!write_cc_sbatch_header(sbatchFile, args) || !write_cc_run_command(sbatchFile, args)) {
        LOGGER_ERROR("Error writing CC sbatch file.");
        throw std::runtime_error("Error.");
    }
    
}