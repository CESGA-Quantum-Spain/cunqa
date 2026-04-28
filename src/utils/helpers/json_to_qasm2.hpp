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
            case InstructionTag::ID:
            case InstructionTag::X:
            case InstructionTag::Y:
            case InstructionTag::Z:
            case InstructionTag::H:
            case InstructionTag::S:
            case InstructionTag::SX:
            case InstructionTag::SY:
            case InstructionTag::SZ:
            case InstructionTag::SDG:
            case InstructionTag::SXDG:
            case InstructionTag::SYDG:
            case InstructionTag::SZDG:
            case InstructionTag::T:
            case InstructionTag::TDG:
            case InstructionTag::P0:
            case InstructionTag::P1:
            case InstructionTag::V:
            case InstructionTag::VDG:
            case InstructionTag::K:
                qasm_circt += gate_name + " q["  + std::to_string(qubits[0]) + "];\n";
                break;
            // 1 Parametric 1 qubit gates
            case InstructionTag::U1:
            case InstructionTag::GLOBALP:
            case InstructionTag::P:
            case InstructionTag::RX:
            case InstructionTag::RY:
            case InstructionTag::RZ:
            case InstructionTag::ROTINVX:
            case InstructionTag::ROTINVY:
            case InstructionTag::ROTINVZ:
            case InstructionTag::ROTX:
            case InstructionTag::ROTY:
            case InstructionTag::ROTZ:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 2 Parametric 1 qubit gates
                case InstructionTag::U2:
                case InstructionTag::R:
                {
                    params = instruction.at("params").get<std::vector<double>>();
                    qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                    break;
                }
            // 3 Parametric 1 qubit gates
            case InstructionTag::U3: 
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // 4 Parametric 1 qubit gates
            case InstructionTag::U:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) +", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            //UNITARY
            case InstructionTag::UNITARY:
            {
                matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<std::vector<double>>>>>();
                qasm_circt += gate_name + "(" + triple_vector_to_string(matrix[0]) + ") q[" + std::to_string(qubits[0]) + "];\n";
                break;
            }
            // Non-parametric 2 qubit gates
            case InstructionTag::SWAP:
            case InstructionTag::ISWAP:
            case InstructionTag::CX:
            case InstructionTag::CY:
            case InstructionTag::CZ:
            case InstructionTag::CSX:
            case InstructionTag::CSXDG:
            case InstructionTag::CS:
            case InstructionTag::CSDG:
            case InstructionTag::CT:
            case InstructionTag::DCX:
            case InstructionTag::ECR:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            // Parametric 2 qubit gates
            case InstructionTag::CU1:
            case InstructionTag::CP:
            case InstructionTag::CRX:
            case InstructionTag::CRY:
            case InstructionTag::CRZ:
            case InstructionTag::RXX:
            case InstructionTag::RYY:
            case InstructionTag::RZZ:
            case InstructionTag::RZX:
            case InstructionTag::XXMYY:
            case InstructionTag::XXPYY:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt += gate_name + "(" + std::to_string(params[0]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 2 Parametric 2 qubit gates
            case InstructionTag::CU2:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 3 Parametric 2 qubit gates
            case InstructionTag::CU3:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 4 Parametric 2 qubit gates
            case InstructionTag::CU:
            {
                params = instruction.at("params").get<std::vector<double>>();
                qasm_circt +=  gate_name + "(" + std::to_string(params[0]) + ", " + std::to_string(params[1]) + ", " + std::to_string(params[2]) + ", " + std::to_string(params[3]) + ")" + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // Non-parametric 3 qubit gates
            case InstructionTag::CCX:
            case InstructionTag::CCY:
            case InstructionTag::CCZ:
            case InstructionTag::CECR:
            case InstructionTag::CSWAP:
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "], q[" + std::to_string(qubits[2]) + "];\n";
                break;
            // Non-parametric 1 qubit multicontroled
            case InstructionTag::MCX:
            case InstructionTag::MCY:
            case InstructionTag::MCZ:
            case InstructionTag::MCSX:
            {
                qasm_circt += gate_name + " q[" + std::to_string(qubits[0]) + "], q[" + std::to_string(qubits[1]) + "];\n";
                break;
            }
            // 1 parametric 1 qubit multicontroled
            case InstructionTag::MCRX:
            case InstructionTag::MCRY:
            case InstructionTag::MCRZ:
            case InstructionTag::MCP:
            case InstructionTag::MCU1:
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
            case InstructionTag::MCU2:
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
            case InstructionTag::MCU3:
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
            case InstructionTag::MCU:
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
            case InstructionTag::MCSWAP:
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
            case InstructionTag::MEASURE:
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