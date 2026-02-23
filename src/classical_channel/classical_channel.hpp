#pragma once

#include <string>
#include <vector>
#include <memory>

#include <utils/json.hpp>

namespace cunqa {
namespace comm {

class ClassicalChannel {
public:
    std::string endpoint;

    ClassicalChannel(const std::string& qpu_id);
    ~ClassicalChannel();

    void publish();
    void connect(const std::string& qpu_id);
    void send_info(const std::string& data, const std::string& target);
    std::string recv_info(const std::string& origin);

    void send_measure(const int& measurement, const std::string& target);
    int recv_measure(const std::string& origin);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    JSON communications;
    std::string qpu_id;
};  

} // End of comm namespace
} // End of cunqa namespace