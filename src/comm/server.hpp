#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

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
    std::string nodename;
    std::string endpoint;
    JSON device;

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
            {"nodename", obj.nodename}, 
            {"endpoint", obj.endpoint},
            {"device", obj.device}
        };
    }

    friend void from_json(const JSON& j, Server& obj) {
        j.at("mode").get_to(obj.mode);
        j.at("nodename").get_to(obj.nodename);
        j.at("endpoint").get_to(obj.endpoint);
        j.at("device").get_to(obj.device);
    }
};

} // End of comm namespace
} // End of cunqa namespace

