#pragma once

#include <string>
#include <vector>
#include <bitset>

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "csim/type.hpp"

#include "utils/json.hpp"
#include "utils/constants.hpp"

namespace cunqa {

inline void update_qulacs_circuit(QuantumCircuit& circuit, JSON& circuit_json)
{
    for (const auto& instruction : circuit_json) {

        auto inst_type = constants::INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        std::vector<UINT> qubits = instruction.at("qubits").get<std::vector<UINT>>();

        switch (inst_type)
        {
        case constants::MEASURE:
            circuit.add_X_gate(qubits[0]);
            break;
        case constants::X:
            circuit.add_X_gate(qubits[0]);
            break;
        case constants::Y:
            circuit.add_Y_gate(qubits[0]);
            break;
        case constants::Z:
            circuit.add_Z_gate(qubits[0]);
            break;
        case constants::H:
            circuit.add_H_gate(qubits[0]);
            break;
        case constants::S:
            circuit.add_S_gate(qubits[0]);
            break;
        case constants::SDAG:
            circuit.add_Sdag_gate(qubits[0]);
            break;
        case constants::T:
            circuit.add_T_gate(qubits[0]);
            break;
        case constants::TDAG:
            circuit.add_Tdag_gate(qubits[0]);
            break;
        case constants::SX:
            circuit.add_sqrtX_gate(qubits[0]);
            break;
        case constants::SQRTXDAG:
            circuit.add_sqrtXdag_gate(qubits[0]);
            break;
        case constants::SQRTY:
            circuit.add_sqrtY_gate(qubits[0]);
            break;
        case constants::SQRTYDAG:
            circuit.add_sqrtYdag_gate(qubits[0]);
            break;
        case constants::P0:
            circuit.add_P0_gate(qubits[0]);
            break;
        case constants::P1:
            circuit.add_P1_gate(qubits[0]);
            break;
        case constants::U1: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U1_gate(qubits[0], params[0]);
            break;
        }
        case constants::U2: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U2_gate(qubits[0], params[0], params[1]);
            break;
        }
        case constants::U3: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U3_gate(qubits[0], params[0], params[1], params[2]);
            break;
        }
        case constants::RX: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RX_gate(qubits[0], params[0]);
            break;
        }
        case constants::RY: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RY_gate(qubits[0], params[0]);
            break;
        }
        case constants::RZ: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RZ_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVX: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvX_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVY: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvY_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVZ: {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvZ_gate(qubits[0], params[0]);
            break;
        }
        case constants::CX:
            circuit.add_CNOT_gate(qubits[0], qubits[1]);
            break;
        case constants::CZ:
            circuit.add_CZ_gate(qubits[0], qubits[1]);
            break;
        case constants::ECR:
            circuit.add_ECR_gate(qubits[0], qubits[1]);
            break;
        case constants::SWAP:
            circuit.add_SWAP_gate(qubits[0], qubits[1]);
            break;
        default:
            std::cerr << "Instruction not suported!" << "\n" << "Instruction that failed: " << instruction.dump(4) << "\n";
        };
    }
}

inline JSON convert_to_counts(const std::vector<ITYPE>& result, int num_qubits)
{
    std::unordered_map<std::string, int> counts;

    for (auto& value : result) {
        std::bitset<64> bs(value);
        std::string bitstring = bs.to_string();

        if (num_qubits <= 0) {
            bitstring = "";
        } else if (num_qubits < 64) {
            bitstring = bitstring.substr(64 - num_qubits);
        } 
        counts[bitstring]++;
    }

    JSON result_in_counts;
    for (const auto& count : counts) {
        result_in_counts[count.first] = count.second;
    }

    return result_in_counts;
}

} // End namespace cunqa