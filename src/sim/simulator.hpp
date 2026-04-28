#pragma once

#include <span>
#include <string_view>

#include "run_config.hpp"
#include "circuit.hpp"

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

    virtual void apply_gate(const OneQubitNoParam& instruction) 
    { 
        unsupported_gate(instruction); 
    }

    virtual void apply_gate(const OneQubitOneParam& instruction) 
    { 
        unsupported_gate(instruction); 
    }
    
    virtual void apply_gate(const OneQubitTwoParam& instruction) 
    { 
        unsupported_gate(instruction); 
    }

    virtual void apply_gate(const OneQubitThreeParam& instruction) 
    { 
        unsupported_gate(instruction); 
    }

    virtual void apply_gate(const OneQubitFourParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitNoParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitOneParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitTwoParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitThreeParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitFourParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const ThreeQubitNoParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const MulticontrolNoParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const MulticontrolParam& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const MultiPauli& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const FusedSwap& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const MatrixGate& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const DiagonalMatrixGate& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const OneQubitNoise& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const TwoQubitNoise& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const RandomUnitary& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const Measure& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const Reset& instruction)
    {
        unsupported_gate(instruction);
    }

    virtual void apply_gate(const Copy& instruction)
    {
        unsupported_gate(instruction);
    }

protected:
    template <typename Gate>
    [[noreturn]] inline void unsupported_gate(const Gate& instruction) const
    {
        throw std::runtime_error(
            "Gate " + INVERTED_INSTRUCTIONS_MAP.at(instruction.tag) + 
            " not supported by " + get_name() + " simulator." 
        );
    }
};

} // End of sim namespace
} // End of cunqa namespace