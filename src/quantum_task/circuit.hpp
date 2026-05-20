#pragma once

#include <string>
#include <variant>
#include <vector>
#include <array>
#include <complex>
#include <functional>

#include "utils/constants.hpp"
#include "utils/json.hpp"

namespace cunqa {

using Qubit = int;
using Clbit = int;

using DiagonalMatrix = std::vector<std::complex<double>>;
using Matrix = std::vector<std::vector<std::complex<double>>>;

struct OneQubitNoParam {
    Qubit qubit;
};

struct OneQubitOneParam {
    Qubit qubit;
    double param;
};

struct OneQubitTwoParam {
    Qubit qubit;
    std::array<double, 2> params;
};

struct OneQubitThreeParam {
    Qubit qubit;
    std::array<double, 3> params;
};

struct OneQubitFourParam {
    Qubit qubit;
    std::array<double, 4> params;
};

struct TwoQubitNoParam {
    std::array<Qubit, 2> qubits;
};

struct TwoQubitOneParam {
    std::array<Qubit, 2> qubits;
    double param;
};

struct TwoQubitTwoParam {
    std::array<Qubit, 2> qubits;
    std::array<double, 2> params;
};

struct TwoQubitThreeParam {
    std::array<Qubit, 2> qubits;
    std::array<double, 3> params;
};

struct TwoQubitFourParam {
    std::array<Qubit, 2> qubits;
    std::array<double, 4> params;
};

struct ThreeQubitNoParam {
    std::array<Qubit, 3> qubits;
};

// Here we use std::vector for simplicity and
// because multicontrolled are not as used as 
// the rest of the gates.
struct MultiNoParam {
    std::vector<Qubit> qubits;
};

struct MultiParam {
    std::vector<Qubit> qubits;
    std::vector<double> params;
};

struct PauliNoParam {
    std::vector<Qubit> qubits;
    std::string paulistr;
};

struct PauliParam {
    std::vector<Qubit> qubits;
    double param;
    std::string paulistr;
};

struct MultiPauli {
    std::vector<Qubit> qubits;
    double param;
    std::vector<unsigned int> pauli_id_list;
};

struct NumControlsNoParam {
    std::vector<Qubit> qubits;
    int num_controls;
};

struct NumControlsParam {
    std::vector<Qubit> qubits;
    double param;
    int num_controls;
};

struct FusedSwap {
    std::vector<Qubit> qubits;
    int block_size;
};

struct MatrixGate {
    std::vector<Qubit> qubits;
    Matrix matrix;
};

struct DiagonalMatrixGate {
    std::vector<Qubit> qubits;
    DiagonalMatrix matrix;
};

struct OneQubitNoise {
    Qubit qubit;
    double params;
    int seed;
};

struct TwoQubitNoise {
    std::array<Qubit, 2> qubits;
    double params;
    int seed;
};

struct RandomUnitary {
    std::vector<Qubit> qubits;
    int seed;
};

struct Measure { 
    Qubit qubit;
    Clbit clbit;
};

struct Reset {
    std::vector<Qubit> qubits;
};

struct Copy {
    std::vector<Clbit> l_clbits;
    std::vector<Clbit> r_clbits;
};

struct ClassicalComm {
    std::vector<Clbit> clbits;
    std::vector<std::string> qpus;
};

struct QuantumComm {
    std::vector<Qubit> qubits;
    std::vector<std::string> qpus;
};

struct ClassicalIf {
    std::vector<Clbit> clbits;
};

using InstructionVariant = std::variant<
    std::monostate,
    OneQubitNoParam,
    OneQubitOneParam,
    OneQubitTwoParam,
    OneQubitThreeParam,
    OneQubitFourParam,
    TwoQubitNoParam,
    TwoQubitOneParam,
    TwoQubitTwoParam,
    TwoQubitThreeParam,
    TwoQubitFourParam,
    ThreeQubitNoParam,
    MultiNoParam,
    MultiParam,
    PauliNoParam,
    PauliParam,
    MultiPauli,
    NumControlsNoParam,
    NumControlsParam,
    FusedSwap,
    MatrixGate,
    DiagonalMatrixGate,
    OneQubitNoise,
    TwoQubitNoise,
    RandomUnitary,
    Measure,
    Reset,
    Copy,
    ClassicalComm,
    QuantumComm,
    ClassicalIf
>;

struct Instruction {
    InstructionType type;
    InstructionVariant payload;
};

struct Circuit {
    std::vector<Instruction> instructions;
    std::vector<double*> params;

