#pragma once

#include <string>
#include <any>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "logger.hpp"
#include "args_qraise.hpp"

using namespace cunqa;


bool write_noise_model_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
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

bool write_noise_model_gpu_resources(std::ofstream& sbatchFile, const CunqaArgs& args)
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

bool write_noise_model_sbatch_header(std::ofstream& sbatchFile, const CunqaArgs& args)
{
    sbatchFile << "#!/bin/bash\n";
    sbatchFile << "#SBATCH --job-name=qraise \n";

    bool success_writing_resources;
    if (args.gpu) {
        if(!write_noise_model_gpu_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_noise_model_gpu_resources failed");
            return false;
        }
    } else {
        if (!write_noise_model_resources(sbatchFile, args)) {
            LOGGER_ERROR("write_noise_model_gpu_resources failed");
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

bool write_noise_model_run_command(std::ofstream& sbatchFile,const CunqaArgs& args)
{
    std::string run_command;
    std::string subcommand;
    std::string noise_properties_path;
    std::string noise_properties;
    int thermal_relaxation;
    int readout_error;
    int gate_error;
    int fakeqmio;
    std::string mode = args.co_located ? "co_located" : "hpc";

    thermal_relaxation = args.no_thermal_relaxation ? 0 : 1;
    readout_error = args.no_readout_error ? 0 : 1;
    gate_error = args.no_gate_error ? 0 : 1;
    fakeqmio = args.fakeqmio.has_value() ? 1 : 0;
    noise_properties_path = args.fakeqmio.has_value() ? std::any_cast<std::string>(args.fakeqmio.value()) : std::any_cast<std::string>(args.noise_properties.value());

    noise_properties = R"({"noise_properties_path":")" + noise_properties_path
               + R"(","thermal_relaxation":")" +  std::to_string(thermal_relaxation)
               + R"(","readout_error":")" +  std::to_string(readout_error)
               + R"(","gate_error":")" +  std::to_string(gate_error)
               + R"(","fakeqmio":")" +  std::to_string(fakeqmio)+ R"("})" ;

    subcommand = mode + " no_comm " + std::any_cast<std::string>(args.family_name) + " Aer \'" + noise_properties + "\'" + "\n";
    run_command =  "srun --task-epilog=$EPILOG_PATH setup_qpus " + subcommand;

    sbatchFile << run_command;

    return true;
}

void write_noise_model_sbatch(std::ofstream& sbatchFile,const CunqaArgs& args)
{
    if (args.n_qpus == 0 || args.time == "") {
        LOGGER_ERROR("qraise needs two mandatory arguments:\n \t -n: number of vQPUs to be raised\n\t -t: maximum time vQPUs will be raised (hh:mm:ss)\n");
        throw std::runtime_error("Bad arguments.");
    } else if (exists_family_name(args.family_name, constants::QPUS_FILEPATH)) {
        LOGGER_ERROR("There are QPUs with the same family name as the provided: {}.", args.family_name.c_str());
        throw std::runtime_error("Bad family name.");
    } else if (args.simulator == "Munich" or args.simulator == "Cunqa"){
        LOGGER_ERROR("Personalized noise models are only supported in AerSimulatorbut {} was provided.", args.simulator.c_str());
        throw std::runtime_error("Bad simulator.");
    } else if (args.cc || args.qc){
        LOGGER_ERROR("Personalized noise models not supported with classical/quantum communications schemes");
        throw std::runtime_error("Bad communication scheme.");
    } else if (args.backend.has_value()) {
        LOGGER_WARN("Because noise properties were provided backend will be redefined according to them.");
    } else if (!write_noise_model_sbatch_header(sbatchFile, args) || !write_noise_model_run_command(sbatchFile, args)) {
        LOGGER_ERROR("Error writing noise sbatch file.");
        throw std::runtime_error("Error.");
    }  
}