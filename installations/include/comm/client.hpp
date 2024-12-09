#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include "strategy_def.h"
#include "utils/constants.hpp"
#include "config/net_config.hpp"

using json = nlohmann::json;
using namespace config::net;

#if COMM_LIB == ASIO
    #include "strategies/asio/asio_client.hpp"
    using SelectedClient = AsioClient;
#elif COMM_LIB == ZMQ
    #include "strategy/zmq_comm.hpp"
    using SelectedClient = ZMQClient;
#elif COMM_LIB == CROW
    #include "strategy/crow_comm.hpp"
    using SelectedClient = CrowClient;
#else
    #error "A valid library should be defined (ASIO, ZMQ o CROW) in COMM_LIB."
#endif

class Client {
    std::unique_ptr<SelectedClient> strategy;
    json qpus_json;

public:

    Client(const std::optional<std::string> &filepath) :
        strategy{std::make_unique<SelectedClient>()} 
    { 
        std::string final_filepath;
        if (filepath.has_value())
            final_filepath = filepath.value();
        else
            final_filepath = std::getenv("STORE") + "/.api_simulator/qpu.json"s;
        std::ifstream file(final_filepath);  

        if (!file.is_open()) {
            std::cerr << "Cannot open the JSON file\n";
        }

        try {
            file >> qpus_json;
        } catch (const json::parse_error& e) {
            std::cerr << "Error parsing the QPU info into JSON: " << e.what() << "\n";
        }
    }

    void connect(const int& qpu_id, const std::string_view& net = INFINIBAND) {
        json server_ip_config_json = qpus_json.at(std::to_string(qpu_id));
        auto server_ip_config = server_ip_config_json.template get<NetConfig>();
        strategy->connect(server_ip_config);
    }

    inline std::string read_result() { return strategy->read_result(); }

    inline void send_data(const std::string& data) { strategy->send_data(data); }

    inline void send_data(std::ifstream& file) { strategy->send_data(file); }
    
    inline void stop() { strategy->stop(); }

};
