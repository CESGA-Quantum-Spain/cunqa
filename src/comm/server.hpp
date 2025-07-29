#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <string>

#include "backends/simple_backend.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace comm {

class ServerException : public std::exception {
    std::string message;
public:
    explicit ServerException(const std::string& msg) : message(msg) { }

    const char* what() const noexcept override {
        return message.c_str();
    }
};

class Server {
public:
    std::string mode;
    std::string hostname;
    std::string nodename;
    std::string ip;
    std::string global_ip;
    std::string port;

    Server(const std::string& mode);
    ~Server();

    void accept();
    std::string recv_data();
    void send_result(const std::string& result);
    void close();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    friend void to_json(JSON& j, const Server& obj) {
        j = {   
            {"mode", obj.mode}, 
            {"hostname", obj.hostname},
            {"nodename", obj.nodename}, 
            {"ip", obj.ip},
            {"global_ip", obj.global_ip},
            {"port", obj.port},
        };
    }

    friend void from_json(const JSON& j, Server& obj) {
        j.at("mode").get_to(obj.mode);
        j.at("hostname").get_to(obj.hostname);
        j.at("nodename").get_to(obj.nodename);
        j.at("ip").get_to(obj.ip);
        j.at("global_ip").get_to(obj.global_ip);
        j.at("port").get_to(obj.port);
    }
};

} // End of comm namespace
} // End of cunqa namespace

