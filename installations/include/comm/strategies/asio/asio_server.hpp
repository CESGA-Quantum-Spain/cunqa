#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include "utils/helpers.hpp"
#include "asio_common.hpp"
#include "utils/constants.hpp"
#include "config/net_config.hpp"

using namespace config::net;
using namespace std::string_literals;
using boost::asio::ip::tcp;

class AsioServer {
    NetConfig net_config;
    boost::asio::io_context io_context;
    std::shared_ptr<tcp::socket> socket;
    tcp::acceptor acceptor;

public:
    AsioServer(const NetConfig& net_config, const std::string_view& net = INFINIBAND) :
        net_config{net_config},
        acceptor{io_context, tcp::endpoint{boost::asio::ip::address::from_string(net_config.IPs.at(std::string(net))), 
                            static_cast<unsigned short>(stoul(net_config.port))}}
    { 
        _accept(); 
    }

    std::string recv_data() 
    { 
        return read_string(*socket);
    }

    inline void send_result(const std::string& result) {
        send_string(socket, result);    
    }

private:

    void _accept() {
        socket = std::make_shared<tcp::socket>(acceptor.get_executor());
        acceptor.accept(*socket);
    }
};
