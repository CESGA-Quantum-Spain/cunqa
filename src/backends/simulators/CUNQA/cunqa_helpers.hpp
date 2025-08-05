#pragma once

#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include "utils/constants.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {

// Convert CunqaSim's counts to standard format, ie pass counts keys from decimal to binary and then reverse the bit string
inline JSON convert_standard_results_cunqa(const std::unordered_map<int, int>& res, const int& num_qubits) 
{
    JSON modified_res;
    for (const auto& [key, value] : res) {
        modified_res[std::to_string(key)] = value;
    } 
    
    std::vector<std::string> keys_to_erase; 
    for (const auto& [key, value] : res) {
        std::bitset<64> binary_key(key); // 64 is the maximun size of bitset. I need to give a const that is known at compile time so i choose this one
        std::string binary_string = binary_key.to_string();

        std::string trunc_bitstring(binary_string.rbegin(), binary_string.rbegin() + num_qubits); // Drop the padding by truncating to the last characters
        
        modified_res.emplace(trunc_bitstring, value); 
        keys_to_erase.push_back(std::to_string(key)); 
    }

    // Erase the keys after the iteration
    for (const auto& key : keys_to_erase) {
        modified_res.erase(key);
    }

    return modified_res; 
}
}//End of cunqa namespace