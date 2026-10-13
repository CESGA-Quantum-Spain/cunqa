#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <utility>

#include "utils/json.hpp"

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

    void send_measure(const bool& measurement, const std::string& target);
    bool recv_measure(const std::string& origin);

    // Same as above, but the message also carries the simulated time at which
    // it was sent. The timestamp is empty if the sender has no simulated clock.
    void send_measure(const bool& measurement, double send_time, const std::string& target);
    std::pair<bool, std::optional<double>> recv_timed_measure(const std::string& origin);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    JSON communications;
    std::string qpu_id;
};  

} // End of comm namespace
} // End of cunqa namespace