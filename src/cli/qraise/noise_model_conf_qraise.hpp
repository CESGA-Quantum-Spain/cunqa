#pragma once

#include <string>
#include <any>

#include "argparse/argparse.hpp"
#include "utils/constants.hpp"
#include "logger.hpp"
#include "args_qraise.hpp"

std::string get_noise_model_run_command(const CunqaArgs& args, const std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string noise_properties_path;
    std::string noise_properties;
    int thermal_relaxation;
    int readout_error;
    int gate_error;
    int fakeqmio;

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
    LOGGER_DEBUG("Qraise noisy CunqaBackend. \n");
    LOGGER_DEBUG("Run command: {}", run_command);

    return run_command;
}