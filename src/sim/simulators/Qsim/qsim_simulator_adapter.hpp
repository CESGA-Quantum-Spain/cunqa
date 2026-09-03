#pragma once

#include <array>
#include <vector>
#include <string_view>

#include "circuit.hpp"
#include "sim/simulator.hpp"

#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class QsimSimulatorAdapter final : public Simulator {
public:
    QsimSimulatorAdapter();
    ~QsimSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Qsim";
    }

    inline std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return QSIM_BASIS_GATES;
    }

    void set_noise_model(const JSON& noise_properties) override
    {
        throw std::runtime_error("Set noise model not implemented in QsimSimulator");
    }

    std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const override;

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitTwoParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload) override;

    void apply_gate(const InstructionType& type, const MatrixGate& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;
    
private:
    struct State;
    std::unique_ptr<State> state_;

    static constexpr auto QSIM_BASIS_GATES = std::to_array<std::string_view>({
        "measure",
        "id", "x", "y", "z", "h", "s", "t", "sx", "sy", "hz2",
        "rx", "ry", "rz",
        "gp",
        "id2", "cx", "cz", "swap", "iswap",
        "cp",
        "rxy",
        "fs",
        "unitary", "cunitary"
    });
};


} // End of sim namespace
} // End of cunqa namespace