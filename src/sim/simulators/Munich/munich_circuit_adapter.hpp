#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "circuit.hpp"
#include "dynamic_circuit/instruction_type.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace cunqa {

struct MunichCircuit : public Circuit {
    JSON instructions;
    std::vector<double*> params;

    explicit MunichCircuit(const JSON& instructions_json)
        : instructions(instructions_json)
    {

        for (auto& instruction : instructions) {

            auto instruction_type =
                instruction_type_from_name(instruction.at("name").get<std::string>());

            switch (instruction_type) {
                // One-parameter gates
                case InstructionType::RX:
                case InstructionType::RY:
                case InstructionType::RZ:
                case InstructionType::RAXIS:
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
                case InstructionType::CRAXIS:
                case InstructionType::CP:
                case InstructionType::CU1:
                case InstructionType::RXX:
                case InstructionType::RYY:
                case InstructionType::RZZ:
                case InstructionType::RXY:
                case InstructionType::RZX:
                case InstructionType::MCRX:
                case InstructionType::MCRY:
                case InstructionType::MCRZ:
                case InstructionType::MCRAXIS:
                case InstructionType::MCP:
                case InstructionType::MCU1:
                case InstructionType::MULTIPAULIROTATION:
                {
                    auto* param = instruction.at("params").at(0).get_ptr<double*>();
                    if (param == nullptr) {
                        throw std::runtime_error("Expected a floating-point JSON parameter.");
                    }
                    params.push_back(param);
                    break;
                }

                // Two-parameter gates
                case InstructionType::U2:
                case InstructionType::R:
                case InstructionType::CU2:
                case InstructionType::XXMYY:
                case InstructionType::XXPYY:
                case InstructionType::FS:
                case InstructionType::MCU2:
                {
                    for (std::size_t i = 0; i < 2; ++i) {
                        auto* param = instruction.at("params").at(i).get_ptr<double*>();
                        if (param == nullptr) {
                            throw std::runtime_error("Expected a floating-point JSON parameter.");
                        }
                        params.push_back(param);
                    }
                    break;
                }

                // Three-parameter gates
                case InstructionType::U3:
                case InstructionType::CU3:
                case InstructionType::MCU3:
                {
                    for (std::size_t i = 0; i < 3; ++i) {
                        auto* param = instruction.at("params").at(i).get_ptr<double*>();
                        if (param == nullptr) {
                            throw std::runtime_error("Expected a floating-point JSON parameter.");
                        }
                        params.push_back(param);
                    }
                    break;
                }

                // Four-parameter gates
                case InstructionType::U:
                case InstructionType::CU:
                case InstructionType::MCU:
                {
                    for (std::size_t i = 0; i < 4; ++i) {
                        auto* param = instruction.at("params").at(i).get_ptr<double*>();
                        if (param == nullptr) {
                            throw std::runtime_error("Expected a floating-point JSON parameter.");
                        }
                        params.push_back(param);
                    }
                    break;
                }

                default:
                    break;
            }
        }
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
    }
};

} // namespace cunqa