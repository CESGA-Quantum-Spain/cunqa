
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <queue>
#include "zmq.hpp"

#include "comm/server.hpp"
#include "utils/helpers/net_functions.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

using namespace cunqa;

namespace {

const auto store = getenv("STORE");
const std::string HOME = getenv("HOME");
const std::string filepath = store + "/.cunqa/qpus.json"s;
//const std::string QPU_ENDPOINT = getenv("ZMQ_SERVER");
const std::string QPU_ENDPOINT = "tcp://10.5.7.14:8181";

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


class Intermediary {

public:
    Intermediary() : server(std::make_unique<comm::Server>("cloud")), socket_{context_, zmq::socket_type::req}
    {}

    ~Intermediary() 
    {
        socket_.close();
    }

    void turn_ON()
    {
        QMIOConfig qmio_config;
        JSON qmio_config_json = qmio_config;

        JSON qpu_info = {
            {"real_qpu", "qmio"},
            {"backend", qmio_config_json},
            {"net", {
                {"endpoint", server->endpoint},
                {"nodename", "qmio_node"},
                {"mode", "co_located"}
            }},
            {"family", "real_qmio"},
            {"name", "QMIO"}
        };
        write_on_file(qpu_info, filepath);

        socket_.connect(QPU_ENDPOINT);

        std::thread listen([this](){this->recv_data_();});
        std::thread compute([this](){this->send_to_QPU_();});

        listen.join();
        compute.join();
    }

private:

    std::unique_ptr<comm::Server> server;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::queue<std::string> message_queue_;
    std::condition_variable queue_condition_;
    std::mutex queue_mutex_;

    void recv_data_()
    {
        while (true) {
            try {
                LOGGER_DEBUG("Waiting to recv from outside...");
                auto message = server->recv_data();
                    {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    if (message.compare("CLOSE"s) == 0) {
                        server->accept();
                        continue;
                    }
                    else
                        message_queue_.push(message);
                }
                queue_condition_.notify_one();
            } catch (const std::exception& e) {
                LOGGER_INFO("There has happened an error receiving the circuit, the server keeps on iterating.");
                LOGGER_ERROR("Official message of the error: {}", e.what());
            }
        }
    }

    void send_to_QPU_()
    {}
    
};

} // End namespace


int main(int argc, char *argv[]) {

    LOGGER_DEBUG("Inside setup_qmio");
    Intermediary intermediary;
    intermediary.turn_ON();
    
    return 0;
}