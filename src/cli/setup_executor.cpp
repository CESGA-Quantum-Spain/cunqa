
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "sim/backends/quantum_comm/qc_executor.hpp"
#include "sim/simulators/simulator_factory.hpp"

#include "logger.hpp"

using namespace cunqa::sim;

int main(int argc, char *argv[])
{
    std::string sim_arg;
    std::size_t n_qpus;
    if (argc == 3) {
        sim_arg = argv[1];
        n_qpus = static_cast<size_t>(std::stoull(argv[2]));
    } else {
        LOGGER_ERROR("Passing incorrect number of arguments.");
        return EXIT_FAILURE;
    }

    QCExecutor executor(make_simulator(sim_arg), n_qpus);
    executor.run();
    
    return EXIT_SUCCESS;
}