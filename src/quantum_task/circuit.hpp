#pragma once

#include <string>
#include <variant>
#include <vector>
#include <array>
#include <complex>
#include <functional>
#include <stdexcept>

#include "utils/json.hpp"
#include "instruction_type.hpp"

#include "logger.hpp"

namespace cunqa {

using DiagonalMatrix = std::vector<std::complex<double>>;
using Matrix = std::vector<std::vector<std::complex<double>>>;

struct OneQubitNoParam {
    std::size_t qubit;
};

struct OneQubitOneParam {
    std::size_t qubit;
    double param;
};

struct OneQubitTwoParam {
    std::size_t qubit;
    std::array<double, 2> params;
};

struct OneQubitThreeParam {
    std::size_t qubit;
    std::array<double, 3> params;
};

struct OneQubitFourParam {
    std::size_t qubit;
    std::array<double, 4> params;
};

struct TwoQubitNoParam {
    std::array<std::size_t, 2> qubits;
};

struct TwoQubitOneParam {
    std::array<std::size_t, 2> qubits;
    double param;
};

struct TwoQubitTwoParam {
    std::array<std::size_t, 2> qubits;
    std::array<double, 2> params;
};

struct TwoQubitThreeParam {
    std::array<std::size_t, 2> qubits;
    std::array<double, 3> params;
};

struct TwoQubitFourParam {
    std::array<std::size_t, 2> qubits;
    std::array<double, 4> params;
};

struct ThreeQubitNoParam {
    std::array<std::size_t, 3> qubits;
};

// Here we use std::vector for simplicity and
// because multicontrolled are not as used as 
// the rest of the gates.
struct MulticontrolNoParam {
    std::vector<std::size_t> qubits;
};

struct MulticontrolParam {
    std::vector<std::size_t> qubits;
    std::vector<double> params;
};

struct MultiPauli {
    std::vector<std::size_t> qubits;
    double param;
    std::vector<unsigned int> pauli_id_list;
};

struct FusedSwap {
    std::vector<std::size_t> qubits;
    int block_size;
};

struct MatrixGate {
    std::vector<std::size_t> qubits;
    Matrix matrix;
};

struct DiagonalMatrixGate {
    std::vector<std::size_t> qubits;
    DiagonalMatrix matrix;
};

struct OneQubitNoise {
    std::size_t qubit;
    double params;
    int seed;
};

struct TwoQubitNoise {
    std::array<std::size_t, 2> qubits;
    double params;
    int seed;
};

struct RandomUnitary {
    std::vector<std::size_t> qubits;
    int seed;
};

struct Measure { 
    std::size_t qubit;
    std::size_t clbit;
    bool save;
};

struct Reset {
    std::vector<std::size_t> qubits;
};

struct Copy {
    std::vector<std::size_t> l_clbits;
    std::vector<std::size_t> r_clbits;
};

struct ClassicalComm {
    std::vector<std::size_t> clbits;
    std::vector<std::string> qpus;
};

struct GenEnt {
    std::size_t qubit;
    std::vector<std::string> qpus;
    std::string tag;
};

struct ClassicalIf {
    std::vector<std::size_t> clbits;
};


using InstructionVariant = std::variant<
    std::monostate,
    OneQubitNoParam,
    OneQubitOneParam,
    OneQubitTwoParam,
    OneQubitThreeParam,
    OneQubitFourParam,
    TwoQubitNoParam,
    TwoQubitOneParam,
    TwoQubitTwoParam,
    TwoQubitThreeParam,
    TwoQubitFourParam,
    ThreeQubitNoParam,
    MulticontrolNoParam,
    MulticontrolParam,
    MultiPauli,
    FusedSwap,
    MatrixGate,
    DiagonalMatrixGate,
    OneQubitNoise,
    TwoQubitNoise,
    RandomUnitary,
    Measure,
    Reset,
    Copy,
    ClassicalComm,
    GenEnt,
    ClassicalIf
>;

struct Instruction {
    InstructionType type;
    InstructionVariant payload;
};

struct Circuit {
    std::vector<Instruction> instructions;
    std::vector<double*> params;

