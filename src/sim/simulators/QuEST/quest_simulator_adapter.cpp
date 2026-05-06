#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>
#include <random>
#include <stdexcept>

#include "quest_simulator_adapter.hpp"
#include "quest.h"

#include "simulator.hpp"
#include "circuit.hpp"
#include "run_config.hpp"

#include "utils/constants.hpp"
#include "logger.hpp"

namespace {
using namespace cunqa;

std::vector<std::vector<qcomp>> cunqamatrix_to_questmatrix(const CUNQAMatrix& cunqa_matrix)
{
    size_t n = cunqa_matrix.size();
    if (n == 0) return {};

    std::vector<std::vector<qcomp>> quest_mat;

    for (const auto& row : cunqa_matrix) {
        std::vector<qcomp> complexRow;
        for (const auto& complex : row) {
            complexRow.emplace_back(complex[0], complex[1]);
        }
        quest_mat.push_back(complexRow);
    }

    return quest_mat;
}

void update_meas_counter(std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>& meas_counter, const std::unordered_map<std::string, std::string>& shot_bitstrings)
{
    for (const auto& [circ_id, outcome] : shot_bitstrings) {
        meas_counter[circ_id][outcome]++;
    }
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

QuestSimulatorAdapter::QuestSimulatorAdapter() = default;
QuestSimulatorAdapter::~QuestSimulatorAdapter() = default;

void QuestSimulatorAdapter::initialize() {
    if (config.method == "statevector" || config.method == "automatic"){
        int vec_or_mat = 0;
    } else if (config.method == "density_matrix") {
        int vec_or_mat = 1;
    } else {
        LOGGER_ERROR("QuEST simulator only supports statevector or density matrix simulation, while {} was given", config.method);
        throw std::invalid_argument{"QuEST simulator only supports statevector or density matrix simulation"};
    }
    const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
    unsigned num_threads = 1;
    if (num_threads_char != nullptr) {
        num_threads = std::stoi(num_threads_char);
    }
    int useMultithread = (num_threads > 1) ? 1 : 0;
    int useGpuAccel = (qc.quantum_tasks[0].config.at("device")["device_name"] == "GPU") ? 1 : 0;
    if (!isQuESTEnvInit()) {
        initCustomQuESTEnv(0, useGpuAccel, useMultithread);
    }

    qubits_state = std::make_unique<Qureg>(
        createCustomQureg(config.num_qubits, vec_or_mat, 0, useGpuAccel, useMultithread)
    );
}

void QuestSimulatorAdapter::clear()
{
    initZeroState(qubits_state);
}


void QuestSimulatorAdapter::apply_gate(const OneQubitNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::ID:
            break;

        case InstructionTag::X:
            applyPauliX(qubits_state, instruction.qubit);
            break;

        case InstructionTag::Y:
            applyPauliY(qubits_state, instruction.qubit);
            break;

        case InstructionTag::Z:
            applyPauliZ(qubits_state, instruction.qubit);
            break;

        case InstructionTag::H:
            applyHadamard(qubits_state, instruction.qubit);
            break;

        case InstructionTag::S:
            applyS(qubits_state, instruction.qubit);
            break;
        
        case InstructionTag::T:
            applyT(qubits_state, instruction.qubit);
            break;
        
        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const OneQubitOneParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::P:
            applyPhaseShift(qubits_state, instruction.qubit, instruction.param);
            break;

        case InstructionTag::RX:
            applyRotateX(qubits_state, instruction.qubit, instruction.param);
            break;

        case InstructionTag::RY:
            applyRotateY(qubits_state, instruction.qubit, instruction.param);
            break;

        case InstructionTag::RZ:
            applyRotateZ(qubits_state, instruction.qubit, instruction.param);
            break;
        
        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const OneQubitFourParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::RAXIS:
            applyRotateAroundAxis(
                qubits_state, 
                instruction.qubit, 
                instruction.params[0],
                instruction.params[1], //axis
                instruction.params[2], //axis 
                instruction.params[3]  //axis
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const TwoQubitNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::SWAP:
            applySwap(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;
        
        case InstructionTag::SQRTSWAP:
            applySqrtSwap(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        case InstructionTag::CX:
            applyControlledPauliX(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        case InstructionTag::CY:
            applyControlledPauliY(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        case InstructionTag::CZ:
            applyControlledPauliZ(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        case InstructionTag::CH:
            applyControlledHadamard(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        case InstructionTag::CS:
            applyControlledS(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;
        
        case InstructionTag::CT:
            applyControlledT(qubits_state, *instruction.qubits.begin(), instruction.qubits.back());
            break;

        
        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const TwoQubitOneParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::CP:
            applyTwoQubitPhaseShift(qubits_state, *instruction.qubits.begin(), instruction.qubits.back(), instruction.param);
            break;

        case InstructionTag::CRX:
            applyControlledRotateX(qubits_state, *instruction.qubits.begin(), instruction.qubits.back(), instruction.param);
            break;

        case InstructionTag::CRY:
            applyControlledRotateY(qubits_state, *instruction.qubits.begin(), instruction.qubits.back(), instruction.param);
            break;

        case InstructionTag::CRZ:
            applyControlledRotateZ(qubits_state, *instruction.qubits.begin(), instruction.qubits.back(), instruction.param);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const TwoQubitFourParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::CRAXIS:
            applyControlledRotateAroundAxis(
                qubits_state,
                *intruction.qubits.begin(),
                intruction.qubits.back(),
                instruction.params[0],
                instruction.params[1], //axis
                instruction.params[2], //axis 
                instruction.params[3]  //axis
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const ThreeQubitNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::CSWAP:
            applyControlledSwap(qubits_state, instruction.qubits[0], instruction.qubits[1], instruction.qubits[2]);
            break;

        case InstructionTag::CSQRTSWAP:
            applyControlledSqrtSwap(qubits_state, instruction.qubits[0], instruction.qubits[1], instruction.qubits[2]);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const PauliNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::PAULISTR:
            applyPauliStr(qubits_state, getPauliStr(instruction.paulistr));
            break;

        case InstructionTag::CPAULISTR:   
            applyControlledPauliStr(qubits_state, instruction.qubits[0], getPauliStr(instruction.paulistr));
            break;

        case constants::MCPAULISTR:
            applyMultiControlledPauliStr(qubits_state, instruction.qubits, getPauliStr(instruction.paulistr));
            break;
        

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const PauliParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::PAULIGADGET:
            applyPauliGadget(qubits_state, getPauliStr(instruction.paulistr), instruction.param);
            break;

        case InstructionTag::NONUNITARYPAULIGADGET:
            applyNonUnitaryPauliGadget(qubits_state, getPauliStr(instruction.paulistr), instruction.param);
            break;

        case InstructionTag::CPAULIGADGET:
            applyControlledPauliGadget(qubits_state, instruction.qubits[0], getPauliStr(instruction.paulistr), instruction.param);
            break;

        case constants::MCPAULIGADGET:
            applyMultiControlledPauliGadget(qubits_state, instruction.qubits, getPauliStr(instruction.paulistr), instruction.param);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const MultiNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MCX:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledPauliX(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCY:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledPauliY(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCZ:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledPauliZ(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCH:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledHadamard(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCS:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledS(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCT:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledPauliX(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCSWAP:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledSwap(qubits_state, controls, instruction.qubits.back());
            break;

        case InstructionTag::MCSQRTSWAP:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledSqrtSwap(qubits_state, controls, instruction.qubits.back());
            break;

        case constants::MX:
            applyMultiQubitNot(qubits_state, instruction.qubits);
            break;

        case constants::CMX:
            std::vector<int> targets(instruction.qubits.begin()+1, instruction.qubits.end());
            applyControlledMultiQubitNot(qubits_state, instruction.qubits[0], targets);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const MultiParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MCRX:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledRotateX(qubits_state, controls, instruction.qubits.back(), instruction.params[0]);
            break;

        case InstructionTag::MCRY:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledRotateY(qubits_state, controls, instruction.qubits.back(), instruction.params[0]);
            break;

        case InstructionTag::MCRZ:
            std::vector<int> controls(instruction.qubits.begin(), instruction.qubits.end()-1);
            applyMultiControlledRotateZ(qubits_state, controls, instruction.qubits.back(), instruction.params[0]);
            break;

        case InstructionTag::MCP:
            applyMultiQubitPhaseShift(qubits_state, instruction.qubits, instruction.params[0]);
            break;

        case InstructionTag::MCRAXIS:
            applyMultiControlledRotateAroundAxis(
                qubits_state,
                controls,
                isntruction.qubits.back(),
                instruction.params[0],
                instruction.params[1], //axis
                instruction.params[2], //axis
                instruction.params[3]  //axis
            );
            break;

        case InstructionTag::PHASEGADGET:
            applyPhaseGadget(qubits_state, instruction.qubits, instruction.params[0]);
            break;

        case InstructionTag::CPHASEGADGET:
            std::vector<int> targets(instruction.qubits.begin() + 1, instruction.qubits.end());
            applyControlledPhaseGadget(qubits_state, instruction.qubits[0], targets, instruction.params[0]);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const NumControlsNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MCMX:
            std::vector<int> controls(instruction.qubits.begin(),                               instruction.qubits.begin() + instruction.num_controls + 1);
            std::vector<int> targets(instruction.qubits.begin() + instruction.num_controls + 1, instruction.qubits.end());
            applyMultiControlledMultiQubitNot(qubits_state, controls, targets);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const NumControlsParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MCPHASEGADGET:
            std::vector<int> controls(instruction.qubits.begin(),                               instruction.qubits.begin() + instruction.num_controls + 1);
            std::vector<int> targets(instruction.qubits.begin() + instruction.num_controls + 1, instruction.qubits.end());
            applyMultiControlledPhaseGadget(qubits_state, controls, targets, instruction.param);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const MatrixGate& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::UNITARY:
        {
            auto cunqa_matrix = instruction.matrix;
            CompMatr quest_matrix = createCompMatr(instruction.qubits.size());
            // Using this constructor setCompMatr(CompMatr out, std::vector<std::vector<qcomp>> in);
            setCompMatr(quest_matrix, cunqamatrix_to_questmatrix(cunqa_matrix));
            
            applyCompMatr(qubits_state, instruction.qubits, quest_matrix); //instruction.qubits must be std::vector<int>
            break;
        }
        case constants::CUNITARY:
        {
            auto cunqa_matrix = instruction.matrix;
            CompMatr quest_matrix = createCompMatr(instruction.qubits.size() - 1);
            // Using this constructor setCompMatr(CompMatr out, std::vector<std::vector<qcomp>> in);
            setCompMatr(quest_matrix, cunqamatrix_to_questmatrix(cunqa_matrix));
            
            std::vector<int> targets(instruction.qubits.begin() + 1, instruction.qubits.end());
            applyControlledCompMatr(qubits_state, instruction.qubits[0], targets, quest_matrix);
            break;
        }

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const Measure& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MEASURE:
            creg[instruction.clbit] =
                static_cast<bool>(applyQubitMeasurement(qubits_state, instruction.qubit));
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QuestSimulatorAdapter::apply_gate(const Copy& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::COPY:
            if (instruction.l_clbits.size() != instruction.r_clbits.size()) {
                throw std::runtime_error(
                    "The number of copied clbits and the number of clbits "
                    "copied on does not match."
                );
            }

            for (size_t i = 0; i < instruction.l_clbits.size(); ++i)
                creg[instruction.l_clbits[i]] = creg[instruction.r_clbits[i]];

            break;

        default:
            unsupported_gate(instruction);
    }
}


} // End of sim namespace
} // End of cunqa namespace