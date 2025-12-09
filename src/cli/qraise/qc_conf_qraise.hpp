#pragma once

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"

std::string get_qc_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    if (args.simulator != "Aer" && args.simulator != "Munich" && args.simulator != "Cunqa") {
        LOGGER_ERROR("Quantum communications only are available under \"Aer\", \"Munich\" and \"CUNQA\" simulators, but the following simulator was provided: {}", args.simulator);
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

    subcommand = mode + " qc " + args.family_name + " " + args.simulator;
    LOGGER_DEBUG("Qraise with quantum communications and default backend. \n");

    int simulator_n_cores = args.cores_per_qpu * args.n_qpus; 
    int simulator_memory = args.mem_per_qpu.has_value() ? args.mem_per_qpu.value() * args.n_qpus : DEFAULT_MEM_PER_CORE * args.cores_per_qpu * args.n_qpus;
    
    run_command =  "srun -n " + std::to_string(args.n_qpus) + " -c 1 --mem-per-cpu=1G --exclusive --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand + " &\n";

    // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
    run_command += "sleep 1\n";
    run_command +=  "srun -n 1 -c " + std::to_string(simulator_n_cores) + " --mem=" + std::to_string(simulator_memory) + "G --exclusive setup_executor " + args.simulator + " " + args.family_name + "\n";

    #ifndef USE_ZMQ_BTW_QPU
        LOGGER_ERROR("For quantum communications ZMQ has to be available.");
        return "0";
    #endif

    return run_command;
}