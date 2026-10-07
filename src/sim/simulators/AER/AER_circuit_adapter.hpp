#pragma once

#include <complex>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// AER dependencies
#include "framework/circuit.hpp"

#include "circuit.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {

struct AERCircuit : public Circuit {
    // "gp" is kept out of this list: it is not one of AER's gate names, 
    // AER carries a global phase on the circuit header instead.
    JSON instructions = JSON::array();
    JSON global_phase_instructions = JSON::array();

    std::vector<double*> params;
    // Subset of `params` pointing at the "gp" angles, so the total global phase is
    // a sum over just those (usually none) rather than a pass over every gate.
    std::vector<double*> global_phase_params;

    // Where each entry of `params` lives in the kept AER::Circuit: AER builds one op per
    // instruction, so op `op` is `instructions[op]` and the angle is its param `k`. "gp"
    // angles live in no op; they reach AER as the circuit's global phase.
    struct ParamTarget {
        bool global_phase;
        std::size_t op;
        std::size_t k;
    };
    std::vector<ParamTarget> param_targets;

    explicit AERCircuit(const JSON& instructions_json)
    {
        auto adapt_instr_format = [](JSON& instruction) {
            auto normalize_index_list = [](JSON& field, const char* name) {
                if (field.is_number_integer()) {
                    field = JSON::array({ field });
                    return;
                }

                if (!field.is_array()) {
                    throw std::invalid_argument(
                        std::string(name) + " must be an integer or an array of integers"
                    );
                }

                for (const auto& item : field) {
                    if (!item.is_number_integer()) {
                        throw std::invalid_argument(
                            std::string(name) + " must contain only integers"
                        );
                    }
                }
            };

            if (auto it = instruction.find("clbits"); it != instruction.end()) {
                normalize_index_list(*it, "clbits");

                JSON memory = std::move(*it);
                instruction.erase(it);
                instruction["memory"] = std::move(memory);
            }

            if (auto it = instruction.find("qubits"); it != instruction.end()) {
                normalize_index_list(*it, "qubits");
            }

            // cunqa carries matrix data in "matrix", but AER reads it from "params":
            // "unitary" wants a list holding exactly one matrix, while "diagonal"
            // wants the flat list of 2^n diagonal entries, which is the shape the
            // circuit already uses.
            if (auto it = instruction.find("matrix"); it != instruction.end()) {
                JSON matrix = std::move(*it);
                instruction.erase(it);

                if (instruction.at("name") == "unitary")
                    instruction["params"] = JSON::array({ std::move(matrix) });
                else
                    instruction["params"] = std::move(matrix);
            }
        };

        // First pass: normalize each instruction and route it to the right list.
        // Record where it landed so the parameters can still be collected in the
        // original circuit order, which is what update_params binds against.
        std::vector<std::pair<bool, std::size_t>> location;
        location.reserve(instructions_json.size());

        for (auto instruction : instructions_json) {
            adapt_instr_format(instruction);

            const bool is_global_phase = (instruction.at("name") == "gp");
            JSON& target = is_global_phase ? global_phase_instructions : instructions;

            location.emplace_back(is_global_phase, target.size());
            target.push_back(std::move(instruction));
        }

        // Second pass: neither list grows from here on, so the pointers taken below
        // stay valid for the lifetime of the circuit.
        for (const auto& [is_global_phase, index] : location) {
            JSON& instruction =
                is_global_phase ? global_phase_instructions[index] : instructions[index];

            auto instruction_type =
                instruction_type_from_name(instruction.at("name").get<std::string>());

            const std::size_t params_before = params.size();
            auto bind = [&](std::size_t k) {
                auto* param = instruction.at("params").at(k).get_ptr<double*>();
                if (param == nullptr) {
                    throw std::runtime_error("Expected a floating-point JSON parameter.");
                }
                params.push_back(param);
                param_targets.push_back({is_global_phase, index, k});
            };

            switch (instruction_type) {
                // One-parameter gates
                case InstructionType::RX:
                case InstructionType::RY:
                case InstructionType::RZ:
                case InstructionType::GLOBALP:
                case InstructionType::P:
                case InstructionType::U1:
                case InstructionType::ROTX:
                case InstructionType::ROTY:
                case InstructionType::ROTZ:
                case InstructionType::ROTINVX:
                case InstructionType::ROTINVY:
                case InstructionType::ROTINVZ:
                case InstructionType::CRX:
                case InstructionType::CRY:
                case InstructionType::CRZ:
                case InstructionType::CP:
                case InstructionType::CU1:
                case InstructionType::RXX:
                case InstructionType::RYY:
                case InstructionType::RZZ:
                case InstructionType::RZX:
                case InstructionType::MCRX:
                case InstructionType::MCRY:
                case InstructionType::MCRZ:
                case InstructionType::MCP:
                case InstructionType::MCU1:
                {
                    bind(0);
                    break;
                }

                // Two-parameter gates
                case InstructionType::U2:
                case InstructionType::R:
                case InstructionType::CU2:
                case InstructionType::MCU2:
                {
                    for (std::size_t i = 0; i < 2; ++i)
                        bind(i);
                    break;
                }

                // Three-parameter gates
                case InstructionType::U3:
                case InstructionType::CU3:
                case InstructionType::MCU3:
                {
                    for (std::size_t i = 0; i < 3; ++i)
                        bind(i);
                    break;
                }

                // Four-parameter gates
                case InstructionType::U:
                case InstructionType::CU:
                case InstructionType::MCU:
                {
                    for (std::size_t i = 0; i < 4; ++i)
                        bind(i);
                    break;
                }

                default:
                    break;
            }

            // gp is a one-parameter gate, so its angle is the pointer just added.
            if (is_global_phase && params.size() > params_before)
                global_phase_params.push_back(params.back());
        }
    }

