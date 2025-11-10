#include "zmq.hpp"
#include <iostream>
#include <string>

#include "comm/client.hpp"
#include "logger.hpp"


namespace cunqa {
namespace comm {
    
struct Client::Impl {
    Impl() :
        socket_{context_, zmq::socket_type::client}
    { }

    ~Impl() 
    {
        socket_.close();
    }

    void connect(const std::string& ip, const std::string& port) 
    {
        try {
            socket_.connect("tcp://" + ip + ":" + port);
            LOGGER_DEBUG("Client successfully connected to server at {}:{}.", ip, port);
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Unable to connect to endpoint {}:{}. Error: {}", ip, port, e.what());
        } catch (const std::exception& e) {
            LOGGER_ERROR("Trying to connect to a QPU located in a external node. {}", e.what());
            throw;
        }
    }

    void send(const std::string& data) 
    {
        try {
            zmq::message_t message(data.begin(), data.end());
            socket_.send(message, zmq::send_flags::none);
            LOGGER_DEBUG("Circuit sent: {}", data);
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error sending the circuit: {}", e.what());
        }
    }

    std::string recv() 
    {
        try {
            zmq::message_t reply;
            auto size = socket_.recv(reply, zmq::recv_flags::none);
            std::string result(static_cast<char*>(reply.data()), size.value());
            LOGGER_DEBUG("Result correctly received: {}", result);
            return result;
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error receiving the circuit: {}", e.what());
        }

        return std::string("{}");
    }

    void disconnect(const std::string& endpoint)
    {
        if (endpoint != "") {
            socket_.disconnect(endpoint);
        } else {
            socket_.close();
            socket_ = zmq::socket_t(context_, zmq::socket_type::client);
        }
    }

    zmq::context_t context_;
    zmq::socket_t socket_;
};


Client::Client() :
    pimpl_{std::make_unique<Impl>()}
{ }

Client::~Client() = default;

void Client::connect(const std::string& ip, const std::string& port) {
    pimpl_->connect(ip, port);
}

FutureWrapper<Client> Client::send_circuit(const std::string& circuit) 
{ 
    pimpl_->send(circuit);
    return FutureWrapper<Client>(this); 
}

FutureWrapper<Client> Client::send_parameters(const std::string& parameters) 
{ 
    pimpl_->send(parameters);
    return FutureWrapper<Client>(this); 
}

std::string Client::recv_results() {
    return pimpl_->recv();
}

void Client::disconnect(const std::string& endpoint) {
    pimpl_->disconnect(endpoint);
}

} // End of comm namespace
} // End of cunqa namespace