#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include "comm/server.hpp"
#include "logger.hpp"
#include "utils/helpers/net_functions.hpp"
#include "utils/constants.hpp"

namespace as = boost::asio;
using namespace std::string_literals;
using as::ip::tcp;

namespace cunqa {
namespace comm {

struct Server::Impl {
    as::io_context io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;

    Impl(const std::string& ip, const std::string& port) :
        acceptor_{io_context_, tcp::endpoint{as::ip::address::from_string(ip), 
                            static_cast<unsigned short>(stoul(port))}},
        socket_{acceptor_.get_executor()}
    {}

    void accept()
    {
        acceptor_.accept(socket_);
    }

    std::string recv() 
    {
        try {
            uint32_t data_length_network;
            as::read(socket_, as::buffer(&data_length_network, sizeof(data_length_network)));
            
            uint32_t data_length = ntohl(data_length_network);
            std::string data(data_length, '\0');
            as::read(socket_, as::buffer(&data[0], data_length));
            return data;
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                // Client closed the connection cleanly
                LOGGER_DEBUG("Client disconnected gracefully.");
                socket_.close(); 
                return "CLOSE";
            } else if (e.code() == boost::asio::error::connection_reset) {
                LOGGER_ERROR("Client connection reset (forcible close).");
                socket_.close(); 
                return "CLOSE";
            } else {
                LOGGER_ERROR("Error receiving data: {}", e.what());
                throw; // rethrow unexpected errors
            }
            /* if (e.code() == as::error::eof) {
                LOGGER_DEBUG("Client disconnected, closing conection.");
                socket_->close(); 
                return std::string("CLOSE");
            } else {
                LOGGER_ERROR("Error receiving the circuit.");
                throw;
            } */
        }

        return std::string();
    }

    void send(const std::string& result) 
    {
        try {    
            auto data_length = legacy_size_cast<uint32_t, std::size_t>(result.size());
            auto data_length_network = htonl(data_length);

            as::write(socket_, as::buffer(&data_length_network, sizeof(data_length_network))); 
            as::write(socket_, as::buffer(result));
        } catch (const boost::system::system_error& e) {
            LOGGER_ERROR("Error sending the result.");
            throw;
        }
    }

    void close()
    {
        this->socket_.close();
    }
};

Server::Server(const std::string& mode) :
    mode{mode},
    hostname{get_hostname()},
    nodename{get_nodename()},
    ip{get_IP_address(mode)},
    global_ip{get_global_IP_address()},
    port{get_port()},
    pimpl_{std::make_unique<Impl>(ip, port)}
{ }

Server::~Server() = default;

void Server::accept() 
{
    pimpl_->accept();
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
