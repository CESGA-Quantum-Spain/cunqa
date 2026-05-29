#pragma once

#include <vector>
#include <string_view>

#include "sim/simulator.hpp"
#include "quantum_task/circuit.hpp"
#include "quantum_task/run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QulacsSimulatorAdapter final : public Simulator {
public:

    QulacsSimulatorAdapter();
    ~QulacsSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Qulacs";
    }
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return QULACS_BASIS_GATES;
    }

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit, const JSON& noise_model) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitTwoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitThreeParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const FusedSwap& payload) override;
    void apply_gate(const InstructionType& type, const MultiPauli& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;
    void apply_gate(const InstructionType& type, const DiagonalMatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const RandomUnitary& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitNoise& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitNoise& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;

private:
    struct State;
    std::unique_ptr<State> state_;

    static constexpr std::array<std::string_view, 49> QULACS_BASIS_GATES = {{
        "measure",
        "id", "x", "y", "z", "h", "s", "sdg", "t", "tdg", "p0", "p1", "sx", "sxdg", "sy", "sydg", "multipauli",
        "u1", "rx", "ry", "rz", "rotinvx", "rotinvy", "rotinvz", "rotx", "roty", "rotz", "parametricrx", "parametricry", "parametricrz", "multipaulirotation",
        "u2",
        "u3",
        "cx", "cz", "swap", "fusedswap", "ecr",
        "cp",
        "amplitudedampingnoise", "bitflipnoise", "densematrix", "dephasingnoise", "depolarizingnoise", "diagonalmatrix", "independentxznoise", "randomunitary", "sparsematrix", "twoqubitdepolarizingnoise",
    }};

};


} // End of sim namespace
} // End of cunqa namespace