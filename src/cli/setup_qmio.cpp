
#include <cstdlib>
#include <iostream>
#include <string>

#include "comm/server.hpp"
#include "utils/helpers/net_functions.hpp"
#include "utils/json.hpp"
#include "utils/constants.hpp"
#include "logger.hpp"

using namespace cunqa;

namespace {

int set_up_linker(const std::string& family)
{
    std::string command = "python " + constants::INSTALL_PATH + "/cunqa/real_qpus/qmio_linker.py " + family;
    const char* c_command = command.c_str();
    int status = std::system(c_command);

    return status;
} 

} // End namespace


int main(int argc, char *argv[]) {

    if (argc < 2) {
        LOGGER_ERROR("No family name was provided for QMIO");
        return 1;
    }

    std::string family = argv[1];

    if (family == "default") {
        family = std::getenv("SLURM_JOB_ID");
    }
    
    int setup = set_up_linker(std::string(family));
    
    if (setup == 1) {
        LOGGER_ERROR("An error occur in the qmio_linker.py.");
        return 1;
    }

    return 0;
}