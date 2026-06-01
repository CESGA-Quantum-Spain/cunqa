#pragma once

#include <span>
#include <map>
#include <string_view>
#include <utility>

#include "dynamic_circuit/instruction_type.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "sim/run_config.hpp"

namespace cunqa {
namespace sim {

class Simulator {
public:
    std::map<int, bool> creg;
    std::map<int, bool> save_clbit;
    std::size_t num_qubits;
    RunConfig config;
    
    std::string get_measures() const
    {
        std::string result;
        for (const auto& [clbit, value] : creg)
            if (save_clbit.contains(clbit) && save_clbit.at(clbit))    
                result = (value ? '1' : '0') + result;
        return result;
    }

    void set_num_qubits(std::pair<std::size_t, std::size_t> num_qubits_)
    {
        num_qubits = num_qubits_.first + num_qubits_.second;
    }

    virtual std::string get_name() const = 0;
    virtual std::span<const std::string_view> get_basis_gates() const = 0;

    virtual void initialize() = 0;
    virtual void clear() = 0;

    virtual std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const = 0;
    virtual JSON native_execute(const Circuit& circuit, const JSON& noise_model) = 0;

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
            "Gate " + std::string(instruction_type_name(type)) + 
            " not supported by " + get_name() + " simulator." 
        );
    }
};

} // End of sim namespace
} // End of cunqa namespace