    static Circuit from_json(const JSON& circuit_json) 
    {
        Circuit circuit;
        for (auto const& instruction : circuit_json) {
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
                        auto param = instruction.at("params").get<double>();
                        circuit.params.push_back(&param);
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
                        auto params = instruction.at("params").get<std::array<double, 2>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = OneQubitTwoParam {
                                instruction.at("qubits").get<std::size_t>(), 
                                params
                            }
                        };
                        break;
                    }
                    case InstructionType::U3:
                    {
                        auto params = instruction.at("params").get<std::array<double, 3>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        circuit.params.push_back(&params[2]);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = OneQubitThreeParam {
                                instruction.at("qubits").get<std::size_t>(), 
                                params
                            }
                        };
                        break;
                    }
                    case InstructionType::U:
                    {
                        auto params = instruction.at("params").get<std::array<double, 4>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        circuit.params.push_back(&params[2]);
                        circuit.params.push_back(&params[3]);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = OneQubitFourParam {
                                instruction.at("qubits").get<std::size_t>(), 
                                params
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
                        auto param = instruction.at("params").get<double>();
                        circuit.params.push_back(&param);
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
                        auto params = instruction.at("params").get<std::array<double, 2>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = TwoQubitTwoParam {
                                instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                                params
                            }
                        };
                        break;
                    }
                    case InstructionType::CU3:
                    {
                        auto params = instruction.at("params").get<std::array<double, 3>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        circuit.params.push_back(&params[2]);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = TwoQubitThreeParam {
                                instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                                params
                            }
                        };
                        break;
                    }
                    case InstructionType::CU:
                    {
                        auto params = instruction.at("params").get<std::array<double, 4>>();
                        circuit.params.push_back(&params[0]);
                        circuit.params.push_back(&params[1]);
                        circuit.params.push_back(&params[2]);
                        circuit.params.push_back(&params[3]);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = TwoQubitFourParam{
                                instruction.at("qubits").get<std::array<std::size_t, 2>>(),
                                params
                            }
                        };
                        break;
                    }
                    case InstructionType::CECR:
                    case InstructionType::CSWAP:
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
                    case InstructionType::MCSX:
                    case InstructionType::MCSWAP:
                    {
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = MulticontrolNoParam{
                                instruction.at("qubits").get<std::vector<std::size_t>>()
                            }
                        };
                        break;
                    }
                    case InstructionType::MCRX:
                    case InstructionType::MCRY:
                    case InstructionType::MCRZ:
                    case InstructionType::MCP:
                    case InstructionType::MCU1:
                    case InstructionType::MCU2:
                    case InstructionType::MCU3:
                    case InstructionType::MCU:
                    {
                        auto params = instruction.at("params").get<std::vector<double>>();
                        for(auto& param : params)
                            circuit.params.push_back(&param);
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = MulticontrolParam{
                                instruction.at("qubits").get<std::vector<std::size_t>>(), 
                                params
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
                        double param = 0;
                        if (instruction.contains("param"))
                            param = instruction.at("param");
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
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = OneQubitNoise{
                                instruction.at("qubits").get<std::size_t>(), 
                                instruction.at("params").get<double>(),
                                seed
                            }
                        };
                        break;
                    }
                    case InstructionType::TWOQUBITDEPOLARIZINGNOISE:
                    {
                        int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                        cunqa_instruction = {
                            .type = instruction_type,
                            .payload = TwoQubitNoise{
                                instruction.at("qubits").get<std::array<std::size_t, 2>>(), 
                                instruction.at("params").get<double>(),
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
                circuit.instructions.push_back(cunqa_instruction);
            } catch(const std::exception& e) {
                LOGGER_ERROR(
                    "Error in JSON instruction when processing the circuit: \n{}\n"
                    "Error message: \n\t{}",
                    instruction.dump(4), 
                    e.what()
                );
                return Circuit();
            };
        }
        return circuit;
    }

};

} // End of cunqa namespace
