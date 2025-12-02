#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cunqa {
namespace comm {

class ClassicalChannel {
public:
    std::string endpoint;

    ClassicalChannel();
    ClassicalChannel(const std::string& id);
    ~ClassicalChannel();

    void publish(const std::string& suffix = "");

    void connect(const std::string& endpoint, const std::string& id = "");
    void connect(const std::string& endpoint, const bool force_endpoint);
    void connect(const std::vector<std::string>& endpoints, const bool force_endpoint);

    void send_info(const std::string& data, const std::string& target);
    std::string recv_info(const std::string& origin);

    void send_measure(const int& measurement, const std::string& target);
    int recv_measure(const std::string& origin);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};  

} // End of comm namespace
} // End of cunqa namespace