#include <vector>
#include <map>
#include <string>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>

// Given a counts dictionary, marginalize counts on the selected regions. Example:
// {"1100": 501, "0100": 499} with region_sizes = [2 2] woudl result in 
// {"11": 501, "01": 499} and {"00": 1000}
// multiple registers bitstrings, eg "00 10", are also processed correctly, as spaces are ignored
std::vector<std::map<std::string, int>> marginalizeCountsByRegions(
    const std::map<std::string, int>& counts,
    const std::vector<int>& region_sizes, 
    const bool check_lenght = false
) {
    if (check_lenght){
        // Validate that region sizes sum to the bitstring length
    int total_length = 0;
    for (int size : region_sizes) {
        total_length += size;
    }
    
    // Get the bitstring length (removing spaces)
    int bitstring_length = 0;
    if (!counts.empty()) {
        const std::string& first_key = counts.begin()->first;
        for (char c : first_key) {
            if (!std::isspace(c)) {
                bitstring_length++;
            }
        }
    }
    
    if (bitstring_length != total_length) {
        throw std::invalid_argument(
            "Region sizes do not sum to bitstring length"
        );
    }
    }
    
    // Create a result vector with one map per region
    std::vector<std::map<std::string, int>> results(region_sizes.size());
    
    // Process each bitstring and its count
    for (const auto& [key, count] : counts) {
        // Remove spaces from the key
        std::string clean_key;
        for (char c : key) {
            if (!std::isspace(c)) {
                clean_key += c;
            }
        }
        
        // Extract regions and update results
        int pos = 0;
        for (size_t region_idx = 0; region_idx < region_sizes.size(); ++region_idx) {
            int region_size = region_sizes[region_idx];
            std::string region = clean_key.substr(pos, region_size);
            results[region_idx][region] += count;
            pos += region_size;
        }
    }
    
    return results;
}

std::vector<double> recombineProbs(
    const std::vector<double>& probs,
    bool per_qubit,
    const std::vector<int>* partial_ptr,
    int num_qubits
) {
    // Set up partial indices (reversed for big-endian)
    std::vector<int> partial;
    if (partial_ptr == nullptr) {
        for (int i = 0; i < num_qubits; ++i) {
            partial.push_back(num_qubits - 1 - i);
        }
    } else {
        for (int idx : *partial_ptr) {
            partial.push_back(num_qubits - 1 - idx);
        }
    }

    if (per_qubit) {
        int short_num_qubits = partial.size();
        std::vector<std::vector<double>> new_probs(short_num_qubits, std::vector<double>(2, 0.0));

        // Iterate through all bitstrings
        for (int base_ten_bitstring = 0; base_ten_bitstring < static_cast<int>(probs.size()); ++base_ten_bitstring) {
            double prob = probs[base_ten_bitstring];

            // For each qubit in the partial list
            for (int i = 0; i < short_num_qubits; ++i) {
                int i_qubit = partial[i];
                // Extract bit at position i_qubit using bitwise operation
                int zero_one = (base_ten_bitstring >> i_qubit) & 1;
                new_probs[i][zero_one] += prob;
            }
        }

        // Flatten result for return
        std::vector<double> result;
        for (const auto& row : new_probs) {
            result.insert(result.end(), row.begin(), row.end());
        }
        return result;

    } else {
        // Probabilities of partial bitstrings
        int short_num_qubits = partial.size();
        int num_short_bitstrings = 1 << short_num_qubits;
        std::vector<double> short_bitstring_probs(num_short_bitstrings, 0.0);

        // Iterate through all bitstrings
        for (int base_ten_bitstring = 0; base_ten_bitstring < static_cast<int>(probs.size()); ++base_ten_bitstring) {
            double prob = probs[base_ten_bitstring];

            // Extract the partial bitstring using bitwise operations
            int shortened_bitstring = 0;
            for (int i = 0; i < short_num_qubits; ++i) {
                int i_qubit = partial[i];
                int bit = (base_ten_bitstring >> i_qubit) & 1;
                shortened_bitstring |= (bit << i);
            }

            short_bitstring_probs[shortened_bitstring] += prob;
        }

        return short_bitstring_probs;
    }
}

std::vector<double> countsToProbs(
    const std::map<std::string, int>& counts,
    bool per_qubit = false,
    const std::vector<int>* partial = nullptr
) {
    // Get number of qubits from first key
    int num_qubits = counts.begin()->first.length();
    int num_bitstrings = 1 << num_qubits; // 2^num_qubits

    // Convert string-based counts to integer-based for faster access
    std::vector<int> count_array(num_bitstrings, 0);
    for (const auto& [key, value] : counts) {
        int bitstring_int = std::stoi(key, nullptr, 2);
        count_array[bitstring_int] = value;
    }

    // Calculate total shots
    unsigned int all_shots = 0;
    for (int i = 0; i < num_bitstrings; ++i) {
        all_shots += count_array[i];
    }

    // Calculate probabilities
    std::vector<double> probs(num_bitstrings);
    for (int i = 0; i < num_bitstrings; ++i) {
        probs[i] = static_cast<double>(count_array[i]) / all_shots;
    }

    // Apply recombination if needed
    if (per_qubit || partial != nullptr) {
        probs = recombineProbs(probs, per_qubit, partial, num_qubits);
    }

    return probs;
}
