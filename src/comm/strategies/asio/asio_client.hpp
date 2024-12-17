#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include "config/net_config.hpp"
#include "utils/constants.hpp"
#include "utils/helpers.hpp"
//#include "asio_common.hpp"
#include <future>
#include <thread>

using boost::asio::ip::tcp;
using namespace std::literals;
using namespace config;

class AsioClient {
public:

    AsioClient() :
        socket_{io_context_} 
    { }

    ~AsioClient() {
        socket_.close();
        io_context_.stop();
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    
    inline void connect(const NetConfig& server_ipconfig, const std::string_view& net = INFINIBAND) 
    {
        tcp::resolver resolver{io_context_};
        auto endpoint = resolver.resolve(server_ipconfig.IPs.at(std::string(net)), server_ipconfig.port);
        boost::asio::connect(socket_, endpoint);

        worker_ = std::thread([this]() { io_context_.run(); });
    }

    std::future<std::string> submit(const std::string& data) 
    {        
        _send_data(data);
        auto promise = _recv_result();
        return promise->get_future();
    }

private:

    void _send_data(const std::string& data) {
        auto data_length = legacy_size_cast<uint32_t, std::size_t>(data.size());
        auto data_length_network = htonl(data_length);
        auto buffer = boost::asio::buffer(&data_length_network, sizeof(data_length_network));

        socket_.write_some(buffer);
        socket_.write_some(boost::asio::buffer(data));
    }

    std::shared_ptr<std::promise<std::string>> _recv_result(){
        auto promise = std::make_shared<std::promise<std::string>>();
        auto result = std::make_shared<std::string>(); // Crear el objeto antes del callback
        auto data_length_network = std::make_shared<uint32_t>();
        socket_.async_read_some(
            boost::asio::buffer(data_length_network.get(), sizeof(uint32_t)),
            [this, data_length_network, result, promise](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    auto data_length = ntohl(*data_length_network);
                    *result = std::string(data_length, '\0');
                    socket_.async_read_some(
                        boost::asio::buffer(*result),
                        [result, promise](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                            if (!ec) {
                                promise->set_value(result->data());
                            } else {
                                std::cerr << "Error en la segunda lectura: " << ec.message() << std::endl;
                            }
                        }
                    );
                } else {
                    std::cerr << "Error en la lectura: " << ec.message() << std::endl;
                }
            }
        );
        return promise;
    }

    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::thread worker_;
};