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
        QuantumComputation(quantum_task.config.at("num_qubits").get<size_t>(), quantum_task.config.at("num_clbits").get<size_t>()),
        quantum_tasks{quantum_task}
    { 
        n_qubits = quantum_task.config.at("num_qubits").get<size_t>();
    }
    QuantumComputationAdapter(const std::vector<QuantumTask>& quantum_tasks) : 
        QuantumComputation(get_num_qubits_(quantum_tasks), get_num_clbits_(quantum_tasks)),
        quantum_tasks{quantum_tasks}
    { 
        n_qubits = get_num_qubits_(quantum_tasks);
        n_comm_qubits = get_num_comm_qubits_(quantum_tasks);
    }

    std::vector<QuantumTask> quantum_tasks;
    size_t n_qubits;
    size_t n_comm_qubits = 0;

private:

    std::size_t get_num_qubits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t tmp_n_qubits;
        for (auto& quantum_task : quantum_tasks) {
            tmp_n_qubits += quantum_task.config.at("num_qubits").get<size_t>();
        }

        if (quantum_tasks[0].config.contains("n_communication_qubits")) {
            size_t n_comm_qubits = quantum_tasks[0].config.at("n_communication_qubits").get<size_t>();
            if (n_comm_qubits % 2 != 0) { // Ensure communication qubits always in pairs
                n_comm_qubits++;
            }
            tmp_n_qubits += n_comm_qubits;
        } else {
            tmp_n_qubits += 2;
        }

        return tmp_n_qubits; 
    }

    std::size_t get_num_clbits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        size_t num_clbits = 0;
        for(const auto& quantum_task: quantum_tasks) {
            num_clbits += quantum_task.config.at("num_clbits").get<std::size_t>();
        }
        return num_clbits;
    }

    std::size_t get_num_comm_qubits_(const std::vector<QuantumTask>& quantum_tasks) 
    {
        if (quantum_tasks[0].config.contains("n_communication_qubits")) {
            size_t n_comm_qubits = quantum_tasks[0].config.at("n_communication_qubits").get<size_t>();
            if (n_comm_qubits % 2 != 0) { // Ensure communication qubits always in pairs
                n_comm_qubits++;
            }
            return n_comm_qubits;
        } else {
            return 2;
        }
    }

};


const std::unordered_map<int, OpType> MUNICH_INSTRUCTIONS_MAP = {
    // MEASURE
    {cunqa::MEASURE, OpType::Measure},

    // ONE QUBIT NO PARAM
    {cunqa::ID, OpType::I},
    {cunqa::X, OpType::X},
    {cunqa::Y, OpType::Y},
    {cunqa::Z, OpType::Z},
    {cunqa::H, OpType::H},
    {cunqa::S, OpType::S},
    {cunqa::SDG, OpType::Sdg},
    {cunqa::SX, OpType::SX},
    {cunqa::SXDG, OpType::SXdg},
    {cunqa::T, OpType::T},
    {cunqa::TDG, OpType::Tdg},
    {cunqa::V, OpType::V},
    {cunqa::VDG, OpType::Vdg},

    // ONE QUBIT ONE PARAM
    {cunqa::RX, OpType::RX},
    {cunqa::RY, OpType::RY},
    {cunqa::RZ, OpType::RZ},
    {cunqa::GLOBALP, OpType::GPhase},
    {cunqa::P, OpType::P},
    {cunqa::U1, OpType::P},

    // ONE QUBIT TWO PARAM
    {cunqa::U2, OpType::U2},

    // ONE QUBIT THREE PARAM 
    {cunqa::U3, OpType::U},

    // TWO QUBIT NO PARAM
    {cunqa::CX, OpType::X},
    {cunqa::CY, OpType::Y},
    {cunqa::CZ, OpType::Z},
    {cunqa::CH, OpType::H},
    {cunqa::CSX, OpType::SX},
    {cunqa::CS, OpType::S},
    {cunqa::CSDG, OpType::Sdg},
    {cunqa::SWAP, OpType::SWAP},
    {cunqa::ISWAP, OpType::iSWAP},
    {cunqa::ECR, OpType::ECR},
    {cunqa::DCX, OpType::DCX},

    // TWO QUBIT ONE PARAM
    {cunqa::CU1, OpType::P},
    {cunqa::CP, OpType::P},
    {cunqa::CRX, OpType::RX},
    {cunqa::CRY, OpType::RY},
    {cunqa::CRZ, OpType::RZ},
    {cunqa::RXX, OpType::RXX},
    {cunqa::RYY, OpType::RYY},
    {cunqa::RZZ, OpType::RZZ},
    {cunqa::RZX, OpType::RZX},
    {cunqa::XXMYY, OpType::XXminusYY},
    {cunqa::XXPYY, OpType::XXplusYY},

    // TWO QUBITS TWO PARAMS
    {cunqa::CU2, OpType::U2},

    // TWO QUBITS THREE PARAMS
    {cunqa::CU3, OpType::U},

    // THREE QUBITS NO PARAMS
    {cunqa::CSWAP, OpType::SWAP},
    
    // MULTICONTROLED NO PARAM
    {cunqa::MCX, OpType::X},

    // MULTICONTROLED PARAM
    {cunqa::MCP, OpType::P},

    // SPECIAL
    {cunqa::RESET, OpType::Reset}
};


inline void quantum_task_to_mqt_circuit(const JSON& circuit, QuantumComputation& mqt_circuit) 
{ 
    int inst_type;
    std::vector<unsigned int> qubits;
    for (auto& instruction : circuit) {
        inst_type = INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        qubits = instruction.at("qubits").get<std::vector<unsigned int>>();

        switch (INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>()))
        {
        case MEASURE:
        {
            mqt_circuit.emplace_back(std::make_unique<NonUnitaryOperation>(
                instruction.at("qubits").get<std::vector<Qubit>>()[0], 
                instruction.at("clbits").get<std::vector<Bit>>()[0]));
            break;
        }
        case ID:
        case X:
        case Y:
        case Z:
        case H:
        case S:
        case SDG:
        case SX:
        case SXDG:
        case T:
        case TDG:
        case V:
        case VDG:
        case RESET:
        case RX:
        case RY:
        case RZ:
        case GLOBALP:
        case P:
        case U1:
        case U2:
        case U3:
        case U:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case ECR:
        case SWAP:
        case ISWAP:
        case DCX:
        {
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case CX:
        case CY:
        case CZ:
        case CH:
        case CSX:
        case CS:
        case CSDG:
        case CSWAP:
        {
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case RXX:
        case RYY:
        case RZZ:
        case RZX:
        case XXMYY:
        case XXPYY:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case CP:
        case CRX:
        case CRY:
        case CRZ:
        case CU1:
        case CU2:
        case CU3:
        case CU:
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
            break;
        }
        case MCX:
        {
            Controls controls(qubits.begin(), qubits.end() - 1);
            mqt_circuit.emplace_back(std::make_unique<StandardOperation>(controls, qubits[qubits.size() - 1], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
            break;
        }
        case MCP:
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