    JSON to_json() const {
        JSON json;
        JSON instructionsArray = JSON::array();
        
        for (const auto& instruction : instructions) {
            JSON instrJson;
            instrJson["type"] = static_cast<int>(instruction.type);
            
            // Helper lambda to convert arrays to JSON arrays
            auto arrayToJson = [](const auto& arr) {
                JSON jsonArr = JSON::array();
                for (const auto& elem : arr) {
                    jsonArr.push_back(elem);
                }
                return jsonArr;
            };
            
            // Helper lambda to convert complex matrix to JSON
            auto matrixToJson = [](const Matrix& matrix) {
                JSON jsonMatrix = JSON::array();
                for (const auto& row : matrix) {
                    JSON jsonRow = JSON::array();
                    for (const auto& elem : row) {
                        JSON jsonElem;
                        jsonElem["real"] = elem.real();
                        jsonElem["imag"] = elem.imag();
                        jsonRow.push_back(jsonElem);
                    }
                    jsonMatrix.push_back(jsonRow);
                }
                return jsonMatrix;
            };
            
            // Helper lambda to convert diagonal matrix to JSON
            auto diagonalMatrixToJson = [](const DiagonalMatrix& matrix) {
                JSON jsonMatrix = JSON::array();
                for (const auto& elem : matrix) {
                    JSON jsonElem;
                    jsonElem["real"] = elem.real();
                    jsonElem["imag"] = elem.imag();
                    jsonMatrix.push_back(jsonElem);
                }
                return jsonMatrix;
            };
            
            // Match instruction type and extract payload
            std::visit([&](const auto& payload) {
                using T = std::decay_t<decltype(payload)>;
                
                if constexpr (std::is_same_v<T, std::monostate>) {
                    // Empty instruction
                    instrJson["payload"] = JSON::object();
                }
                else if constexpr (std::is_same_v<T, OneQubitNoParam>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                }
                else if constexpr (std::is_same_v<T, OneQubitOneParam>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["param"] = payload.param;
                }
                else if constexpr (std::is_same_v<T, OneQubitTwoParam>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, OneQubitThreeParam>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, OneQubitFourParam>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, TwoQubitNoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                }
                else if constexpr (std::is_same_v<T, TwoQubitOneParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["param"] = payload.param;
                }
                else if constexpr (std::is_same_v<T, TwoQubitTwoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, TwoQubitThreeParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, TwoQubitFourParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, ThreeQubitNoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                }
                else if constexpr (std::is_same_v<T, MultiNoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                }
                else if constexpr (std::is_same_v<T, MultiParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["params"] = arrayToJson(payload.params);
                }
                else if constexpr (std::is_same_v<T, PauliNoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["paulistr"] = payload.paulistr;
                }
                else if constexpr (std::is_same_v<T, PauliParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["param"] = payload.param;
                    instrJson["payload"]["paulistr"] = payload.paulistr;
                }
                else if constexpr (std::is_same_v<T, MultiPauli>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["param"] = payload.param;
                    instrJson["payload"]["pauli_id_list"] = arrayToJson(payload.pauli_id_list);
                }
                else if constexpr (std::is_same_v<T, NumControlsNoParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["num_controls"] = payload.num_controls;
                }
                else if constexpr (std::is_same_v<T, NumControlsParam>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["param"] = payload.param;
                    instrJson["payload"]["num_controls"] = payload.num_controls;
                }
                else if constexpr (std::is_same_v<T, FusedSwap>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["block_size"] = payload.block_size;
                }
                else if constexpr (std::is_same_v<T, MatrixGate>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["matrix"] = matrixToJson(payload.matrix);
                }
                else if constexpr (std::is_same_v<T, DiagonalMatrixGate>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["matrix"] = diagonalMatrixToJson(payload.matrix);
                }
                else if constexpr (std::is_same_v<T, OneQubitNoise>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["params"] = payload.params;
                    instrJson["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, TwoQubitNoise>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["params"] = payload.params;
                    instrJson["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, RandomUnitary>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["seed"] = payload.seed;
                }
                else if constexpr (std::is_same_v<T, Measure>) {
                    instrJson["payload"]["qubit"] = payload.qubit;
                    instrJson["payload"]["clbit"] = payload.clbit;
                }
                else if constexpr (std::is_same_v<T, Reset>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                }
                else if constexpr (std::is_same_v<T, Copy>) {
                    instrJson["payload"]["l_clbits"] = arrayToJson(payload.l_clbits);
                    instrJson["payload"]["r_clbits"] = arrayToJson(payload.r_clbits);
                }
                else if constexpr (std::is_same_v<T, ClassicalComm>) {
                    instrJson["payload"]["clbits"] = arrayToJson(payload.clbits);
                    instrJson["payload"]["qpus"] = arrayToJson(payload.qpus);
                }
                else if constexpr (std::is_same_v<T, QuantumComm>) {
                    instrJson["payload"]["qubits"] = arrayToJson(payload.qubits);
                    instrJson["payload"]["qpus"] = arrayToJson(payload.qpus);
                }
                else if constexpr (std::is_same_v<T, ClassicalIf>) {
                    instrJson["payload"]["clbits"] = arrayToJson(payload.clbits);
                }
            }, instruction.payload);
            
            instructionsArray.push_back(instrJson);
        }
        
        json["instructions"] = instructionsArray;
        
        // Note: params are pointers to doubles, so we cannot serialize them directly
        // Add them if need arise
        
        return json;
    }
};

} // End of cunqa namespace