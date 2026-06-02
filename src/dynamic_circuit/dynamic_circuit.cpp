
#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>

#include "dynamic_circuit.hpp"
#include "instruction_type.hpp"

#include "logger.hpp"

namespace cunqa {

DynamicCircuit::DynamicCircuit(const JSON& instructions_json)
{
    for (auto const& instruction : instructions_json) {
        try {
            Instruction cunqa_instruction;
            auto instruction_type = instruction_type_from_name(instruction.at("name").get<std::string>());
            switch (instruction_type) {
                case InstructionType::ID:
                case InstructionType::X:
                case InstructionType::Y:
                case InstructionType::Z:
                case InstructionType::H:
                case InstructionType::S:
                case InstructionType::SX:
                case InstructionType::SY:
                case InstructionType::SZ:
                case InstructionType::SDG:
                case InstructionType::SXDG:
                case InstructionType::SYDG:
                case InstructionType::SZDG:
                case InstructionType::T:
                case InstructionType::TDG:
                case InstructionType::P0:
                case InstructionType::P1:
                case InstructionType::V:
                case InstructionType::VDG:
                case InstructionType::K:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitNoParam {
                            instruction.at("qubits").get<std::size_t>()
                        }
                    };
                    break;
                }
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
                {
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitOneParam {
                            instruction.at("qubits").get<std::size_t>(), 
                            param
                        }
                    };
                    break;
                }
                case InstructionType::U2:
                case InstructionType::R:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 2>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitTwoParam {
                            instruction.at("qubits").get<std::size_t>(), 
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::U3:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 3>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitThreeParam {
                            instruction.at("qubits").get<std::size_t>(), 
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::U:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 4>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitFourParam {
                            instruction.at("qubits").get<std::size_t>(), 
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::ECR:
                case InstructionType::SWAP:
                case InstructionType::ISWAP:
                case InstructionType::CX:
                case InstructionType::CY:
                case InstructionType::CZ:
                case InstructionType::CH:
                case InstructionType::CSX:
                case InstructionType::CSXDG:
                case InstructionType::CS:
                case InstructionType::CSDG:
                case InstructionType::CT:
                case InstructionType::DCX:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitNoParam {
                            instruction.at("qubits").get<std::array<std::size_t, 2>>()
                        }
                    };
                    break;
                }
                case InstructionType::CRX:
                case InstructionType::CRY:
                case InstructionType::CRZ:
                case InstructionType::CP:
                case InstructionType::CU1:
                case InstructionType::RXX:
                case InstructionType::RYY:
                case InstructionType::RZZ:
                case InstructionType::RZX:
                {
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitOneParam {
                            instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                            param
                        }
                    };
                    break;
                }
                case InstructionType::CU2:
                case InstructionType::XXMYY:
                case InstructionType::XXPYY:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 2>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitTwoParam {
                            instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::CU3:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 3>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitThreeParam {
                            instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::CU:
                {
                    auto instr_params = instruction.at("params").get<std::array<double, 4>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitFourParam{
                            instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::CECR:
                case InstructionType::CSWAP:
                case InstructionType::CSQRTSWAP:
                case InstructionType::CCX:
                case InstructionType::CCY:
                case InstructionType::CCZ:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = ThreeQubitNoParam{
                            instruction.at("qubits").get<std::array<std::size_t, 3>>()
                        }
                    };
                    break;
                }
                case InstructionType::MCX:
                case InstructionType::MCY:
                case InstructionType::MCZ:
                case InstructionType::MCH:
                case InstructionType::MCSX:
                case InstructionType::MCS:
                case InstructionType::MCT:
                case InstructionType::MCSWAP:
                case InstructionType::MCSQRTSWAP:
                case InstructionType::MX:
                case InstructionType::CMX:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = MultiNoParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>()
                        }
                    };
                    break;
                }
                case InstructionType::MCRX:
                case InstructionType::MCRY:
                case InstructionType::MCRZ:
                case InstructionType::MCRAXIS:
                case InstructionType::MCP:
                case InstructionType::MCU1:
                case InstructionType::MCU2:
                case InstructionType::MCU3:
                case InstructionType::MCU:
                case InstructionType::PHASEGADGET:
                case InstructionType::CPHASEGADGET:
                {
                    auto instr_params = instruction.at("params").get<std::vector<double>>();
                    auto param_ptrs = params.add_parameters(instr_params);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = MultiParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            param_ptrs
                        }
                    };
                    break;
                }
                case InstructionType::PAULISTR:
                case InstructionType::CPAULISTR:
                case InstructionType::MCPAULISTR:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = PauliNoParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            instruction.at("paulistr").get<std::string>()
                        }
                    };
                    break;
                }
                case InstructionType::PAULIGADGET:
                case InstructionType::CPAULIGADGET:
                case InstructionType::MCPAULIGADGET:
                case InstructionType::NONUNITARYPAULIGADGET:
                {
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = PauliParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            param,
                            instruction.at("paulistr").get<std::string>()
                        }
                    };
                    break;
                }
                case InstructionType::MCMX:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = NumControlsNoParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            instruction.at("num_controls").get<int>()
                        }
                    };
                    break;
                }
                case InstructionType::MCPHASEGADGET:
                {
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = NumControlsParam{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            param
                        }
                    };
                    break;
                }
                case InstructionType::UNITARY:
                case InstructionType::SPARSEMATRIX:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = MatrixGate{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            Matrix() // TODO: Arreglar
                        }
                    };
                    break;
                }
                case InstructionType::DIAGONAL:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = DiagonalMatrixGate {
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            DiagonalMatrix() // TODO: Arreglar
                        }
                    };
                    break;
                }
                case InstructionType::RANDOMUNITARY:
                {
                    int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = RandomUnitary {
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            seed
                        }
                    };
                    break;
                }
                case InstructionType::FUSEDSWAP:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = FusedSwap {
                            instruction.at("qubits").get<std::vector<std::size_t>>(),
                            instruction.at("block_size").get<int>()
                        }
                    };
                    break;
                }
                case InstructionType::MULTIPAULI:
                case InstructionType::MULTIPAULIROTATION:
                {
                    double instr_param = 0.0;
                    if (instruction.contains("param")) {
                        instr_param = instruction.at("param").at(0).get<double>();
                    }
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = MultiPauli{
                            instruction.at("qubits").get<std::vector<std::size_t>>(), 
                            param,
                            instruction.at("pauli_id_list").get<std::vector<unsigned int>>()
                        }
                    };
                    break;
                }
                case InstructionType::AMPLITUDEDAMPINGNOISE:
                case InstructionType::BITFLIPNOISE:
                case InstructionType::DEPHASINGNOISE:
                case InstructionType::DEPOLARIZINGNOISE:
                case InstructionType::INDEPENDENTXZNOISE:
                {
                    int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = OneQubitNoise{
                            instruction.at("qubits").get<std::size_t>(), 
                            param,
                            seed
                        }
                    };
                    break;
                }
                case InstructionType::TWOQUBITDEPOLARIZINGNOISE:
                {
                    int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                    auto instr_param = instruction.at("params").at(0).get<double>();
                    auto* param = params.add_parameter(instr_param);
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = TwoQubitNoise{
                            instruction.at("qubits").get<std::array<std::size_t, 2>>(), 
                            param,
                            seed
                        }
                    };
                    break;
                }
                case InstructionType::SEND:
                case InstructionType::RECV:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = ClassicalComm{
                            instruction.at("clbits").get<std::vector<std::size_t>>(), 
                            instruction.at("qpus").get<std::vector<std::string>>()
                        }
                    };
                    break;
                }
                case InstructionType::GENENT:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = GenEnt{
                            instruction.at("comm_qubit").get<std::size_t>(),
                            instruction.at("qpus").get<std::vector<std::string>>(),
                            instruction.at("tag")
                        }
                    };
                    break;
                }
                case InstructionType::CIF:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = ClassicalIf{
                            instruction.at("clbits").get<std::vector<std::size_t>>(), 
                            static_cast<bool>(instruction.at("condition").get<int>()),
                            instruction.at("operation").get<std::string>()
                        }
                    };
                    break;
                }
                case InstructionType::ENDCIF:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = ClassicalIf{
                            instruction.at("clbits").get<std::vector<std::size_t>>(), 
                        }
                    };
                    break;
                }
                case InstructionType::COPY:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = Copy{
                            instruction.at("l_clbits").get<std::vector<std::size_t>>(),
                            instruction.at("r_clbits").get<std::vector<std::size_t>>()
                        }
                    };
                    break;
                }
                case InstructionType::RESET:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = Reset{
                            instruction.at("qubits").get<std::vector<std::size_t>>()
                        }
                    };
                    break;
                }
                case InstructionType::SAVE_STATE:
                {
                    // TODO
                    break;
                }    
                case InstructionType::MEASURE:
                {
                    cunqa_instruction = {
                        .type = instruction_type,
                        .payload = Measure{
                            instruction.at("qubits").get<std::size_t>(),
                            instruction.at("clbits").get<std::size_t>(),
                            instruction.at("save").get<bool>()
                        }
                    };
                    break;
                }
                default:
                    throw std::runtime_error("Instruction not suported!");
            } // End switch
            instructions.push_back(cunqa_instruction);
        } catch(const std::exception& e) {
            LOGGER_ERROR(
                "Error in JSON instruction when processing the circuit: \n{}\n"
                "Error message: \n\t{}",
                instruction.dump(4), 
                e.what()
            );
        };
    }
}

} // End of cunqa namespace