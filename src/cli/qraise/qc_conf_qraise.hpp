#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "utils_qraise.hpp"
#include "logger.hpp"

std::string get_qc_run_command(const CunqaArgs& args, const std::string& mode)
{
    #ifdef USE_MPI_BTW_QPU
        LOGGER_ERROR("Quantum Communications are not supported with MPI.");
        return "0";
    #endif

    std::vector<std::string> simulators_with_qc = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    bool is_available_simulator = std::find(simulators_with_qc.begin(), simulators_with_qc.end(), std::string(args.simulator)) != simulators_with_qc.end();

    std::vector<std::string> available_simulators = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    std::string simulator = std::any_cast<std::string>(args.simulator);
    auto check_sim_availability = std::find(available_simulators.begin(), available_simulators.end(), simulator);
    if (check_sim_availability == available_simulators.end()) {
        LOGGER_ERROR("Quantum communications only are available under \"Aer\", \"Munich\", \"Aer\", \"Maestro\" and \"Qulacs\" simulators, but the following simulator was provided: {}", args.simulator);
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 


    std::string setup_qpus = "setup_qpus";
    std::string setup_executor = "setup_executor";
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    subcommand = mode + " qc " + args.family_name + " " + args.simulator;
    LOGGER_DEBUG("Qraise with quantum communications and default backend. \n");

    int simulator_n_cores = args.cores_per_qpu * args.n_qpus; 
    int simulator_memory = args.mem_per_qpu.has_value() ? args.mem_per_qpu.value() * args.n_qpus : DEFAULT_MEM_PER_CORE * args.cores_per_qpu * args.n_qpus;
    
#ifdef USE_ZMQ_BTW_QPU
    run_command =  "srun -n " + std::to_string(args.n_qpus) + " -c 1 --mem-per-cpu=1G --exclusive --task-epilog=$EPILOG_PATH " + setup_qpus + " " +  subcommand + " &\n";

    // This is done to avoid run conditions in the IP publishing of the QPUs for the executor
    run_command += "sleep 1\n";
    run_command +=  "srun -n 1 -c " + std::to_string(simulator_n_cores) + " --mem=" + std::to_string(simulator_memory) + "G --exclusive " + setup_executor + " " + args.simulator + " " + args.family_name + "\n";
#endif

    return run_command;
}