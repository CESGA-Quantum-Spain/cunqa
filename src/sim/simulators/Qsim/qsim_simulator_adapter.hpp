#pragma once

#include <vector>

#include "simulator.hpp"
#include "circuit.hpp"
#include "run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QsimSimulatorAdapter final : public Simulator {
public:
    QsimSimulatorAdapter() = default;
    ~QsimSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Qsim";
    }

    inline std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return QSIM_BASIS_GATES;
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
    qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space;
    qsim::SimulatorBasic<qsim::ParallelFor>::State state; 
    qsim::SimulatorBasic<qsim::ParallelFor> simulator;

    static constexpr std::array<std::string_view, 26> QSIM_BASIS_GATES = {{
        "measure",
        "id", "x", "y", "z", "h", "s", "t", "sx", "sy", "hz2",
        "rx", "ry", "rz", "rxy", 
        "id2", "cx", "cz", "swap", "iswap",
        "cp",
        "hz2",
        "fs",
        "gp",
        "unitary", "cunitary"
    }};
};


} // End of sim namespace
} // End of cunqa namespace