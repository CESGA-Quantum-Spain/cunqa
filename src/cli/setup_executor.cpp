
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "qpu.hpp"
#include "backends/simulators/Munich/munich_executor.hpp"

#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"
#include "logger.hpp"

using namespace std::string_literals;

using namespace cunqa::sim;

int main(int argc, char *argv[])
{
    std::string sim_arg;
    if (argc == 2)
        sim_arg = argv[1]; 
    else {
        LOGGER_ERROR("Passing incorrect number of arguments.");
        return EXIT_FAILURE;
    }

    switch(murmur::hash(sim_arg)) {
        case murmur::hash("Aer"): 
            LOGGER_ERROR("Aer does not support quantum communications.");
            break;
        case murmur::hash("Munich"):
        {
            LOGGER_DEBUG("Raising executor with Munich.");
            MunichExecutor executor;
            executor.run();
            break;
        }
        default:
            LOGGER_ERROR("Not a supported simulator: {}.", sim_arg);
            return EXIT_FAILURE;
    }     
    return EXIT_SUCCESS;
}