
#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include "comm/client.hpp"
#include "logger.hpp"
#include "utils/helpers/net_functions.hpp"

namespace as = boost::asio;
using namespace std::string_literals;
using as::ip::tcp;

namespace cunqa {
namespace comm {

struct Client::Impl {
    as::io_context io_context_;
    tcp::socket socket_;

    Impl() :
        io_context_{},
        socket_{io_context_}
    { }

    ~Impl() 
    {
        socket_.close();
        io_context_.stop();
    }

    void connect(const std::string& ip, const std::string& port) 
    {   
        try {
            tcp::resolver resolver{io_context_};
            auto endpoint = resolver.resolve(ip, port);
            as::connect(socket_, endpoint);
            LOGGER_DEBUG("Client succesfully connected to server.");
        } catch (const boost::system::system_error& e) {
            LOGGER_ERROR("Imposible to connect to endpoint {}:{}. Server not available.", ip, port);
            throw;
        }
    }

    void send(const std::string& data) 
    {
        auto data_length = legacy_size_cast<uint32_t, std::size_t>(data.size());
        auto data_length_network = htonl(data_length);

        try {
            as::write(socket_, as::buffer(&data_length_network, sizeof(data_length_network))); 
            as::write(socket_, as::buffer(data));
            LOGGER_DEBUG("Message sent.");
        } catch (const boost::system::system_error& e) {
            LOGGER_ERROR("Error sending the circuit.");
        }
        
    }

    std::string recv() 
    {
        try {
            uint32_t result_length_network;
            as::read(socket_, as::buffer(&result_length_network, sizeof(result_length_network)));
            uint32_t result_length = ntohl(result_length_network);

            std::string result(result_length, '\0');
            as::read(socket_, as::buffer(&result[0], result_length));
            LOGGER_DEBUG("Result received: {}", result);
            return result;
        } catch (const boost::system::system_error& e) {
            LOGGER_ERROR("Error receiving the circuit: {} (HINT: Check the circuit format and/or if QPUs are still up working.)", e.code().message());
        }

        return std::string("{}");
    }

    void disconnect()
    {
        socket_.close(); // Only a unique server per client
        socket_ = tcp::socket(socket_.get_executor());
    }
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
    pimpl_->disconnect();
}

} // End of comm namespace
} // End of cunqa namespace