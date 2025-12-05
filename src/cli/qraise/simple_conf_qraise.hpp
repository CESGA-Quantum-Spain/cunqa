#pragma once

#include <string>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"

std::string get_simple_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::vector<std::string> simple_simulators = {"Cunqa", "Aer", "Munich", "Maestro", "Qulacs"};
    bool is_available_simulator = std::find(simple_simulators.begin(), simple_simulators.end(), std::string(args.simulator)) != simple_simulators.end();

    if (!is_available_simulator) {
        LOGGER_ERROR("Available simple simulators are \"Aer\", \"Cunqa\", \"Munich\", \"Maestro\" and \"Qulacs\" , but the following was provided: {}", std::string(args.simulator));
        std::system("rm qraise_sbatch_tmp.sbatch");
        return "0";
    } 

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
            backend_path = std::string(args.backend.value());
            backend = R"({"backend_path":")" + backend_path + R"("})" ;
            subcommand = mode + " no_comm " + std::string(args.family_name) + " " + std::string(args.simulator) + " \'" + backend + "\'" "\n";
            run_command = "srun --task-epilog=$EPILOG_PATH setup_qpus " + subcommand;
            LOGGER_DEBUG("Qraise with no communications and personalized backend. \n");
        }
    } else {
        subcommand = mode + " no_comm " + std::string(args.family_name) + " " + std::string(args.simulator) + "\n";
        run_command = "srun --task-epilog=$EPILOG_PATH setup_qpus " + subcommand;
        LOGGER_DEBUG("Qraise default with no communications. \n");
    }

    LOGGER_DEBUG("Run command: {}", run_command);

    return run_command;
}