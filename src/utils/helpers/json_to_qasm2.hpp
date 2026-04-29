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

        switch (INSTRUCTIONS_MAP.at(gate_name))
        {   
            // Non-parametric 1 qubit gates
            case InstructionType::ID:
            case InstructionType::X:
            case InstructionType::Y:
            case InstructionType::Z:
            case InstructionType::H:
            case InstructionType::S:
            case InstructionType::SX:
            case InstructionType::SY:
            case InstructionType::SZ:
            case InstructionType::SDG:
            case InstructionType::SXDG:
            case InstructionType::SYDG:
            case InstructionType::SZDG:
            case InstructionType::T:
            case InstructionType::TDG:
            case InstructionType::P0:
            case InstructionType::P1:
            case InstructionType::V:
            case InstructionType::VDG:
            case InstructionType::K:
                qasm_circt += gate_name + " q["  + std::to_string(qubits[0]) + "];\n";
                break;
            // 1 Parametric 1 qubit gates
            case InstructionType::U1:
            case InstructionType::GLOBALP:
            case InstructionType::P:
            case InstructionType::RX:
            case InstructionType::RY:
            case InstructionType::RZ:
            case InstructionType::ROTINVX:
            case InstructionType::ROTINVY:
            case InstructionType::ROTINVZ:
            case InstructionType::ROTX:
            case InstructionType::ROTY:
            case InstructionType::ROTZ:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 2 Parametric 1 qubit gates
                case InstructionType::U2:
                case InstructionType::R:
                {
                    params = instruction.at("params").get<std::vector<double>>();
                    qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                    break;
                }
            // 3 Parametric 1 qubit gates
            case InstructionType::U3: 
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 4 Parametric 1 qubit gates
            case InstructionType::U:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) +", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            //UNITARY
            case InstructionType::UNITARY:
            {
                matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<std::vector<double>>>>>();
                qasm_circt += gate_name + "(" + triple_vector_to_string(matrix[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // Non-parametric 2 qubit gates
            case InstructionType::SWAP:
            case InstructionType::ISWAP:
            case InstructionType::CX:
            case InstructionType::CY:
            case InstructionType::CZ:
            case InstructionType::CSX:
            case InstructionType::CSXDG:
            case InstructionType::CS:
            case InstructionType::CSDG:
            case InstructionType::CT:
            case InstructionType::DCX:
            case InstructionType::ECR:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            // Parametric 2 qubit gates
            case InstructionType::CU1:
            case InstructionType::CP:
            case InstructionType::CRX:
            case InstructionType::CRY:
            case InstructionType::CRZ:
            case InstructionType::RXX:
            case InstructionType::RYY:
            case InstructionType::RZZ:
            case InstructionType::RZX:
            case InstructionType::XXMYY:
            case InstructionType::XXPYY:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 2 Parametric 2 qubit gates
            case InstructionType::CU2:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 3 Parametric 2 qubit gates
            case InstructionType::CU3:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 4 Parametric 2 qubit gates
            case InstructionType::CU:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // Non-parametric 3 qubit gates
            case InstructionType::CCX:
            case InstructionType::CCY:
            case InstructionType::CCZ:
            case InstructionType::CECR:
            case InstructionType::CSWAP:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "], q[" + std::to_string(qubits[2]) + "];\n";
                break;
            // Non-parametric 1 qubit multicontroled
            case InstructionType::MCX:
            case InstructionType::MCY:
            case InstructionType::MCZ:
            case InstructionType::MCSX:
            {
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 1 parametric 1 qubit multicontroled
            case InstructionType::MCRX:
            case InstructionType::MCRY:
            case InstructionType::MCRZ:
            case InstructionType::MCP:
            case InstructionType::MCU1:
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
            case InstructionType::MCU2:
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
            case InstructionType::MCU3:
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
            case InstructionType::MCU:
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
            case InstructionType::MCSWAP:
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
            case InstructionType::MEASURE:
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