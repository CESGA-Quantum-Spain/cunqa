
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "qpu.hpp"
#include "backends/simulators/AER/aer_executor.hpp"
#include "backends/simulators/Munich/munich_executor.hpp"
#include "backends/simulators/CUNQA/cunqa_executor.hpp"


#include "utils/json.hpp"
#include "utils/helpers/murmur_hash.hpp"
#include "logger.hpp"

using namespace std::string_literals;

using namespace cunqa::sim;

int main(int argc, char *argv[])
{
    std::string sim_arg;
    std::string family_name;
    if (argc == 3) {
        sim_arg = argv[1];
        family_name = argv[2]; 
    } else {
        LOGGER_ERROR("Passing incorrect number of arguments.");
        return EXIT_FAILURE;
    }

    if (family_name == "default")
        family_name = std::getenv("SLURM_JOB_ID");

    switch(murmur::hash(sim_arg)) {
        case murmur::hash("Aer"): 
        {
            LOGGER_DEBUG("Raising executor with Aer.");
            AerExecutor executor(family_name);
            executor.run();
            break;
        }
        case murmur::hash("Munich"):
        {
            LOGGER_DEBUG("Raising executor with Munich.");
            MunichExecutor executor(family_name);
            executor.run();
            break;
        }
        case murmur::hash("Cunqa"):
        {
            LOGGER_DEBUG("Raising executor with Cunqa.");
            CunqaExecutor executor(family_name);
            executor.run();
            break;
        }
        default:
            LOGGER_ERROR("Not a supported simulator: {}.", sim_arg);
            return EXIT_FAILURE;
    }     
    return EXIT_SUCCESS;
}