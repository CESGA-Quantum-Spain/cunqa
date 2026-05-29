#pragma once

#include <vector>

#include "sim/simulator.hpp"
#include "quantum_task/circuit.hpp"
#include "quantum_task/run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QuestSimulatorAdapter final : public Simulator {
public:
    QuestSimulatorAdapter();
    ~QuestSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Quest";
    }

    inline std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return QUEST_BASIS_GATES;
    }

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit, const JSON& noise_model) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitFourParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitFourParam& payload) override;

    void apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload) override;

    void apply_gate(const InstructionType& type, const MultiNoParam& payload) override;
    void apply_gate(const InstructionType& type, const MultiParam& payload) override;

    void apply_gate(const InstructionType& type, const PauliNoParam& payload) override;
    void apply_gate(const InstructionType& type, const PauliParam& payload) override;

    void apply_gate(const InstructionType& type, const NumControlsNoParam& payload) override;
    void apply_gate(const InstructionType& type, const NumControlsParam& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;
private:
    struct State;
    std::unique_ptr<State> state_;

    static constexpr std::array<std::string_view, 53> QUEST_BASIS_GATES = {{
        "s", "cs", "mcs",
        "t", "ct", "mct",
        "h", "ch", "mch",
        "swap", "cswap", "mcswap",
        "sqrtswap", "csqrtswap", "mcsqrtswap",
        "x", "y", "z",
        "cx", "cy", "cz",
        "mcx", "mcy", "mcz",
        "paulistr", "cpaulistr", "mcpaulistr",
        "rx", "ry", "rz",
        "crx", "cry", "crz",
        "mcrx", "mcry", "mcrz",
        "raxis", "craxis", "mcraxis",
        "pauligadget", "nonunitarypauligadget", "cpauligadget", "mcpauligadget",
        "phasegadget", "cphasegadget", "mcphasegadget",
        "p", "cp", "mcp",
        "mx", "cmx", "mcmx",
        "measure"
    }};
};


} // End of sim namespace
} // End of cunqa namespace