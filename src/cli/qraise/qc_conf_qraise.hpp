#pragma once

#include "argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"

std::string get_qc_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    if (args.simulator != "Cunqa" && args.simulator != "Munich") {
        LOGGER_ERROR("Quantum communications only are available under \"Munich\" simulators, but the following simulator was provided: {}", args.simulator);
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

    subcommand = mode + " qc " + args.family_name + " " + args.simulator;
    LOGGER_DEBUG("Qraise with quantum communications and default CunqaSimulator backend. \n");

    #ifdef USE_ZMQ_BTW_QPU
    int num_ports = args.n_qpus * 3;
    run_command =  "srun -n " + std::to_string(args.n_qpus) + " --resv-ports=" + std::to_string(num_ports) + " --task-epilog=$EPILOG_PATH setup_qpus $INFO_PATH " +  subcommand + " &\n";
    LOGGER_DEBUG("Run command with ZMQ comm: {}", run_command);

    // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
    run_command += "sleep 1\n";
    run_command +=  "srun -n 1 --resv-ports=" + std::to_string(args.n_qpus) + " setup_executor " + args.simulator + "\n";
    #else
    LOGGER_ERROR("For quantum communications ZMQ has to be available.");
    return "0";
    #endif

    return run_command;
}