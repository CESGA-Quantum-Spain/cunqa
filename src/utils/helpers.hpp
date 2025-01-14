#pragma once
#include <iostream>
#include <bit>
#include <cstdint>
#include <vector>
#include <list>
#include <bitset>
#include <cmath>
#include <fstream> 
#include "constants.hpp"
#include <random>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Secure cast of size
template<typename TO, typename FROM>
TO legacy_size_cast(FROM value)
{
    static_assert(std::is_unsigned_v<FROM> && std::is_unsigned_v<TO>,
                  "Only unsigned types can be cast here!");
    TO result = value;
    return result;
}

void print_bits(uint8_t block) {
    for(int i = 7; i >= 0; --i) {
        std::cout << ((block >> i) & 1);
    }
    std::cout << "\n";
}

void print_bits(std::vector<uint8_t> bin_circ) {
    for(const auto& block: bin_circ) {
        for(int i = 7; i >= 0; --i) {
            std::cout << ((block >> i) & 1);
        }
    }
    std::cout << "\n";
}

// The input is a list of jsons in string format, each json corresponding to a gate/measure with all its properties.
std::vector<uint8_t> from_json_to_bin(json qc_json){

    std::string name;
    std::vector<uint8_t> circ_binaries;
    double p;
    uint8_t p_arr[8];

    int bit_counter = 0;
    for (const auto& gate: qc_json) {
        name = gate["name"];
        circ_binaries.push_back(GATE_NAMES[name]);
        switch (GATE_NAMES[name]) {
            case MEASURE: // MEASURE
                circ_binaries.push_back(gate["qubits"][0]);
                circ_binaries.push_back(gate["memory"][0]);
                break;
            case ID:
            case X:
            case Y:
            case Z:
            case H: // ONE GATE NO PARAM
                circ_binaries.push_back(gate["qubits"][0]);
                break;
            case RX:
            case RY:
            case RZ: // ONE GATE PARAM
                circ_binaries.push_back(gate["qubits"][0]);
                p = gate["params"][0];

                std::memcpy(p_arr, &p, sizeof(p));
                circ_binaries.insert(circ_binaries.end(), std::begin(p_arr), std::end(p_arr));

                break;
            case CX:
            case CY:
            case CZ: // TWO GATE NO PARAM
                circ_binaries.push_back(gate["qubits"][0]);
                circ_binaries.push_back(gate["qubits"][1]);
                break;
            default:
                std::cerr << "Unsupported gate: " << name << "\n";
                return std::vector<uint8_t>();
        }
    }

    return circ_binaries;
}

std::vector<json> from_bin_to_json(const std::vector<uint8_t>& circ_binaries){
    std::vector<json> circ_json;
    json aux_json;
    double p;

    int i = 0;
    while (i < circ_binaries.size()) {
        aux_json["name"] = INVERTED_GATE_NAMES[circ_binaries[i]];
        switch (circ_binaries[i]) {
            case MEASURE: // MEASURE
                aux_json["qubits"] = {circ_binaries[i+1]};
                aux_json["memory"] = {circ_binaries[i+2]};
                i += 3;
                break;
            case ID:
            case X:
            case Y:
            case Z:
            case H: // ONE GATE NO PARAM
                aux_json["qubits"] = {circ_binaries[i+1]};
                i += 2;
                break;
            case RX:
            case RY:
            case RZ: // ONE GATE PARAM
                aux_json["qubits"] = {circ_binaries[i+1]};
                
                std::memcpy(&p, &circ_binaries[i+2], sizeof(double));
                aux_json["param"] = {p};
                i += 10;
                break;
            case CX:
            case CY:
            case CZ: // TWO GATE NO PARAM
                aux_json["qubits"] = {circ_binaries[i+1], circ_binaries[i+2]};
                i += 3;
                break;
            default:
                std::cerr << "Uncorrect bit sequence\n";
                return std::vector<json>();
        }

        circ_json.push_back(aux_json);
        aux_json = {};
    }

    return circ_json;

} 



void generate_random_circuit(unsigned long size, const std::string& filename) {
    
    std::vector<int> keys;
    for(auto gates : GATE_NAMES) {
        keys.push_back(gates.second);
    }

    // Abrir el archivo en modo escritura
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo para escribir." << std::endl;
        return;
    }

    // Inicializar el generador de números aleatorios
    std::random_device rd;
    
    std::mt19937 gen_gates(rd());
    std::uniform_int_distribution<> dist_gates(0, GATE_NAMES.size() - 1);

    std::mt19937 gen_qubits(rd());
    std::uniform_int_distribution<> dist_qubits(0, 31);

    std::mt19937 gen_double(rd()); // Generador Mersenne Twister
    std::uniform_real_distribution<> dist_double(-5.0, 5.0);

    json gate_json = {};

    // Escribir `count` nombres aleatorios en el archivo
    file << "[";

    std::cout << size << "\n";
    for (unsigned long i = 0; i < size; ++i) {
        auto gate = keys[dist_gates(gen_gates)];
        gate_json["name"] = INVERTED_GATE_NAMES[gate];
        switch (gate) {
            case MEASURE: // MEASURE
                gate_json["qubits"] = {dist_qubits(gen_qubits)};
                gate_json["memory"] = {dist_qubits(gen_qubits)};
                break;
            case ID:
            case X:
            case Y:
            case Z:
            case H: // ONE GATE NO PARAM
                gate_json["qubits"] = {dist_qubits(gen_qubits)};
                break;
            case RX:
            case RY:
            case RZ: // ONE GATE PARAM
                gate_json["qubits"] = {dist_qubits(gen_qubits)};
                gate_json["params"] = {dist_double(gen_double)};
                break;
            case CX:
            case CY:
            case CZ: // TWO GATE NO PARAM
                gate_json["qubits"] = {dist_qubits(gen_qubits), dist_qubits(gen_qubits)};
                break;
            default:
                std::cerr << "Uncorrect bit sequence\n";
        }
        file << gate_json.dump() << ", ";
        gate_json = {};
    }
    file.seekp(-2, std::ios::end);
    file << "]";

    file.close();
}












