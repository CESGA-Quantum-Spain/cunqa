#pragma once

#include <string>
#include <any>

#include "argparse.hpp"
#include "utils/constants.hpp"
#include "args_qraise.hpp"
#include "logger.hpp"

std::string get_fakeqmio_run_command(auto& args, std::string& mode)
{
    std::string run_command;
    std::string subcommand;
    std::string backend_path;
    std::string backend;
    int thermal_relaxation;
    int readout_error;
    int gate_error;

    if (args.no_thermal_relaxation){
        thermal_relaxation = 0;
    }else{
        thermal_relaxation = 1;
    }

    if (args.no_readout_error){
        readout_error = 0;
    }else{
        readout_error = 1;
    }

    if (args.no_gate_error){
        gate_error = 0;
    }else{
        gate_error = 1;
    }

    backend_path = std::any_cast<std::string>(args.fakeqmio.value());

    backend = R"({"fakeqmio_path":")" + backend_path
            + R"(","thermal_relaxation":")" +  std::to_string(thermal_relaxation)
            + R"(","readout_error":")" +  std::to_string(readout_error)
            + R"(","gate_error":")" +  std::to_string(gate_error)+ R"("})" ;

    subcommand = mode + " no_comm " + std::any_cast<std::string>(args.family_name) + " Aer \'" + backend + "\'" + "\n";
    run_command =  "srun --task-epilog=$BINARIES_DIR/epilog.sh setup_qpus $INFO_PATH " + subcommand;
    LOGGER_DEBUG("Qraise FakeQmio. \n");
    LOGGER_DEBUG("Run command: {}", run_command);

    return run_command;
}