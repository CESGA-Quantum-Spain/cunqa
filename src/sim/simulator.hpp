#pragma once

#include <span>
#include <string_view>

#include "quantum_task/run_config.hpp"
#include "quantum_task/circuit.hpp"

namespace cunqa {
namespace sim {

class Simulator {
public:
    std::vector<bool> creg;
    RunConfig config;
    
    virtual inline std::string get_name() const = 0;
    virtual inline std::span<const std::string_view> get_basis_gates() const = 0;
    
    virtual void initialize() = 0;
    virtual void clear() = 0;

    virtual JSON native_execute(const Circuit& circuit, const JSON& noise_model) = 0;

    inline std::string get_measures() const
    {
        std::string result;
        for (const auto& cbit : creg)
            result += cbit ? '1' : '0';
        return result;
    }

    virtual void apply_gate(const InstructionType& type, const OneQubitNoParam& payload) 
    { 
        unsupported_gate(type, payload); 
    }

    virtual void apply_gate(const InstructionType& type, const OneQubitOneParam& payload) 
    { 
        unsupported_gate(type, payload); 
    }
    
    virtual void apply_gate(const InstructionType& type, const OneQubitTwoParam& payload) 
    { 
        unsupported_gate(type, payload); 
    }

    virtual void apply_gate(const InstructionType& type, const OneQubitThreeParam& payload) 
    { 
        unsupported_gate(type, payload); 
    }

    virtual void apply_gate(const InstructionType& type, const OneQubitFourParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitThreeParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitFourParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const MultiNoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const MultiParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const PauliNoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const PauliParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const MultiPauli& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const NumControlsNoParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const NumControlsParam& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const FusedSwap& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const MatrixGate& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const DiagonalMatrixGate& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const OneQubitNoise& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const TwoQubitNoise& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const RandomUnitary& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const Measure& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const Reset& payload)
    {
        unsupported_gate(type, payload);
    }

    virtual void apply_gate(const InstructionType& type, const Copy& payload)
    {
        unsupported_gate(type, payload);
    }

protected:
    template <typename Gate>
    [[noreturn]] inline void unsupported_gate(const InstructionType& type, const Gate& payload) const
    {
        throw std::runtime_error(
            "Gate " + INVERTED_INSTRUCTIONS_MAP.at(type) + 
            " not supported by " + get_name() + " simulator." 
        );
    }
};

} // End of sim namespace
} // End of cunqa namespace