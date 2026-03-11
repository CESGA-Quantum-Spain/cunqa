#pragma once 

#include "ir/QuantumComputation.hpp"
#include "ir/operations/OpType.hpp"
#include "dd/Operations.hpp"

#include "quantum_task.hpp"
#include "utils/constants.hpp"
#include "logger.hpp"


namespace cunqa {
namespace sim {
using namespace qc;

// Extension of QuantumComputation for Distributed Classical Communications
class QuantumComputationAdapter : public QuantumComputation
{
public:
    // Constructors
    QuantumComputationAdapter() = default;
    QuantumComputationAdapter(const QuantumTask& quantum_task) : 
        QuantumComputation(quantum_task.config.at("num_qubits").get<std::size_t>(), quantum_task.config.at("num_clbits").get<std::size_t>()),
        quantum_tasks{quantum_task}
    { }
    QuantumComputationAdapter(const std::vector<QuantumTask>& quantum_tasks) : 
        QuantumComputation(get_num_qubits_(quantum_tasks), get_num_clbits_(quantum_tasks)),
        quantum_tasks{quantum_tasks}
    { }

    std::vector<QuantumTask> quantum_tasks;

private:

    std::size_t get_num_qubits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t num_qubits = 0;
        for(const auto& quantum_task: quantum_tasks) {
            num_qubits += quantum_task.config.at("num_qubits").get<std::size_t>();
        }
        return num_qubits + 2;
    }

    std::size_t get_num_clbits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t num_clbits = 0;
        for(const auto& quantum_task: quantum_tasks) {
            num_clbits += quantum_task.config.at("num_clbits").get<std::size_t>();
        }
        return num_clbits;
    }

};


const std::unordered_map<int, OpType> MUNICH_INSTRUCTIONS_MAP = {
    // MEASURE
    {cunqa::constants::MEASURE, OpType::Measure},

    // ONE QUBIT NO PARAM
    {cunqa::constants::ID, OpType::I},
    {cunqa::constants::X, OpType::X},
    {cunqa::constants::Y, OpType::Y},
    {cunqa::constants::Z, OpType::Z},
    {cunqa::constants::H, OpType::H},
    {cunqa::constants::S, OpType::S},
    {cunqa::constants::SDG, OpType::Sdg},
    {cunqa::constants::SX, OpType::SX},
    {cunqa::constants::SXDG, OpType::SXdg},
    {cunqa::constants::T, OpType::T},
    {cunqa::constants::TDG, OpType::Tdg},
    {cunqa::constants::V, OpType::V},
    {cunqa::constants::VDG, OpType::Vdg},

    // ONE QUBIT ONE PARAM
    {cunqa::constants::RX, OpType::RX},
    {cunqa::constants::RY, OpType::RY},
    {cunqa::constants::RZ, OpType::RZ},
    {cunqa::constants::GLOBALP, OpType::GPhase},
    {cunqa::constants::P, OpType::P},
    {cunqa::constants::U1, OpType::P},

    // ONE QUBIT TWO PARAM
    {cunqa::constants::U2, OpType::U2},

    // ONE QUBIT THREE PARAM 
    {cunqa::constants::U3, OpType::U},

    // TWO QUBIT NO PARAM
    {cunqa::constants::CX, OpType::X},
    {cunqa::constants::CY, OpType::Y},
    {cunqa::constants::CZ, OpType::Z},
    {cunqa::constants::CH, OpType::H},
    {cunqa::constants::CSX, OpType::SX},
    {cunqa::constants::CS, OpType::S},
    {cunqa::constants::CSDG, OpType::Sdg},
    {cunqa::constants::SWAP, OpType::SWAP},
    {cunqa::constants::ISWAP, OpType::iSWAP},
    {cunqa::constants::ECR, OpType::ECR},
    {cunqa::constants::DCX, OpType::DCX},

    // TWO QUBIT ONE PARAM
    {cunqa::constants::CU1, OpType::P},
    {cunqa::constants::CP, OpType::P},
    {cunqa::constants::CRX, OpType::RX},
    {cunqa::constants::CRY, OpType::RY},
    {cunqa::constants::CRZ, OpType::RZ},
    {cunqa::constants::RXX, OpType::RXX},
    {cunqa::constants::RYY, OpType::RYY},
    {cunqa::constants::RZZ, OpType::RZZ},
    {cunqa::constants::RZX, OpType::RZX},
    {cunqa::constants::XXMYY, OpType::XXminusYY},
    {cunqa::constants::XXPYY, OpType::XXplusYY},

    // TWO QUBITS TWO PARAMS
    {cunqa::constants::CU2, OpType::U2},

    // TWO QUBITS THREE PARAMS
    {cunqa::constants::CU3, OpType::U},

    // THREE QUBITS NO PARAMS
    {cunqa::constants::CSWAP, OpType::SWAP},
    
    // MULTICONTROLED NO PARAM
    {cunqa::constants::MCX, OpType::X},

    // MULTICONTROLED PARAM
    {cunqa::constants::MCP, OpType::P},

    // SPECIAL
    {cunqa::constants::RESET, OpType::Reset},
    {cunqa::constants::BARRIER, OpType::Barrier},

};


inline void quantum_task_to_mqt_circuit(const JSON& circuit, QuantumComputation& mqt_circuit) 
{ 
    int inst_type;
    std::vector<unsigned int> qubits;
    for (auto& instruction : circuit) {
        inst_type = constants::INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        qubits = instruction.at("qubits").get<std::vector<unsigned int>>();

        switch (constants::INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>()))
        {
        case constants::MEASURE:
        {
            mqt_circuit.emplace_back(std::make_unique<NonUnitaryOperation>(
                instruction.at("qubits").get<std::vector<Qubit>>()[0], 
                instruction.at("clbits").get<std::vector<Bit>>()[0]));
            break;
        }
        case constants::ID:
        case constants::X:
        case constants::Y:
        case constants::Z:
        case constants::H:
        case constants::S:
        case constants::SDG:
        case constants::SX:
        case constants::SXDG:
        case constants::T:
        case constants::TDG:
        case constants::V:
        case constants::VDG:
        case constants::RESET:
        case constants::BARRIER:
        {
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case constants::RX:
        case constants::RY:
        case constants::RZ:
        case constants::GLOBALP:
        case constants::P:
        case constants::U1:
        case constants::U2:
        case constants::U3:
        case constants::U:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case constants::ECR:
        case constants::SWAP:
        case constants::ISWAP:
        case constants::DCX:
        {
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case constants::CX:
        case constants::CY:
        case constants::CZ:
        case constants::CH:
        case constants::CSX:
        case constants::CS:
        case constants::CSDG:
        case constants::CSWAP:
        {
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case constants::RXX:
        case constants::RYY:
        case constants::RZZ:
        case constants::RZX:
        case constants::XXMYY:
        case constants::XXPYY:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case constants::CP:
        case constants::CRX:
        case constants::CRY:
        case constants::CRZ:
        case constants::CU1:
        case constants::CU2:
        case constants::CU3:
        case constants::CU:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case constants::MCX:
        {
            Controls controls(qubits.begin(), qubits.end() - 1);
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(controls, qubits[qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case constants::MCP:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            Controls controls(qubits.begin(), qubits.end() - 1);
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(controls, qubits[qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        default:
        {
            std::string gate_name = instruction.at("name").get<std::string>();
            LOGGER_ERROR("Gate {} not supported.", gate_name);
            break;
        }

        } // end switch 
    } // end for
}

} // End of sim namespace
} // End of cunqa namespace