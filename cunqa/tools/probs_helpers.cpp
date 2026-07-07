#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <vector>
#include <map>
#include <string>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <optional>
#include <stdexcept>

namespace py = pybind11;

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
        std::vector<double> new_probs(2 * short_num_qubits, 0.0);

        // Iterate through all bitstrings
        for (int base_ten_bitstring = 0; base_ten_bitstring < static_cast<int>(probs.size()); ++base_ten_bitstring) {
            double prob = probs[base_ten_bitstring];

            // For each qubit in the partial list
            for (int i = 0; i < short_num_qubits; ++i) {
                int i_qubit = partial[i];
                // Extract bit at position i_qubit using bitwise operation
                int zero_one = (base_ten_bitstring >> i_qubit) & 1;
                new_probs[i * 2 + zero_one] += prob;
            }
        }
        
        return new_probs;

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



PYBIND11_MODULE(probs_helpers, m) {
    m.doc() = "Functions for manipulating measurement counts and estimating probabilities";

    m.def("counts_to_probs",
        [](const std::map<std::string, int>& counts,
           bool per_qubit = false,
           const std::optional<std::vector<int>>& partial = std::nullopt) {
            auto result = countsToProbs(
                counts,
                per_qubit,
                partial.has_value() ? &partial.value() : nullptr
            );
            // Conversion to numpy array
            if (per_qubit) {
                // Reshape to (2, num_qubits) for per_qubit case
                int size = result.size() / 2;
                std::vector<size_t> shape = {static_cast<size_t>(size), 2};
                return py::array_t<double>(shape, result.data());
            } else {
                // Return as 1D array for partial case
                return py::array_t<double>(result.size(), result.data());
            }
        },
        py::arg("counts"),
        py::arg("per_qubit") = false,
        py::arg("partial") = py::none(),
        R"pbdoc(
            Convert quantum measurement counts to probabilities.
            
            Args:
                counts: Dictionary mapping bitstrings to measurement counts
                per_qubit: If True, marginalize to per-qubit probabilities
                partial: Optional list of qubit indices to marginalize over
                
            Returns:
                List of probabilities corresponding to bitstrings
        )pbdoc"
    );

    m.def("recombine_probs",
        [](const std::vector<double>& probs,
           bool per_qubit,
           const std::optional<std::vector<int>>& partial_ptr,
           int num_qubits) {
            auto result = recombineProbs(
                probs,
                per_qubit,
                partial_ptr.has_value() ? &partial_ptr.value() : nullptr,
                num_qubits
            );
            // Conversion to numpy array
            if (per_qubit) {
                // Reshape to (2, num_qubits) for per_qubit case
                int short_num_qubits = result.size() / 2;
                std::vector<size_t> shape = {static_cast<size_t>(short_num_qubits), 2};
                return py::array_t<double>(shape, result.data());
            } else {
                // Return as 1D array for partial case
                return py::array_t<double>(result.size(), result.data());
            }
        },
        py::arg("probs"),
        py::arg("per_qubit"),
        py::arg("partial") = py::none(),
        py::arg("num_qubits"),
        R"pbdoc(
            Recombine probabilities based on marginalization parameters.
            
            Args:
                probs: List of probabilities for all bitstrings
                per_qubit: If True, marginalize to per-qubit probabilities
                partial: Optional list of qubit indices to marginalize over
                num_qubits: Total number of qubits in the system
                
            Returns:
                List of recombined probabilities
        )pbdoc"
    );

    m.def("marginalize_counts",
        [](const std::map<std::string, int>& counts,
           const std::vector<int>& region_sizes,
           bool check_length = false) {
            return marginalizeCountsByRegions(counts, region_sizes, check_length);
        },
        py::arg("counts"),
        py::arg("region_sizes"),
        py::arg("check_length") = false,
        R"pbdoc(
            Marginalize counts by grouping qubits into regions.
            
            Divides bitstrings into regions of specified sizes and sums counts
            for matching reduced bitstrings across all regions.
            
            Args:
                counts: Dictionary mapping bitstrings to measurement counts
                region_sizes: List of region sizes (must sum to bitstring length)
                check_length: If True, validate that region sizes match bitstring length
                
            Returns:
                List of dictionaries, one per region, with marginalized counts
                
            Example:
                counts = {"010101": 112, "001101": 34, "111111": 1700}
                regions = marginalize_counts(counts, [2, 2, 2])
                # Returns 3 dicts: {"01": 112, "00": 34, "11": 1700}, 
                #                  {"01": 112, "11": 1734},
                #                  {"01": 146, "11": 1700}
        )pbdoc"
    );
}