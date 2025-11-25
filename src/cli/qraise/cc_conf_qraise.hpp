#pragma once

#include <string>
#include <any>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"


std::string get_cc_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    if (std::any_cast<std::string>(args.simulator) != "Cunqa" && std::any_cast<std::string>(args.simulator) != "Munich" && std::any_cast<std::string>(args.simulator) != "Aer") {
        LOGGER_ERROR("Classical communications only are available under \"Cunqa\", \"Munich\" and \"Aer\" simulators, but the following simulator was provided: {}", std::any_cast<std::string>(args.simulator));
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

    if (args.backend.has_value()) {
        backend_path = std::any_cast<std::string>(args.backend.value());
        backend = R"({"backend_path":")" + backend_path + R"("})" ;
        subcommand = mode + " cc " + std::any_cast<std::string>(args.family_name) + " " + std::any_cast<std::string>(args.simulator) + " \'" + backend + "\'" "\n";
        LOGGER_DEBUG("Qraise with classical communications and personalized CunqaSimulator backend. \n");
    } else {
        subcommand = mode + " cc " + std::any_cast<std::string>(args.family_name) + " " + std::any_cast<std::string>(args.simulator) + "\n";
        LOGGER_DEBUG("Qraise with classical communications and default CunqaSimulator backend. \n");
    }

    #ifdef USE_MPI_BTW_QPU
    run_command =  "srun --mpi=pmix --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with MPI comm: {}", run_command);
    #endif

    #ifdef USE_ZMQ_BTW_QPU
    run_command =  "srun --task-epilog=$EPILOG_PATH setup_qpus " +  subcommand;
    LOGGER_DEBUG("Run command with ZMQ comm: {}", run_command);
    #endif

    return run_command;
}