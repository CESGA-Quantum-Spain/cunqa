#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <sys/file.h>
#include <fcntl.h> 
#include <unistd.h> 

#include "qpu.hpp"
#include "sim/simulators/simulator_factory.hpp"
#include "sim/backends/backend_factory.hpp"

#include "utils/helpers/environment.hpp"
#include "utils/constants.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

using namespace cunqa;
using namespace cunqa::sim;

int main(int argc, char *argv[])
{
    std::string mode(argv[1]);
    std::string communications(argv[2]);
    std::string family(argv[3]);
    std::string sim_arg(argv[4]);

    JSON backend_json;
    if (argc == 6)
        backend_json = read_file(std::string(argv[5]));

    if (family == "default") family = get_env_variable("SLURM_JOB_ID");

    QPU qpu(
        make_backend(make_simulator(sim_arg), communications, backend_json), 
        mode, 
        get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_TASK_PID"), 
        family
    );
    qpu.turn_ON();

    return EXIT_SUCCESS;
}