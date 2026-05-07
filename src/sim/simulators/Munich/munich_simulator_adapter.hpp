#pragma once

#include <vector>
#include <string_view>

#include "src/sim/simulator.hpp"

namespace cunqa {
namespace sim {

class MunichSimulatorAdapter final : public Simulator {
public:
    MunichSimulatorAdapter() = default;
    ~MunichSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Munich";
    }
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return MUNICH_BASIS_GATES;
    }

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit, const JSON& noise_model) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Reset& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;

private:

static constexpr std::array<std::string_view, 51> MUNICH_BASIS_GATES = {{
    "measure",
    "id", "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg",
    "u1", "gp", "p", "rx", "ry", "rz",
    "u2",
    "u3",
    "u",
    "cx", "cy", "cz", "ch", "csx", "cs", "csdg", "swap", "iswap", "ecr", "dcx",
    "cu1", "cp", "crx", "cry", "crz", "rxx", "ryy", "rzz", "rzx", "xxmyy", "xxpyy",
    "cu3",
    "cu",
    "cswap",
    "mcx", "mcp",
    "reset"
}};

};


} // End of sim namespace
} // End of cunqa namespace