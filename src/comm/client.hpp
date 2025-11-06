#pragma once

#include <iostream>
#include <fstream>
#include <string_view>
#include <memory>

namespace cunqa {
namespace comm {

template <typename T>
class FutureWrapper {
public: 
    FutureWrapper(T * client) : client_{client} {};
    
    inline std::string get() { return client_->recv_results(); };
    inline bool valid() { return true; };
private:
    T * client_;
};

class Client {
public:

    Client();
    ~Client();

    void connect(const std::string& ip, const std::string& port);
    FutureWrapper<Client> send_circuit(const std::string& circuit);
    FutureWrapper<Client> send_parameters(const std::string& parameters);
    std::string recv_results();
    void disconnect(const std::string& endpoint = "");

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // End of comm namespace
} // End of cunqa namespace