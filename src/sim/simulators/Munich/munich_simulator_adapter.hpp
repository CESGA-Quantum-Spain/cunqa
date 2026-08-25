#pragma once

#include <array>
#include <vector>
#include <string_view>

#include "circuit.hpp"
#include "sim/simulator.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class MunichSimulatorAdapter final : public Simulator {
public:

    MunichSimulatorAdapter();
    ~MunichSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Munich";
    }
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return MUNICH_BASIS_GATES;
    }

    void set_noise_model(const JSON& noise_properties) override;

    std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const override;

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitTwoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitThreeParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitFourParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitThreeParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitFourParam& payload) override;

    void apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload) override;

    void apply_gate(const InstructionType& type, const MultiNoParam& payload) override;
    void apply_gate(const InstructionType& type, const MultiParam& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Reset& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;

private:
    int seed;
    struct State;
    std::unique_ptr<State> state_;
    struct NoiseModel;
    std::unique_ptr<NoiseModel> noise_model_;

static constexpr auto MUNICH_BASIS_GATES = std::to_array<std::string_view>({
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
});

};


} // End of sim namespace
} // End of cunqa namespace