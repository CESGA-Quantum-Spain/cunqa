#pragma once

#include <iostream>
#include <memory>
#include "config/net_config.hpp"
#include "strategy_def.h"

using namespace config;

#if COMM_LIB == ASIO
    #include "strategies/asio/asio_server.hpp"
    using SelectedServer = AsioServer;
#elif COMM_LIB == ZMQ
    #include "strategy/zmq_comm.hpp"
    using SelectedServer = ZMQServer;
#elif COMM_LIB == CROW
    #include "strategy/crow_comm.hpp"
    using SelectedServer = CrowServer;
#else
    #error "A valid library should be defined (ASIO, ZMQ o CROW) in COMM_LIB."
#endif

class Server {
    std::unique_ptr<SelectedServer> strategy;
public:

    Server(const NetConfig& net_config) :
        strategy{std::make_unique<SelectedServer>(net_config)} 
    { }

    inline std::string recv_circuit() { return strategy->recv_data(); }
    
    inline void accept() { strategy->accept(); }

    inline void send_result(const std::string& result) { strategy->send_result(result); }
};