    // Total global phase of the circuit, read through the parameter pointers so it
    // reflects the latest update_params() call.
    double global_phase() const
    {
        double total = 0.0;
        for (const auto* param : global_phase_params)
            total += *param;
        return total;
    }

    void update_params(const std::vector<double>& new_params) override
    {
        if (new_params.size() != params.size()) {
            throw std::runtime_error(
                "Number of parameters is " + std::to_string(params.size()) +
                " but " + std::to_string(new_params.size()) +
                " params were given."
            );
        }

        for (std::size_t i = 0; i < params.size(); ++i) {
            *(params[i]) = new_params[i];
        }

        // Keep the built circuit in step, so an update needs no rebuild.
        if (kept_) {
            for (std::size_t i = 0; i < params.size(); ++i) {
                const auto& target = param_targets[i];
                if (!target.global_phase)
                    kept_->ops[target.op].params[target.k] = new_params[i];
            }
        }
    }

    // The AER::Circuit to run now. Building one from the JSON instructions costs about
    // as much as simulating it, so it is built once and kept; each run gets a copy,
    // because AER modifies the circuit it executes in place (qubit truncation, metadata,
    // gate fusion) and the kept one must stay as built for the next update.
    std::shared_ptr<AER::Circuit> circuit_for_run(int num_clbits) const
    {
        if (!rebindable_)
            return build_(num_clbits);

        if (!kept_ || kept_clbits_ != num_clbits) {
            kept_ = build_(num_clbits);
            kept_clbits_ = num_clbits;
            if (!matches_instructions_(*kept_)) {
                // Cannot rebind this circuit in place: rebuild it on every run instead.
                LOGGER_DEBUG("AER circuit does not map one op per instruction; it will be "
                             "rebuilt on every run.");
                rebindable_ = false;
                auto circuit = std::move(kept_);
                kept_.reset();
                return circuit;
            }
        }

        auto circuit = std::make_shared<AER::Circuit>(*kept_);
        circuit->global_phase_angle = global_phase();
        // A built AER::Circuit draws a fresh random seed, and AER samples with it unless
        // the run sets seed_simulator. A copy would inherit the kept circuit's seed, so
        // every run would sample the same shot noise; draw a new one as a rebuild would.
        circuit->set_random_seed();
        return circuit;
    }

private:
    mutable std::shared_ptr<AER::Circuit> kept_;
    mutable int kept_clbits_ = -1;
    mutable bool rebindable_ = true;

    std::shared_ptr<AER::Circuit> build_(int num_clbits) const
    {
        return std::make_shared<AER::Circuit>(JSON({
            {"config", {{"memory_slots", num_clbits}}},
            {"header", {{"global_phase", global_phase()}}},
            {"instructions", instructions}
        }));
    }

    // Rebinding in place relies on AER building exactly one op per instruction and
    // keeping every angle where the JSON had it.
    bool matches_instructions_(const AER::Circuit& circuit) const
    {
        if (circuit.ops.size() != instructions.size())
            return false;
        for (std::size_t i = 0; i < params.size(); ++i) {
            const auto& target = param_targets[i];
            if (target.global_phase)
                continue;
            const auto& op_params = circuit.ops[target.op].params;
            if (target.k >= op_params.size() || op_params[target.k] != std::complex<double>(*params[i], 0.0))
                return false;
        }
        return true;
    }
};

} // namespace cunqa