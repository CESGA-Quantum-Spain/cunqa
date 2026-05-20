#pragma once

#include <vector>
#include <string_view>

#include "sim/simulator.hpp"
#include "quantum_task/circuit.hpp"
#include "quantum_task/run_config.hpp"
#include "utils/json.hpp"

namespace cunqa {
namespace sim {

class CunqaSimulatorAdapter final : public Simulator {
public:

    CunqaSimulatorAdapter();
    ~CunqaSimulatorAdapter();

    inline std::string get_name() const noexcept override
    {
        return "Cunqa";
    }
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return CUNQA_BASIS_GATES;
    }

    void initialize() override;
    void clear() override;

    JSON native_execute(const Circuit& circuit, const JSON& noise_model) override;

    void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) override;

    void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload) override;
    void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload) override;

    void apply_gate(const InstructionType& type, const Measure& payload) override;
    void apply_gate(const InstructionType& type, const Copy& payload) override;

private:
    struct State;
    std::unique_ptr<State> state_;

    static constexpr std::array<std::string_view, 17> CUNQA_BASIS_GATES = {
        "measure", 
        "id", "x", "y", "z", "h", "sx",
        "rx", "ry", "rz", 
        "swap", "cx", "cy", "cz", 
        "crx", "cry", "crz",
    };

};


} // End of sim namespace
} // End of cunqa namespace