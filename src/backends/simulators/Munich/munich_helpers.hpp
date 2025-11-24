#pragma once

#include <iostream>
#include <vector>
#include "utils/constants.hpp"
#include "quantum_task.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {

// Used in the quantum_task_to_Munich function for printing correctly the matrices of custom unitary gates
inline std::string triple_vector_to_string(const std::vector<std::vector<std::vector<double>>>& data) {
    std::ostringstream oss;
    oss << "[";

    for (size_t i = 0; i < data.size(); ++i) {
        oss << "[";
        for (size_t j = 0; j < data[i].size(); ++j) {
            oss << "[";
            for (size_t k = 0; k < data[i][j].size(); ++k) {
                oss << data[i][j][k];
                if (k != data[i][j].size() - 1) oss << ", ";
            }
            oss << "]";
            if (j != data[i].size() - 1) oss << ", ";
        }
        oss << "]";
        if (i != data.size() - 1) oss << ", ";
    }

    oss << "]";
    return oss.str();
}


// Transfroms a quantum_task, which has config and circuit (these are the instructions) to an OpenQASM2 string
inline std::string quantum_task_to_Munich(const QuantumTask& quantum_task) 
{ 
    std::vector<JSON> instructions = quantum_task.circuit;
    JSON config_json = quantum_task.config;
    std::string qasm_circt = "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n";

    try {
        // Quantum and classical register declaration
        qasm_circt += "qreg q[" + to_string(config_json.at("num_qubits")) + "];";
        qasm_circt += "creg c[" + to_string(config_json.at("num_clbits")) + "];";

        // Instruction processing
        for (const auto& instruction : instructions) {
            std::string gate_name = instruction.at("name");
            auto qubits = instruction.at("qubits");
            std::vector<double> params;
            std::vector<std::vector<std::vector<std::vector<double>>>> matrix;

            switch (constants::INSTRUCTIONS_MAP.at(gate_name))
            {   
                // Non-parametric 1 qubit gates
                case constants::ID:
                case constants::X:
                case constants::Y:
                case constants::Z:
                case constants::H:
                case constants::SX:
                //case constants::S:
                //case constants::SDG:
                //case constants::SXDG:
                //case constants::T:
                //case constants::TDG:
                    qasm_circt += gate_name + " q["  + to_string(qubits[0]) + "];\n";
                    break;
                // Parametric 1 qubit gates
                //case constants::U1:
                case constants::RX:
                case constants::RY:
                case constants::RZ:
                    params = instruction.at("params").get<std::vector<double>>();
                    qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") q[" + to_string(qubits[0]) + "];\n";
                    break;
                //UNITARY
                case constants::UNITARY:
                    matrix = instruction.at("params").get<std::vector<std::vector<std::vector<std::vector<double>>>>>();
                    qasm_circt += gate_name + "(" + triple_vector_to_string(matrix[0]) + ") q[" + to_string(qubits[0]) + "];\n";
                    break;
                // 2-Parametric 1 qubit gates
                //case constants::U2:
                //case constants::R:
                //    qasm_circt += gate_name + "(" + to_string(params[0]) + ", " + to_string(params[1]) + ") q[" + to_string(qubits[0]) + "];\n";
                //    break;
                // 3-Parametric 1 qubit gate
                //case constants::U3:
                //    qasm_circt += gate_name + "(" + to_string(params[0]) + ", " + to_string(params[1]) + ", " + to_string(params[2]) + ") q[" + to_string(qubits[0]) + "];\n";
                //    break;
                // Non-parametric 2 qubit gates
                case constants::CX:
                case constants::CY:
                case constants::CZ:
                //case constants::CSX:
                //case constants::SWAP:
                case constants::ECR:
                    qasm_circt += gate_name + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                    break;
                // Parametric 2 qubit gates
                //case constants::RXX:
                case constants::CRX:
                case constants::CRY:
                case constants::CRZ:
                    params = instruction.at("params").get<std::vector<double>>();
                    qasm_circt += gate_name + "(" + std::to_string(params[0]) + ")" + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                    break;
                // Non-parametric  3 qubit gates
                //case constants::CCX:
                //case constants::CSWAP:
                //    qasm_circt += gate_name + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "], q[" + to_string(qubits[2]) + "];\n";
                //    break;
                // Measure, duh
                case constants::SWAP:
                    qasm_circt += gate_name + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                    break;
                case constants::MEASURE:
                    qasm_circt += "measure q[" + to_string(qubits[0]) + "] -> c[" + to_string(instruction.at("clbits")[0]) + "];\n";
                    break;
                default:
                    LOGGER_ERROR("Error. Invalid gate name: {}", gate_name);
                    throw std::runtime_error("QASM format is not correct"); 
                    break;
            }
        } 
    } catch (const std::exception& e) {
        LOGGER_ERROR("Error translating a gate from JSON to QASM2.");
        throw;
    } 
        
    return qasm_circt;
}

} // End of cunqa namespace
