#pragma once

#include "zmq.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <string_view>
#include "utils/helpers.hpp"
#include "utils/constants.hpp"
#include "logger/logger.hpp"
#include "config/net_config.hpp"

using namespace std::literals;
using namespace config;

class ZmqServer {
public:
    ZmqServer(const NetConfig& net_config)
        : net_config_{net_config},
          socket_{context_, zmq::socket_type::server}
    {
        std::string net;
        if (net_config.mode == "cloud") {
            net = INFINIBAND;
        } else {
            net = LOCAL;
        }
        try {
            SPDLOG_LOGGER_DEBUG(logger, "Trying to bind ZMQ socket to selected IP");
            std::string endpoint = "tcp://" + net_config_.IPs.at(net) + ":" + net_config_.port;
            socket_.bind(endpoint);
            SPDLOG_LOGGER_DEBUG(logger, "Server bound to {}.", endpoint);
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error binding to endpoint {}: {}.", 
                                    net_config_.IPs.at(net) + ":" + net_config_.port, e.what());
            throw;
        }
    }

    void accept()
    {
        SPDLOG_LOGGER_DEBUG(logger, "In ZMQ server we just keep going...");
    }

    // Wait for a request from a client and return the received data as a string.
    std::string recv_data() 
    { 
        try {
            zmq::message_t message;
            auto size = socket_.recv(message, zmq::recv_flags::none);
            std::string data(static_cast<char*>(message.data()), size.value());
            SPDLOG_LOGGER_DEBUG(logger, "Received data: {}", data);
            
            rid_queue_.push(message.routing_id());
            return data;
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error receiving data: {}", e.what());
            return std::string("CLOSE");
        }
    }

    // Send a reply to the client.
    inline void send_result(const std::string& result) 
    {
        try {
            zmq::message_t message(result.begin(), result.end());
            message.set_routing_id(rid_queue_.front());
            rid_queue_.pop();

            socket_.send(message, zmq::send_flags::none);
            SPDLOG_LOGGER_DEBUG(logger, "Sent result: {}", result);
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error sending result: {}", e.what());
            throw;
        }
    }

    // Close the server socket.
    inline void close()
    {
        socket_.close();
    }

private:
    NetConfig net_config_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::queue<uint32_t> rid_queue_;
};
