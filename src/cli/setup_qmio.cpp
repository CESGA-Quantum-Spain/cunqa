
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <random>

#include "utils/helpers/net_functions.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

int generate_random_port() {
    // Define the range for the random number (49152 to 65535)
    const int min_port = 49152;
    const int max_port = 65535;

    // Create a random device to seed the generator
    std::random_device rd;

    // Use a Mersenne Twister engine seeded with the random device
    std::mt19937 gen(rd());

    // Create a uniform integer distribution for the specified range
    std::uniform_int_distribution<> distrib(min_port, max_port);

    // Generate and return the random number
    return distrib(gen);
}

using namespace cunqa;

int main(int argc, char *argv[]) {

    LOGGER_DEBUG("Deploying Real QPU");
    std::string info_path;
    std::string comm_path;
    if (argc == 3) {
        info_path = argv[1]; 
        comm_path = argv[2];
    } else {
        LOGGER_ERROR("Passing incorrect number of arguments.");
        return EXIT_FAILURE;
    }

    JSON qpu_info = {
        {"real_qpu", "qmio"}
    };

    std::string ip = get_global_IP_address();
    int port = generate_random_port();
    std::string endpoint = "tcp://" + ip + ":" + std::to_string(port);
    JSON comm_info = {
        {"real_qpu", "qmio"},
        {"intermediary_endpoint", endpoint}
    };

    write_on_file(qpu_info, info_path);
    write_on_file(comm_info, comm_path);

    std::string home = std::getenv("HOME");
    std::string cunqa_path = home + "/cunqa";
    std::string command = "python -u " + cunqa_path + "/setup_qmio_server.py " + endpoint;

    std::system(command.c_str());
    
    return 0;
}