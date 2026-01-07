#pragma once

#include <string>
#include <algorithm>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"


std::string get_cc_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::vector<std::string> simulators_with_cc = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    bool is_available_simulator = std::find(simulators_with_cc.begin(), simulators_with_cc.end(), std::string(args.simulator)) != simulators_with_cc.end();

    std::vector<std::string> available_simulators = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    std::string simulator = std::any_cast<std::string>(args.simulator);
    auto check_sim_availability = std::find(available_simulators.begin(), available_simulators.end(), simulator);
    if (check_sim_availability == available_simulators.end()) {
        LOGGER_ERROR("Classical communications only are available under \"Cunqa\", \"Munich\", \"Aer\", \"Maestro\" and \"Qulacs\" simulators, but the following simulator was provided: {}", simulator);
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    if (args.backend.has_value()) {
        backend_path = std::string(args.backend.value());
        backend = R"({"backend_path":")" + backend_path + R"("})" ;
        subcommand = mode + " cc " + std::string(args.family_name) + " " + std::string(args.simulator) + " \'" + backend + "\'" "\n";
        LOGGER_DEBUG("Qraise with classical communications and personalized CunqaSimulator backend. \n");
    } else {
        subcommand = mode + " cc " + std::string(args.family_name) + " " + std::string(args.simulator) + "\n";
        LOGGER_DEBUG("Qraise with classical communications and default CunqaSimulator backend. \n");
    }

#ifdef USE_MPI_BTW_QPU
    run_command =  "srun --mpi=pmix --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with MPI comm: {}", run_command);
#elif defined(USE_ZMQ_BTW_QPU)
    run_command =  "srun --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with ZMQ comm: {}", run_command);
#endif

    return run_command;
}