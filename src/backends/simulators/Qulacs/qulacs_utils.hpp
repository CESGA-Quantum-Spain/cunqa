#pragma once

#include <string>
#include <vector>
#include <bitset>

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "csim/type.hpp"

#include "utils/json.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"



namespace {

using CunqaQulacsComplex = std::vector<double>;
using CunqaQulacsDiagonalMatrix = std::vector<CunqaQulacsComplex>;
using CunqaQulacsRow = std::vector<CunqaQulacsComplex>;
using CunqaQulacsMatrix = std::vector<CunqaQulacsRow>;


} // End namespace

namespace cunqa {
namespace sim {


inline ComplexMatrix cunqamatrix_to_qulacsdensematrix(const CunqaQulacsMatrix& cunqa_matrix)
{
    size_t rows = cunqa_matrix.size();
    size_t cols = rows > 0 ? cunqa_matrix[0].size() : 0;

    ComplexMatrix qulacs_matrix(rows, cols);

    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            qulacs_matrix(i, j) = std::complex<double>(cunqa_matrix[i][j][0], cunqa_matrix[i][j][1]);  

    return qulacs_matrix;
}


inline SparseComplexMatrix cunqamatrix_to_sparse(const CunqaQulacsMatrix& cunqa_matrix)
{
    size_t rows = cunqa_matrix.size();
    size_t cols = rows > 0 ? cunqa_matrix[0].size() : 0;

    std::vector<Eigen::Triplet<CPPCTYPE>> triplets;

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            CPPCTYPE val(cunqa_matrix[i][j][0], cunqa_matrix[i][j][1]);
            if (val != CPPCTYPE(0.0, 0.0))
                triplets.emplace_back(i, j, val);
        }
    }

    SparseComplexMatrix qulacs_sparse(rows, cols);
    qulacs_sparse.setFromTriplets(triplets.begin(), triplets.end());
    return qulacs_sparse;
}


inline ComplexVector cunqadiagonal_to_qulacsdiagonal(const CunqaQulacsDiagonalMatrix& cunqa_diagonal)
{
    ComplexVector qulacs_diagonal(cunqa_diagonal.size());

    for (size_t i = 0; i < cunqa_diagonal.size(); ++i)
        qulacs_diagonal(i) = CPPCTYPE(cunqa_diagonal[i][0], cunqa_diagonal[i][1]);

    return qulacs_diagonal;
}


inline void update_qulacs_circuit(QuantumCircuit& circuit, const JSON& circuit_json)
{
    for (const auto& instruction : circuit_json) {

        auto inst_type = constants::INSTRUCTIONS_MAP.at(name);
        std::vector<UINT> qubits = instruction.at("qubits").get<std::vector<UINT>>();

        switch (inst_type)
        {
        case constants::MEASURE:
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
        case constants::SDG:
            circuit.add_Sdag_gate(qubits[0]);
            break;
        case constants::T:
            circuit.add_T_gate(qubits[0]);
            break;
        case constants::TDG:
            circuit.add_Tdag_gate(qubits[0]);
            break;
        case constants::SX:
            circuit.add_sqrtX_gate(qubits[0]);
            break;
        case constants::SXDG:
            circuit.add_sqrtXdag_gate(qubits[0]);
            break;
        case constants::SY:
            circuit.add_sqrtY_gate(qubits[0]);
            break;
        case constants::SYDG:
            circuit.add_sqrtYdag_gate(qubits[0]);
            break;
        case constants::P0:
            circuit.add_P0_gate(qubits[0]);
            break;
        case constants::P1:
            circuit.add_P1_gate(qubits[0]);
            break;
        case constants::U1: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U1_gate(qubits[0], params[0]);
            break;
        }
        case constants::RX: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RX_gate(qubits[0], params[0]);
            break;
        }
        case constants::RY: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RY_gate(qubits[0], params[0]);
            break;
        }
        case constants::RZ: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RZ_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVX: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvX_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVY: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvY_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTINVZ: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotInvZ_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTX: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotX_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTY: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotY_gate(qubits[0], params[0]);
            break;
        }
        case constants::ROTZ: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_RotZ_gate(qubits[0], params[0]);
            break;
        }
        case constants::U2: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U2_gate(qubits[0], params[0], params[1]);
            break;
        }
        case constants::U3: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U3_gate(qubits[0], params[0], params[1], params[2]);
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
        case constants::FUSEDSWAP:
        {
            auto block_size = instruction.at("block_size").get<UINT>();
            circuit.add_FusedSWAP_gate(qubits[0], qubits[1], block_size);
            break;
        }
        case constants::MULTIPAULI:
        {
            auto pauli_id_list = instruction.at("pauli_id_list").get<std::vector<unsigned int>>();
            circuit.add_multi_Pauli_gate(qubits, pauli_id_list);
            break;
        }
        case constants::MULTIPAULIROTATION:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            auto pauli_id_list = instruction.at("pauli_id_list").get<std::vector<unsigned int>>();
            circuit.add_multi_Pauli_rotation_gate(qubits, pauli_id_list, params[0]);
            break;
        }
        case cunqa::constants::UNITARY:
        {
            auto cunqa_matrix = instruction.at("matrix").get<std::vector<CunqaQulacsMatrix>>()[0];
            ComplexMatrix qulacs_matrix = cunqa::sim::cunqamatrix_to_qulacsdensematrix(cunqa_matrix);

            if (qubits.size() > 1) {
                circuit.add_dense_matrix_gate(qubits, qulacs_matrix);
            } else {
                circuit.add_dense_matrix_gate(qubits[0], qulacs_matrix);
            }
            break;
        }
        case cunqa::constants::RANDOMUNITARY:
        {
            if (instruction.contains("seed")) {
                auto seed = instruction.at("seed").get<unsigned int>();
                circuit.add_random_unitary_gate(qubits, seed);
            } else {
                circuit.add_random_unitary_gate(qubits);
            }
            break;
        }
        default:
            std::cerr << "Instruction not suported!" << "\n" << "Instruction that failed: " << instruction.dump(4) << "\n";
        };
    }
}

inline JSON convert_to_counts(const std::vector<ITYPE>& result, int n_qubits)
{
    std::unordered_map<std::string, size_t> counts;
    size_t max_position = (1 << n_qubits) - 1;

    for (auto& value : result) {
        std::bitset<64> bs(value);
        std::string bitstring = bs.to_string();

        if (n_qubits <= 0) {
            bitstring = "";
        } else if (n_qubits < 64) {
            bitstring = bitstring.substr(64 - n_qubits);
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
} // End namespace sim