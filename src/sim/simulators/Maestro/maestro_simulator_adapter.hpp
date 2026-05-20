#pragma once

#include <vector>
#include <string_view>

#include "sim/simulator.hpp"
#include "quantum_task/circuit.hpp"
#include "quantum_task/run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class MaestroSimulatorAdapter final : public Simulator {
public:
    
    MaestroSimulatorAdapter();
    ~MaestroSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Maestro";
    }

    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return MAESTRO_BASIS_GATES;
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

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Reset& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;

private:
    void* maestroInstance = nullptr;
    void* simulator;

    static constexpr std::array<std::string_view, 32> MAESTRO_BASIS_GATES = {{
        "measure",
        "x", "y", "z", "h", "s", "sdg", "t", "tdg", "sx", "sxdg", "k",
        "p", "rx", "ry", "rz",
        "u",
        "cx", "cy", "cz", "ch", "csx", "csxdg", "swap",
        "cp", "crx", "cry", "crz",
        "ccx", "cswap",
        "cu",
        "reset",
    }};

};


} // End of sim namespace
} // End of cunqa namespace