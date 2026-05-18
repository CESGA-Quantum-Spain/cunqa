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
};

} // End of cunqa namespace