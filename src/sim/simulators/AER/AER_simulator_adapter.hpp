#pragma once

#include <vector>
#include <string_view>

#include "circuit.hpp"
#include "sim/simulator.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class AERSimulatorAdapter final : public Simulator {
public:
    
    AERSimulatorAdapter();
    ~AERSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Aer";
    }

    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return AER_BASIS_GATES;
    }

    void set_noise_model(const JSON& noise_properties) override;

    std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const override;

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitThreeParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitFourParam& payload) override;

    void apply_gate(const InstructionType& type, const MultiNoParam& payload) override;
    void apply_gate(const InstructionType& type, const MultiParam& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;
    void apply_gate(const InstructionType& type, const DiagonalMatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Reset& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;
private:
    struct State;
    std::unique_ptr<State> state_;
    struct NoiseModel;
    std::unique_ptr<NoiseModel> noise_model_;

    static constexpr std::array<std::string_view, 63> AER_BASIS_GATES = {{
        "measure",
        "id", "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg",
        "u1", "gp", "rx", "ry", "rz",
        "u2", "r",
        "u3",
        "swap", "cx", "cy", "cz", "csx", "ecr",
        "cp", "cu1", "crx", "cry", "crz", "rxx", "ryy", "rzz", "rzx",
        "cu2",
        "cu3", "cu",
        "ccx", "ccz", "cswap",
        "mcx", "mcy", "mcz", "mcsx",
        "mcp", "mcu1", "mcrx", "mcry", "mcrz",
        "mcu2",
        "mcu3",
        "mcswap",
        "unitary", "diagonal",
    }};

};


} // End of sim namespace
} // End of cunqa namespace