#pragma once

#include <string>
#include <any>

#include "argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"

std::string get_no_comm_run_command(auto& args, std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;

    if (args.backend.has_value()) {
        if(args.backend.value() == "etiopia_computer.json") {
            LOGGER_ERROR("Terrible mistake. Possible solution: {}", cafe);
            std::system("rm qraise_sbatch_tmp.sbatch");
            return "0";
        } else {
            backend_path = std::any_cast<std::string>(args.backend.value());
            backend = R"({"backend_path":")" + backend_path + R"("})" ;
            subcommand = mode + " no_comm " + std::any_cast<std::string>(args.family_name) + " " + std::any_cast<std::string>(args.simulator) + " \'" + backend + "\'" "\n";
            run_command = "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus $INFO_PATH " + subcommand;
            LOGGER_DEBUG("Qraise with no communications and personalized backend. \n");
        }
    } else {
        subcommand = mode + " no_comm " + std::any_cast<std::string>(args.family_name) + " " + std::any_cast<std::string>(args.simulator) + "\n";
        run_command = "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus $INFO_PATH " + subcommand;
        LOGGER_DEBUG("Qraise default with no communications. \n");
    }

    LOGGER_DEBUG("Run command: {}", run_command);

    return run_command;
}