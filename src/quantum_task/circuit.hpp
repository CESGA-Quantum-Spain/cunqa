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
    InstructionTag tag;
    Qubit qubit;
};

struct OneQubitOneParam {
    InstructionTag tag;
    Qubit qubit;
    double param;
};

struct OneQubitTwoParam {
    InstructionTag tag;
    Qubit qubit;
    std::array<double, 2> params;
};

struct OneQubitThreeParam {
    InstructionTag tag;
    Qubit qubit;
    std::array<double, 3> params;
};

struct OneQubitFourParam {
    InstructionTag tag;
    Qubit qubit;
    std::array<double, 4> params;
};

struct TwoQubitNoParam {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
};

struct TwoQubitOneParam {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
    double param;
};

struct TwoQubitTwoParam {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
    std::array<double, 2> params;
};

struct TwoQubitThreeParam {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
    std::array<double, 3> params;
};

struct TwoQubitFourParam {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
    std::array<double, 4> params;
};

struct ThreeQubitNoParam {
    InstructionTag tag;
    std::array<Qubit, 3> qubits;
};

// Here we use std::vector for simplicity and
// because multicontrolled are not as used as 
// the rest of the gates.
struct MulticontrolNoParam {
    InstructionTag tag;
    std::vector<Qubit> qubits;
};

struct MulticontrolParam {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    std::vector<double> params;
};

struct MultiPauli {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    double param;
    std::vector<unsigned int> pauli_id_list;
};

struct FusedSwap {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    int block_size;
};

struct MatrixGate {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    Matrix matrix;
};

struct DiagonalMatrixGate {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    DiagonalMatrix matrix;
};

struct OneQubitNoise {
    InstructionTag tag;
    Qubit qubit;
    double params;
    int seed;
};

struct TwoQubitNoise {
    InstructionTag tag;
    std::array<Qubit, 2> qubits;
    double params;
    int seed;
};

struct RandomUnitary {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    int seed;
};

struct Measure {
    InstructionTag tag;
    Qubit qubit;
    Clbit clbit;
};

struct Reset {
    InstructionTag tag;
    std::vector<Qubit> qubits;
};

struct Copy {
    InstructionTag tag;
    std::vector<Clbit> l_clbits;
    std::vector<Clbit> r_clbits;
};

struct ClassicalComm {
    InstructionTag tag;
    std::vector<Clbit> clbits;
    std::vector<std::string> qpus;
};

struct QuantumComm {
    InstructionTag tag;
    std::vector<Qubit> qubits;
    std::vector<std::string> qpus;
};

struct ClassicalIf {
    InstructionTag tag;
    std::vector<Clbit> clbits;
};

using Instruction = std::variant<
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
    MulticontrolNoParam,
    MulticontrolParam,
    MultiPauli,
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

struct Circuit {
    std::vector<Instruction> instructions;
    std::vector<double*> params;
};

} // End of cunqa namespace