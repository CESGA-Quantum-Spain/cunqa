#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <utility>
#include <numeric>

#include "dynamic_circuit/instruction_type.hpp"
#include "sim/simulator.hpp"

namespace cunqa {

struct ClassicalCommManager {
    std::unordered_map<std::string, std::queue<bool>> sent_clbits;

    inline void send(const bool& value, const std::string& qpu_id)
    {
        sent_clbits[qpu_id].push(value);
    }

    inline bool recv(const std::string& qpu_id, bool& value)
    {
        if (sent_clbits.contains(qpu_id)) {
            value = sent_clbits[qpu_id].front();
            sent_clbits[qpu_id].pop();
            return true;
        } 
        return false;
    }

};        

}                  