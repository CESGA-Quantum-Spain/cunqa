#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <utility>
#include <tuple>
#include <numeric>

#include "dynamic_circuit/instruction_type.hpp"
#include "sim/simulator.hpp"

namespace cunqa {

struct ClassicalCommManager {
    // Each sent clbit keeps the simulated time at which it was sent.
    std::unordered_map<std::string, std::queue<std::pair<bool, double>>> sent_clbits;

    inline void send(const bool& value, const std::string& qpu_id, double send_time = 0.0)
    {
        sent_clbits[qpu_id].push({value, send_time});
    }

    inline bool recv(const std::string& qpu_id, bool& value, double& send_time)
    {
        if (sent_clbits.contains(qpu_id) && !sent_clbits.at(qpu_id).empty()) {
            std::tie(value, send_time) = sent_clbits[qpu_id].front();
            sent_clbits[qpu_id].pop();
            return true;
        } 
        return false;
    }

};        

}                  