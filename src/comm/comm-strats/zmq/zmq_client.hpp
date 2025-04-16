#pragma once

#include "zmq.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include "config/net_config.hpp"
#include "utils/constants.hpp"
#include "utils/helpers.hpp"
#include "logger/logger.hpp"

using namespace std::literals;
using namespace config;

class ZmqClient;

class ZmqFuture {
public:
    ZmqFuture(ZmqClient* client);
    std::string get();
    inline bool valid();
private:
    ZmqClient* client;
};

class ZmqClient {
public:
    friend class ZmqFuture;

    // Create a ZeroMQ context with 1 IO thread and a REQ socket.
    ZmqClient() :
        socket_{context_, zmq::socket_type::client}
    { }

    ~ZmqClient() 
    {
        socket_.close();
        // The context will clean up on destruction.
    }

    // Connect using a NetConfig; constructs an endpoint string like "tcp://<ip>:<port>".
    void connect(const NetConfig& server_ipconfig) 
    {
        std::string net;
        try {
            if (server_ipconfig.mode == "cloud") {
                net = INFINIBAND;
                SPDLOG_LOGGER_DEBUG(logger, "CLOUD mode. INFINIBAND selected as net");
            } else {
                net = LOCAL;
                SPDLOG_LOGGER_DEBUG(logger, "HPC mode. LOCALHOST selected as net");
                std::string nodename = server_ipconfig.nodename;
                const char* my_nodename = std::getenv("SLURMD_NODENAME");
                if ((my_nodename == nullptr) || (nodename != (std::string)my_nodename)) {
                    throw std::runtime_error("QPU deployed on node " + nodename + ".");
                }
            }
            std::string endpoint = "tcp://" + server_ipconfig.IPs.at(net) + ":" + server_ipconfig.port;
            socket_.connect(endpoint);
            SPDLOG_LOGGER_DEBUG(logger, "Client successfully connected to server at {}.", endpoint);
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Unable to connect to endpoint {}:{}. Error: {}", 
                                  server_ipconfig.IPs.at(net), server_ipconfig.port, e.what());
        } catch (const std::exception& e) {
            SPDLOG_LOGGER_ERROR(logger, "Trying to connect to a QPU located in a external node. {}", e.what());
            throw;
        }
    }

    // Send the circuit and return a future-like object to later retrieve the response.
    ZmqFuture submit(const std::string& circuit) 
    {   
        _send_data(circuit);
        return ZmqFuture(this);
    }

private:

    // Send the circuit as a single ZeroMQ message.
    void _send_data(const std::string& data) 
    {
        try {
            zmq::message_t message(data.begin(), data.end());
            socket_.send(message, zmq::send_flags::none);
            SPDLOG_LOGGER_DEBUG(logger, "Circuit sent: {}", data);
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error sending the circuit: {}", e.what());
        }
    }

    // Wait to receive a reply message and return its content as a string.
    std::string _recv_result() 
    {
        try {
            zmq::message_t reply;
            auto size = socket_.recv(reply, zmq::recv_flags::none);
            std::string result(static_cast<char*>(reply.data()), size.value());
            SPDLOG_LOGGER_DEBUG(logger, "Result correctly received: {}", result);
            return result;
        } catch (const zmq::error_t& e) {
            SPDLOG_LOGGER_ERROR(logger, "Error receiving the circuit: {}", e.what());
        }

        return std::string("{}");
    }

    zmq::context_t context_;
    zmq::socket_t socket_;
};

ZmqFuture::ZmqFuture(ZmqClient* client) : client{client} { }

std::string ZmqFuture::get() 
{
    return client->_recv_result();
}

inline bool ZmqFuture::valid() 
{
    return true;
}
