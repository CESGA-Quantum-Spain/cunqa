#pragma once

#include <regex>
#include <string>
#include <bitset>
#include <chrono>

#include "utils/helpers/reverse_bitstring.hpp"

using namespace std::string_literals;
using namespace AER;

namespace cunqa {
namespace sim {

QuantumTask quantum_task_to_AER(const QuantumTask& quantum_task)
{
    JSON new_config = {
        {"method", quantum_task.config.at("method")},
        {"shots", quantum_task.config.at("shots")},
        {"memory_slots", quantum_task.config.at("num_clbits")}
        // TODO: Tune in the different options of the AER simulator
    };

    //JSON Object because if not it generates an array
    JSON new_circuit = {
        {"config", new_config},
        {"instructions", JSON::parse(std::regex_replace(quantum_task.circuit.dump(),
                       std::regex("clbits"), "memory"))}
    };

    return QuantumTask(new_circuit, new_config);
}


void convert_standard_results_Aer(JSON& res, const int& num_clbits) 
{
    JSON counts = res.at("results")[0].at("data").at("counts").get<JSON>();
    JSON modified_counts;
    std::vector<std::string> keys_to_erase; 

    for (const auto& [key, inner] : counts.items()) {
        int decimalValue = std::stoi(key, nullptr, 16);
        std::bitset<64> binary_key(decimalValue); // 64 is the maximun size of bitset. I need to give a const that is known at compile time so i choose this one
        std::string binary_string = binary_key.to_string();
        std::string trunc_bitstring(binary_string.rbegin(), binary_string.rbegin() + num_clbits); // Truncate out any unwanted zeros coming from the first hex character
        std::string reversed_counts = reverse_string(trunc_bitstring);

        modified_counts[reversed_counts] = inner; 
    }

    res.at("results")[0].at("data").at("counts") = modified_counts;
}

} // End of sim namespace
} // End of cunqa namespace