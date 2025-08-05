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

    if (args.simulator != "Aer" && args.simulator != "Munich" && args.simulator != "Cunqa") {
        LOGGER_ERROR("Quantum communications only are available under \"Aer\", \"Munich\" and \"CUNQA\" simulators, but the following simulator was provided: {}", args.simulator);
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

    subcommand = mode + " qc " + args.family_name + " " + args.simulator;
    LOGGER_DEBUG("Qraise with quantum communications and default CunqaSimulator backend. \n");

    #ifdef USE_ZMQ_BTW_QPU
    int num_ports = args.n_qpus * 3;
    int qpu_task_mem = 1; //TODO
    int qpu_task_n_cores = 1; 
    int simulator_task_mem = args.mem_per_qpu * args.n_qpus - args.n_qpus; //TODO
    int simulator_task_n_cores = args.cores_per_qpu * args.n_qpus; 
    run_command =  "srun -n " + std::to_string(args.n_qpus) + " -c " + std::to_string(qpu_task_n_cores) + " --mem-per-cpu=" + std::to_string(qpu_task_mem) + "G --resv-ports=" + std::to_string(num_ports) + " --task-epilog=$EPILOG_PATH setup_qpus $INFO_PATH " +  subcommand + " &\n";
    LOGGER_DEBUG("Run command with ZMQ comm: {}", run_command);

    // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
    run_command += "sleep 1\n";
    run_command +=  "srun -n 1 -c " + std::to_string(simulator_task_n_cores) + " --mem=" + std::to_string(simulator_task_mem) + "G --resv-ports=" + std::to_string(args.n_qpus) + " setup_executor " + args.simulator + "\n";
    #else
    LOGGER_ERROR("For quantum communications ZMQ has to be available.");
    return "0";
    #endif

    return run_command;
}