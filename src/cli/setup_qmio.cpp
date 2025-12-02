
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

const std::string filepath = std::string(constants::CUNQA_PATH) + "/.cunqa/qpus.json"s;
//const std::string QPU_ENDPOINT = getenv("ZMQ_SERVER");
const std::string QPU_ENDPOINT = "WIP";

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


void write_qmio_info()
{
    QMIOConfig qmio_config;
    JSON qmio_config_json = qmio_config;

    JSON qpu_info = {
        {"real_qpu", "qmio"},
        {"backend", qmio_config_json},
        {"net", {
            //{"endpoint", server->endpoint},
            {"nodename", "qmio_node"},
            {"mode", "co_located"}
        }},
        {"family", "real_qmio"},
        {"name", "QMIO"}
    };
    write_on_file(qpu_info, filepath);

}


int set_up_intermediary()
{
    std::string command = "python " + constants::INSTALL_PATH + "/cunqa/intermediary.py QMIO";
    const char* c_command = command.c_str();
    int status = std::system(c_command);

    return status;
} 

} // End namespace


int main(int argc, char *argv[]) {

    LOGGER_DEBUG("Inside setup_qmio");
    write_qmio_info();
    int setup = set_up_intermediary();
    
    if (setup == 1) {
        LOGGER_ERROR("An error occur in the intermediary.py.");
        return 1;
    }

    return 0;
}