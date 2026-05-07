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

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;
    
private:
    qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space;
    qsim::SimulatorBasic<qsim::ParallelFor>::State state; 
    qsim::SimulatorBasic<qsim::ParallelFor> simulator;
    std::mt19937& rgen;

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