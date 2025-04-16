#pragma once

#include <iostream>
#include <fstream>
#include <string_view>
#include <memory>
#include "comm_strat_def.h"
#include "utils/constants.hpp"
#include "logger/logger.hpp"
#include "config/net_config.hpp"
#include "future_wrapper.hpp"
#include "logger/logger.hpp"

using json = nlohmann::json;
using namespace config;

#if COMM_LIB == ASIO
    #include "comm-strats/asio/asio_client.hpp"
    using SelectedClient = AsioClient;
#elif COMM_LIB == ZMQ
    #include "comm-strats/zmq/zmq_client.hpp"
    using SelectedClient = ZmqClient;
#elif COMM_LIB == CROW
    #include "comm-strats/crow_comm.hpp"
    using SelectedClient = CrowClient;
#else
    #error "A valid library should be defined (ASIO, ZMQ o CROW) in COMM_LIB."
#endif

class Client {
    std::unique_ptr<SelectedClient> comm_strat;
    json qpus_json;

public:

    Client(const std::optional<std::string> &filepath) :
        comm_strat{std::make_unique<SelectedClient>()} 
    { 
        std::string final_filepath;
        if (filepath.has_value())
            final_filepath = filepath.value();
        else
            final_filepath = std::getenv("STORE") + "/.cunqa/qpus.json"s;
        std::ifstream file(final_filepath);  

        if (!file.is_open()) {
            std::cerr << "Cannot open the JSON file\n";
        }

        try {
            file >> qpus_json;
        } catch (const json::parse_error& e) {
            std::cerr << "Error parsing the QClient info into JSON: " << e.what() << "\n";
        }
    }

    void connect(const std::string& task_id) {
        try {
            json server_ip_config_json = qpus_json.at(task_id).at("net");
            auto server_ip_config = server_ip_config_json.template get<NetConfig>();
            comm_strat->connect(server_ip_config);
        } catch (const json::out_of_range& e){
            SPDLOG_LOGGER_ERROR(logger, "No server has ID={}. Remember to set the servers with the command qraise.", task_id);
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error on when connecting the client");
        }
    }

    inline FutureWrapper send_circuit(const std::string& circuit) { return FutureWrapper(comm_strat->submit(circuit)); }

    inline FutureWrapper send_parameters(const std::string& parameters) { return FutureWrapper(comm_strat->submit(parameters)); }
};
