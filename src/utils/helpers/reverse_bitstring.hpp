#pragma once

#include <string>
#include <map>

#include "logger.hpp"

namespace cunqa {
inline std::string reverse_string(const std::string& bitstring) 
{
    std::string reverse_bitstring(bitstring.rbegin(), bitstring.rend()); 
    return reverse_bitstring;
}

inline void reverse_bitstring_keys_json(std::map<std::string, std::size_t>& counts) 
{
    std::map<std::string, std::size_t> modified_counts; 

    for (const auto& [key, inner] : counts) {
        std::string reverse_bitstring = reverse_string(key); 
        modified_counts[reverse_bitstring] = inner; 
    }
    counts.clear();
    counts = modified_counts;
}

inline void reverse_bitstring_keys_json(JSON& result) 
{
    JSON counts = result.at("counts").get<JSON>();
    JSON modified_counts; 

    for (const auto& [key, inner] : counts.items()) {
        std::string reverse_bitstring = reverse_string(key); 
        modified_counts[reverse_bitstring] = inner; 
    }
    result.at("counts") = modified_counts;
}

} //End namespace cunqa