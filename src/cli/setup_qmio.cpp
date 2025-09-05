
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <random>

#include "utils/helpers/net_functions.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

using namespace cunqa;

namespace {

struct QMIOConfig {
    std::string name = "QMIOBackend";
    std::string version = "";
    int n_qubits = 32;
    std::string description = "Backend of real QMIO";
    std::vector<std::vector<int>> coupling_map = {{0,1},{2,1},{2,3},{4,3},{5,4},{6,3},{6,12},{7,0},{7,9},{9,10},{11,10},{11,12},{13,21},{14,11},{14,18},{15,8},{15,16},{18,17},{18,19},{20,19},{22,21},{22,31},{23,20},{23,30},{24,17},{24,27},{25,16},{25,26},{26,27},{28,27},{28,29},{30,29},{30,31}};
    std::vector<std::string> basis_gates = {"sx", "x", "rz", "ecr"};
    JSON noise_model = {};
    JSON noise_properties = {};
    std::string noise_path = "";

    friend void to_json(JSON& j, const QMIOConfig& obj)
    {
        j = {   
            {"name", obj.name}, 
            {"version", obj.version},
            {"n_qubits", obj.n_qubits}, 
            {"description", obj.description},
            {"coupling_map", obj.coupling_map},
            {"basis_gates", obj.basis_gates}, 
            {"noise", obj.noise_path}
        };
    }
    
};

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

}


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

    std::string ip = get_global_IP_address();
    int port = generate_random_port();
    QMIOConfig qmio_config;
    JSON config = qmio_config;

    JSON qpu_info = {
        {"real_qpu", "qmio"},
        {"backend", config},
        {"net", {
            {"ip", ip},
            {"port", port},
            {"nodename", "qmio_node"},
            {"mode", "cloud"}
        }},
        {"family", "real_qmio"}
    };

    std::string endpoint = "tcp://" + ip + ":" + std::to_string(port);

    write_on_file(qpu_info, info_path);

    std::string home = std::getenv("HOME");
    std::string cunqa_path = home + "/cunqa";
    std::string command = "python -u " + cunqa_path + "/qmio_helpers.py " + endpoint;

    std::system(command.c_str());
    
    return 0;
}