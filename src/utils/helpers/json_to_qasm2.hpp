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
    qasm_circt += "qreg q[" + std::to_string(config.at("num_qubits").get<int>()) + "];";
    qasm_circt += "creg c[" + std::to_string(config.at("num_clbits").get<int>()) + "];\n";

    // Instruction processing
    for (const auto& instruction : instructions) {
        std::string gate_name = instruction.at("name");
        auto qubits = instruction.at("qubits").get<std::vector<int>>();
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
            case constants::SDG:
            case constants::SXDG:
            case constants::SYDG:
            case constants::SZDG:
            case constants::T:
            case constants::TDG:
            case constants::P0:
            case constants::P1:
            case constants::V:
            case constants::VDG:
            case constants::K:
                qasm_circt += gate_name + " q["  + std::to_string(qubits[0]) + "];\n";
                break;
            // 1 Parametric 1 qubit gates
            case constants::U1:
            case constants::GLOBALP:
            case constants::P:
            case constants::RX:
            case constants::RY:
            case constants::RZ:
            case constants::ROTINVX:
            case constants::ROTINVY:
            case constants::ROTINVZ:
            case constants::ROTX:
            case constants::ROTY:
            case constants::ROTZ:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 2 Parametric 1 qubit gates
                case constants::U2:
                case constants::R:
                {
                    params = instruction.at("params").get<std::vector<double>>();
                    qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                    break;
                }
            // 3 Parametric 1 qubit gates
            case constants::U3: 
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 4 Parametric 1 qubit gates
            case constants::U:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) +", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            //UNITARY
            case constants::UNITARY:
            {
                matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<std::vector<double>>>>>();
                qasm_circt += gate_name + "(" + triple_vector_to_string(matrix[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // Non-parametric 2 qubit gates
            case constants::SWAP:
            case constants::ISWAP:
            case constants::CX:
            case constants::CY:
            case constants::CZ:
            case constants::CSX:
            case constants::CSXDG:
            case constants::CSY:
            case constants::CSZ:
            case constants::CS:
            case constants::CSDG:
            case constants::CT:
            case constants::DCX:
            case constants::ECR:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
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
            case constants::XXMYY:
            case constants::XXPYY:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 2 Parametric 2 qubit gates
            case constants::CU2:
            case constants::CR:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 3 Parametric 2 qubit gates
            case constants::CU3:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 4 Parametric 2 qubit gates
            case constants::CU:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // Non-parametric 3 qubit gates
            case constants::CCX:
            case constants::CCY:
            case constants::CCZ:
            case constants::CECR:
            case constants::CSWAP:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "], q[" + std::to_string(qubits[2]) + "];\n";
                break;
            // Non-parametric 1 qubit multicontroled
            case constants::MCX:
            case constants::MCY:
            case constants::MCZ:
            case constants::MCSX:
            {
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 1 parametric 1 qubit multicontroled
            case constants::MCRX:
            case constants::MCRY:
            case constants::MCRZ:
            case constants::MCP:
            case constants::MCU1:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") ";
                for (size_t i = 0; i < qubits.size(); ++i) {
                    qasm_circt += "q[" + std::to_string(qubits[i]) + "]";
                    if (i != qubits.size() - 1) {
                        qasm_circt += ", ";
                    }
                }
                qasm_circt += ";\n";
                break;
            }
            // 2 parametric 1 qubit multicontroled
            case constants::MCU2:
            case constants::MCR:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ") ";
                for (size_t i = 0; i < qubits.size(); ++i) {
                    qasm_circt += "q[" + std::to_string(qubits[i]) + "]";
                    if (i != qubits.size() - 1) {
                        qasm_circt += ", ";
                    }
                }
                qasm_circt += ";\n";
                break;
            }
            // 3 parametric 1 qubit multicontroled
            case constants::MCU3:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) +", " + std::to_string(params[3]) + ") ";
                for (size_t i = 0; i < qubits.size(); ++i) {
                    qasm_circt += "q[" + std::to_string(qubits[i]) + "]";
                    if (i != qubits.size() - 1) {
                        qasm_circt += ", ";
                    }
                }
                qasm_circt += ";\n";
                break;
            }
            // 4 parametric 1 qubit multicontroled
            case constants::MCU:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) +", " + std::to_string(params[3]) + ") ";
                for (size_t i = 0; i < qubits.size(); ++i) {
                    qasm_circt += "q[" + std::to_string(qubits[i]) + "]";
                    if (i != qubits.size() - 1) {
                        qasm_circt += ", ";
                    }
                }
                qasm_circt += ";\n";
                break;
            }
            // Non-parametric 2 qubit multicontroled
            case constants::MCSWAP:
            {
                qasm_circt += gate_name + " ";
                for (size_t i = 0; i < qubits.size(); ++i) {
                    qasm_circt += "q[" + std::to_string(qubits[i]) + "]";
                    if (i != qubits.size() - 1) {
                        qasm_circt += ", ";
                    }
                }
                qasm_circt += ";\n";
                break;
            }
            case constants::MEASURE:
            {
                auto clbits = instruction.at("clbits").get<std::vector<int>>();
                qasm_circt += "measure q[" + std::to_string(qubits[0]) + "] -> c[" + std::to_string(clbits[0]) + "];\n";
                break;
            }
            default:
                return "Instruction " + gate_name + " not supported";
        }
    } 
        
    return qasm_circt;
}

}