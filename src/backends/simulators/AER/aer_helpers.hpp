#pragma once

#include <regex>
#include <string>
#include <bitset>
#include <chrono>
#include <vector>

#include "logger.hpp"

using namespace std::string_literals;
using namespace AER;


namespace {

const std::vector<std::string> AER_CONFIG_KEYS = {
    "shots",
    "method",
    "precision",
    "enable_truncation",
    "zero_threshold",
    "validation_threshold",
    "max_parallel_threads",
    "max_parallel_experiments",
    "max_parallel_shots",
    "fusion_enable",
    "fusion_verbose",
    "fusion_max_qubit",
    "fusion_threshold",
    "accept_distributed_results",
    "memory",
    "cuStateVec_enable",
    "blocking_qubits",
    "blocking_enable",
    "chunk_swap_buffer_qubits",
    "batched_shots_gpu",
    "batched_shots_gpu_max_qubits",
    "num_threads_per_device",
    "shot_branching_enable",
    "shot_branching_sampling_enable",
    "statevector_parallel_threshold",
    "statevector_sample_measure_opt",
    "stabilizer_max_snapshot_probabilities",
    "extended_stabilizer_sampling_method",
    "extended_stabilizer_metropolis_mixing_time",
    "extended_stabilizer_approximation_error",
    "extended_stabilizer_norm_estimation_samples",
    "extended_stabilizer_norm_estimation_repetitions",
    "extended_stabilizer_parallel_threshold",
    "extended_stabilizer_probabilities_snapshot_samples",
    "matrix_product_state_truncation_threshold",
    "matrix_product_state_max_bond_dimension",
    "mps_sample_measure_algorithm",
    "mps_log_data",
    "mps_swap_direction",
    "chop_threshold",
    "mps_parallel_threshold",
    "mps_omp_threads",
    "mps_lapack",
    "tensor_network_num_sampling_qubits",
    "use_cuTensorNet_autotuning",
    "parameterizations",
    "library_dir",
    "global_phase",
    "_parallel_experiments",
    "_parallel_shots",
    "_parallel_state_update",
    "fusion_allow_kraus",
    "fusion_allow_superop",
    "fusion_parallelization_threshold",
    "_fusion_enable_n_qubits",
    "_fusion_enable_n_qubits_1",
    "_fusion_enable_n_qubits_2",
    "_fusion_enable_n_qubits_3",
    "_fusion_enable_n_qubits_4",
    "_fusion_enable_n_qubits_5",
    "_fusion_enable_diagonal",
    "_fusion_min_qubit",
    "fusion_cost_factor",
    "superoperator_parallel_threshold",
    "unitary_parallel_threshold",
    "memory_blocking_bits",
    "extended_stabilizer_norm_estimation_default_samples",
    "runtime_parameter_bind_enable",
};

}  // End namespace

namespace cunqa {
namespace sim {

JSON quantum_task_to_AER(QuantumTask& quantum_task)
{
    JSON new_config;
    // Generic Aer configuration options
    for (auto& [key, value] : quantum_task.config.items()) {
        if (std::find(AER_CONFIG_KEYS.begin(), AER_CONFIG_KEYS.end(), key) != AER_CONFIG_KEYS.end()) {
            new_config[std::string(key)] = value;
        }
    }

    // CUNQA Aer configuration options
    
    // Seed
    if (quantum_task.config.contains("seed")) {
        new_config["seed_simulator"] = quantum_task.config.at("seed");
    }

    // Device (CPU or GPU)
    std::string device = quantum_task.config.at("device")["device_name"];
    new_config["device"] = device;

    // target_gpus (empty list if device == CPU)
    std::vector<int> target_gpus = (device == "GPU") ? quantum_task.config.at("device")["target_devices"].get<std::vector<int>>() : std::vector<int>();
    new_config["target_gpus"] = target_gpus;

    // memory_slots = num_clbits
    int mem_slots = quantum_task.config.at("num_clbits").get<int>();
    new_config["memory_slots"] = mem_slots;

    // Avoid parallelization. Not recommended.
    if (quantum_task.config.contains("avoid_parallelization")) {
        if (quantum_task.config.at("avoid_parallelization").get<bool>()) {
            LOGGER_DEBUG("Trhead parallelization canceled");
            new_config["max_parallel_threads"] = 1;
        }
    }

    //JSON Object because if not it generates an array
    for (auto& instruction : quantum_task.circuit) {
        instruction = JSON::parse(std::regex_replace(instruction.dump(), std::regex("clbits"), "memory"));
    }
    JSON new_circuit = {
        {"config", new_config},
        {"instructions", quantum_task.circuit}
    };

    return new_circuit;
}


void convert_standard_results_Aer(JSON& res, const int& num_clbits) 
{
    JSON counts = res.at("results")[0].at("data").at("counts").get<JSON>();
    JSON modified_counts;

    for (const auto& [key, inner] : counts.items()) {
        // Remove "0x" prefix if present
        std::string hex_key = key;
        if (hex_key.rfind("0x", 0) == 0) {
            hex_key = hex_key.substr(2);
        }

        // Convert hex string to unsigned long long (support up to 100 bits)
        // Use std::bitset<100> for binary conversion
        std::bitset<100> bits(0);
        size_t hex_len = hex_key.length();
        // Convert hex to binary manually
        for (size_t i = 0; i < hex_len; ++i) {
            char c = hex_key[hex_len - 1 - i];
            int value = 0;
            if (c >= '0' && c <= '9') value = c - '0';
            else if (c >= 'a' && c <= 'f') value = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') value = 10 + (c - 'A');
            for (int j = 0; j < 4; ++j) {
                if ((value >> j) & 1) {
                    size_t bit_pos = i * 4 + j;
                    if (bit_pos < 100) bits.set(bit_pos);
                }
            }
        }

        // Get binary string with num_clbits bits, reversed to match Qiskit/AER convention
        std::string binary_string;
        for (int i = num_clbits - 1; i >= 0; --i) {
            binary_string += bits[i] ? '1' : '0';
        }

        modified_counts[binary_string] = inner; 
    }

    res.at("results")[0].at("data").at("counts") = modified_counts;
}


inline void convert_cunqadiagonal_to_aerdiagonal(const CUNQADiagonalMatrix& cunqa_diagonal, cvector_t& aer_diagonal)
{
    for (auto& z : cunqa_diagonal) {
        std::complex<double> complex = z[0] + std::complex<double>(0, z[1]);
        aer_diagonal.push_back(complex);
    }
}

inline void convert_cunqa_matrix_to_complex_vector(const CUNQAMatrix& cunqa_matrix, CUNQAComplexVector& matrix_data)
{
    for (auto& row : cunqa_matrix) {
        for (auto& z : row) {
            std::complex<double> complex = z[0] + std::complex<double>(0, z[1]);
            matrix_data.push_back(complex);
        }
    }
}

} // End of sim namespace
} // End of cunqa namespace