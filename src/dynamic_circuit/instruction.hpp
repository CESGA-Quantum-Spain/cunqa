#pragma once

#include <complex>
#include <variant>
#include <vector>
#include <array>

#include "instruction_type.hpp"

namespace cunqa {

struct OneQubitNoParam {
    std::size_t qubit;
};

struct OneQubitOneParam {
    std::size_t qubit;
    double* param;
};

struct OneQubitTwoParam {
    std::size_t qubit;
    std::array<double*, 2> params;
};

struct OneQubitThreeParam {
    std::size_t qubit;
    std::array<double*, 3> params;
};

struct OneQubitFourParam {
    std::size_t qubit;
    std::array<double*, 4> params;
};

struct TwoQubitNoParam {
    std::array<std::size_t, 2> qubits;
};

struct TwoQubitOneParam {
    std::array<std::size_t, 2> qubits;
    double* param;
};

struct TwoQubitTwoParam {
    std::array<std::size_t, 2> qubits;
    std::array<double*, 2> params;
};

struct TwoQubitThreeParam {
    std::array<std::size_t, 2> qubits;
    std::array<double*, 3> params;
};

struct TwoQubitFourParam {
    std::array<std::size_t, 2> qubits;
    std::array<double*, 4> params;
};

struct ThreeQubitNoParam {
    std::array<std::size_t, 3> qubits;
};

// Here we use std::vector for simplicity and
// because multicontrolled are not as used as 
// the rest of the gates.
struct MultiNoParam {
    std::vector<std::size_t> qubits;
};

struct MultiParam {
    std::vector<std::size_t> qubits;
    std::vector<double*> params;
};

struct PauliNoParam {
    std::vector<std::size_t> qubits;
    std::string paulistr;
};

struct PauliParam {
    std::vector<std::size_t> qubits;
    double* param;
    std::string paulistr;
};

struct MultiPauli {
    std::vector<std::size_t> qubits;
    double* param;
    std::vector<unsigned int> pauli_id_list;
};

struct NumControlsNoParam {
    std::vector<std::size_t> qubits;
    int num_controls;
};

struct NumControlsParam {
    std::vector<std::size_t> qubits;
    double* param;
    int num_controls;
};

struct FusedSwap {
    std::vector<std::size_t> qubits;
    int block_size;
};

struct MatrixGate {
    std::vector<std::size_t> qubits;
    Matrix matrix;
};

struct DiagonalMatrixGate {
    std::vector<std::size_t> qubits;
    DiagonalMatrix matrix;
};

struct OneQubitNoise {
    std::size_t qubit;
    double* params;
    int seed;
};

struct TwoQubitNoise {
    std::array<std::size_t, 2> qubits;
    double* params;
    int seed;
};

struct RandomUnitary {
    std::vector<std::size_t> qubits;
    int seed;
};

struct Measure { 
    std::size_t qubit;
    std::size_t clbit;
    bool save;
};

struct Reset {
    std::vector<std::size_t> qubits;
};

struct Copy {
    std::vector<std::size_t> l_clbits;
    std::vector<std::size_t> r_clbits;
};

struct ClassicalComm {
    std::vector<std::size_t> clbits;
    std::vector<std::string> qpus;
};

struct GenEnt {
    std::size_t qubit;
    std::vector<std::string> qpus;
    std::string tag;
};

struct ClassicalIf {
    std::vector<std::size_t> clbits;
    bool condition;
    std::string operation;
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
    GenEnt,
    ClassicalIf
>;

struct Instruction {
    InstructionType type;
    InstructionVariant payload;
};

} // End of cunqa namespace