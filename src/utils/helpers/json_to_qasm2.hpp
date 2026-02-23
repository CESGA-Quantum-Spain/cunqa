#pragma once

#include <vector>

#include "utils/constants.hpp"
#include "utils/json.hpp"

namespace
{
using namespace cunqa;

// Used in the json_to_qasm2 function for printing correctly the matrices of custom unitary gates
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


inline std::string json_to_qasm2(const JSON& instructions, const JSON& config) 
{ 
    std::string qasm_circt = "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n";

    // Quantum and classical register declaration
    qasm_circt += "qreg q[" + to_string(config.at("num_qubits")) + "];";
    qasm_circt += "creg c[" + to_string(config.at("num_clbits")) + "];";

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
            case constants::S:
            case constants::SX:
            case constants::SY:
            case constants::SZ:
            case constants::SDAG:
            case constants::SXDAG:
            case constants::SYDAG:
            case constants::SZDAG:
            case constants::T:
            case constants::TDAG:
            case constants::P0:
            case constants::P1:
                qasm_circt += gate_name + " q["  + to_string(qubits[0]) + "];\n";
                break;
            // 1 Parametric 1 qubit gates
            case constants::U1:
            case constants::P:
            case constants::RX:
            case constants::RY:
            case constants::RZ:
            case constants::ROTINVX:
            case constants::ROTINVY:
            case constants::ROTINVZ:
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") q[" + to_string(qubits[0]) + "];\n";
                break;
            // 2 Parametric 1 qubit gates
                case constants::U2:
                case constants::R:
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + to_string(qubits[0]) + "];\n";
                break;
            // 3 Parametric 1 qubit gates
            case constants::U3:
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + to_string(qubits[0]) + "];\n";
                break;
            //UNITARY
            case constants::UNITARY:
                matrix = instruction.at("elements").get<std::vector<std::vector<std::vector<std::vector<double>>>>>();
                qasm_circt += gate_name + "(" + triple_vector_to_string(matrix[0]) + ") q[" + to_string(qubits[0]) + "];\n";
                break;
            // Non-parametric 2 qubit gates
            case constants::SWAP:
            case constants::CX:
            case constants::CY:
            case constants::CZ:
            case constants::CSX:
            case constants::CSY:
            case constants::CSZ:
            case constants::CT:
            case constants::ECR:
                qasm_circt += gate_name + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                break;
            // Parametric 2 qubit gates
            case constants::CU1:
            case constants::CP:
            case constants::CRX:
            case constants::CRY:
            case constants::CRZ:
            case constants::RXX:
            case constants::RYY:
            case constants::RZZ:
            case constants::RZX:
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ")" + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                break;
            // 3 Parametric 2 qubit gates
            case constants::CU2:
            case constants::CR:
            case constants::CU:
            case constants::CU3:
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "];\n";
                break;
            // Non-parametric 3 qubit gates
            case constants::CCX:
            case constants::CCY:
            case constants::CCZ:
            case constants::CECR:
            case constants::CSWAP:
                qasm_circt += gate_name + " q[" + to_string(qubits[0]) + "], q[" + to_string(qubits[1]) + "], q[" + to_string(qubits[2]) + "];\n";
                break;
            case constants::MEASURE:
                qasm_circt += "measure q[" + to_string(qubits[0]) + "] -> c[" + to_string(instruction.at("clbits")[0]) + "];\n";
                break;
            default:
                return "Instruction " + gate_name + " not supported";
        }
    } 
        
    return qasm_circt;
}

}