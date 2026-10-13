#pragma once

#include <complex>
#include <span>
#include <map>
#include <string_view>
#include <utility>
#include <vector>
#include <iostream>

#include "dynamic_circuit/instruction_type.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "sim/run_config.hpp"

namespace cunqa {
namespace sim {

// Kraus operators of a quantum channel, each one stored row-major as a 2^n x 2^n matrix.
using KrausOperators = std::vector<std::vector<std::complex<double>>>;

class Simulator {
public:
    std::map<std::size_t, bool> creg;
    std::map<std::size_t, bool> save_clbit;
    std::size_t num_qubits;
    RunConfig config;
    
    void set_num_qubits(std::pair<std::size_t, std::size_t> num_qubits_)
    {
        num_qubits = num_qubits_.first + num_qubits_.second;
    }
    virtual void set_noise_model(const JSON& noise_properties) = 0;

    std::string get_measures() const
    {
        std::string result;
        for (const auto& [clbit, value] : creg)
            if (save_clbit.contains(clbit) && save_clbit.at(clbit))    
                result = (value ? '1' : '0') + result;
        return result;
    }
    virtual std::string get_name() const = 0;
    virtual std::span<const std::string_view> get_basis_gates() const = 0;

    virtual void initialize() = 0;
    virtual void clear() = 0;

    virtual std::unique_ptr<Circuit> create_circuit(const JSON& instructions_json) const = 0;

    virtual JSON native_execute(const Circuit& circuit) = 0;

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

    // Capabilities used by the time-aware noise model of the quantum-communications scheme.
    // Without Kraus channels (e.g. amplitude damping) it falls back to Pauli-twirled ones;
    // without reset, the communication qubits are not re-initialized when a pair arrives.
    virtual bool supports_kraus() const
    {
        return false;
    }

    virtual bool supports_reset() const
    {
        return false;
    }

    virtual void apply_kraus(const std::vector<std::size_t>& qubits, const KrausOperators& operators)
    {
        (void) qubits;
        (void) operators;
        throw std::runtime_error("Kraus channels are not supported by " + get_name() + " simulator.");
    }

protected:
    template <typename Gate>
    [[noreturn]] inline void unsupported_gate(const InstructionType& type, const Gate& payload) const
    {
        (void) payload;
        throw std::runtime_error(
            "Gate " + std::string(instruction_type_name(type)) + 
            " not supported by " + get_name() + " simulator." 
        );
    }
};

} // End of sim namespace
} // End of cunqa namespace