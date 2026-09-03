#pragma once

#include <array>
#include <vector>
#include <string_view>

#include "circuit.hpp"
#include "sim/simulator.hpp"

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

    void set_noise_model(const JSON& noise_properties) override
    {
        throw std::runtime_error("Set noise model not implemented in MaestroSimulator");
    }

    std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const override;

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit) override;

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
 
    static constexpr auto MAESTRO_BASIS_GATES = std::to_array<std::string_view>({
        "measure",
        "x", "y", "z", "h", "s", "sdg", "t", "tdg", "sx", "sxdg", "k",
        "p", "rx", "ry", "rz",
        "u",
        "cx", "cy", "cz", "ch", "csx", "csxdg", "swap",
        "cp", "crx", "cry", "crz",
        "ccx", "cswap",
        "cu",
        // "reset" is deliberately NOT advertised. apply_gate() implements it via
        // maestrolib's ApplyReset, so it works on the dynamic path, but maestrolib's
        // JSON circuit format has no reset operation at all: its parser accepts
        // "measure" plus a fixed gate map, and anything else raises "Gate type not
        // supported." Since native_execute is the default path, listing reset here
        // would promise something a plain circuit cannot do. maestrolib's QASM 2.0
        // parser does handle reset, so routing native execution through QASM would
        // let it be advertised again.
    });

};


} // End of sim namespace
} // End of cunqa namespace