#pragma once

#include <vector>

#include "simulator.hpp"
#include "circuit.hpp"
#include "run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QuestSimulatorAdapter final : public Simulator {
public:
    QuestSimulatorAdapter() = default;
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

    void apply_gate(const OneQubitNoParam& instruction) override;
    void apply_gate(const OneQubitOneParam& instruction) override;
    void apply_gate(const OneQubitFourParam& instruction) override;

    void apply_gate(const TwoQubitNoParam& instruction) override;
    void apply_gate(const TwoQubitOneParam& instruction) override;
    void apply_gate(const TwoQubitFourParam& instruction) override;

    void apply_gate(const ThreeQubitNoParam& instruction) override;

    void apply_gate(const MultiNoParam& instruction) override;
    void apply_gate(const MultiParam& instruction) override;

    void apply_gate(const PauliNoParam& instruction) override;
    void apply_gate(const PauliParam& instruction) override;

    void apply_gate(const NumControlsNoParam& instruction) override;
    void apply_gate(const NumControlsParam& instruction) override;

    void apply_gate(const MatrixGate& instruction) override;

    void apply_gate(const Measure& instruction) override;
    void apply_gate(const Reset& instruction) override;
    void apply_gate(const Copy& instruction) override;
private:
    std::unique_ptr<Qureg> qubits_state;

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