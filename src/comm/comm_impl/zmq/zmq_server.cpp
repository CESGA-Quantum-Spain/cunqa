#include "zmq.hpp"

#include "comm/server.hpp"
#include "logger.hpp"
#include "utils/helpers/net_functions.hpp"

namespace cunqa {
namespace comm {

struct Server::Impl {
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::queue<uint32_t> rid_queue_;

    std::string zmq_endpoint;

    Impl(const std::string& mode) :
        socket_{context_, zmq::socket_type::server}
    {
        try {
            std::string ip = (mode == "hpc" ? "127.0.0.1"s : get_IP_address());
            socket_.bind("tcp://" + ip + ":*");
            
            char endpoint[256];
            size_t sz = sizeof(endpoint);
            zmq_getsockopt(socket_, ZMQ_LAST_ENDPOINT, endpoint, &sz);
            zmq_endpoint = std::string(endpoint);
            LOGGER_DEBUG("Server bound to {}", endpoint);

        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error binding to endpoint: ", e.what());
            throw;
        }
    }

    std::string recv() 
    { 
        try {
            zmq::message_t message;
            auto size = socket_.recv(message, zmq::recv_flags::none);
            std::string data(static_cast<char*>(message.data()), size.value());
            //LOGGER_DEBUG("Received data: {}", data);
            
            rid_queue_.push(message.routing_id());
            return data;
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error receiving data: {}", e.what());
            return std::string("CLOSE");
        }
    }

    void send(const std::string& result) 
    {
        try {
            zmq::message_t message(result.begin(), result.end());
            message.set_routing_id(rid_queue_.front());
            rid_queue_.pop();
            
            socket_.send(message, zmq::send_flags::none);
            //LOGGER_DEBUG("Sent result: {}", result);
        } catch (const zmq::error_t& e) {
            LOGGER_ERROR("Error sending result: {}", e.what());
            throw;
        }
    }

    void close()
    {
        socket_.close();
    }
};

Server::Server(const std::string& mode) :
    mode{mode},
    nodename{get_nodename()},
    pimpl_{std::make_unique<Impl>(mode)}
{ 
    endpoint = pimpl_->zmq_endpoint;
}

Server::~Server() = default;

void Server::accept() 
{
    // ZMQ does not need to accept connection as Asio
}

std::string Server::recv_data() 
{ 
    return pimpl_->recv();
}

void Server::send_result(const std::string& result) 
{ 
    try {
        pimpl_->send(result);
    } catch (const std::exception& e) {
        throw ServerException(e.what());
    }
}

void Server::close() 
{
    pimpl_->close();
}

} // End of comm namespace
} // End of cunqa namespace
