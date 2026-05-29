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

#include "utils/constants.hpp"
#include "logger.hpp"

namespace {
using namespace cunqa;

std::vector<std::vector<qcomp>> cunqamatrix_to_questmatrix(const Matrix& cunqa_matrix)
{
    size_t n = cunqa_matrix.size();
    if (n == 0) return {};

    std::vector<std::vector<qcomp>> quest_mat;

    for (const auto& row : cunqa_matrix) {
        std::vector<qcomp> complexRow;
        for (const auto& complex : row) {
            complexRow.emplace_back(complex.real(), complex.imag());
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

Qureg init_qureg(int& num_qubits, std::string& method, JSON& device) {
    int vec_or_mat;
    if (method == "statevector" || method == "automatic"){
        vec_or_mat = 0;
    } else if (method == "density_matrix") {
        vec_or_mat = 1;
    } else {
        LOGGER_ERROR("QuEST simulator only supports statevector or density matrix simulation, while {} was given", method);
        throw std::invalid_argument{"QuEST simulator only supports statevector or density matrix simulation"};
    }
    const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
    unsigned num_threads = 1;
    if (num_threads_char != nullptr) {
        num_threads = std::stoi(num_threads_char);
    }
    int useMultithread = (num_threads > 1) ? 1 : 0;
    int useGpuAccel = (device["device_name"] == "GPU") ? 1 : 0;
    if (!isQuESTEnvInit()) {
        initCustomQuESTEnv(0, useGpuAccel, useMultithread);
    }

    return createCustomQureg(num_qubits, vec_or_mat, 0, useGpuAccel, useMultithread);
}


struct QuestSimulatorAdapter::State {
    Qureg qubits_state;

    // Constructor
    State(int& num_qubits, std::string& method, JSON& device) : qubits_state(init_qureg(num_qubits, method, device)) { }
};

QuestSimulatorAdapter::QuestSimulatorAdapter()
: state_(std::make_unique<State>(config.num_qubits, config.method, config.device))
{ }
QuestSimulatorAdapter::~QuestSimulatorAdapter() = default;

void QuestSimulatorAdapter::initialize() {
    initZeroState(state_->qubits_state);
}

void QuestSimulatorAdapter::clear() {
    initZeroState(state_->qubits_state);
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
            break;

        case InstructionType::X:
            applyPauliX(state_->qubits_state, payload.qubit);
            break;

        case InstructionType::Y:
            applyPauliY(state_->qubits_state, payload.qubit);
            break;

        case InstructionType::Z:
            applyPauliZ(state_->qubits_state, payload.qubit);
            break;

        case InstructionType::H:
            applyHadamard(state_->qubits_state, payload.qubit);
            break;

        case InstructionType::S:
            applyS(state_->qubits_state, payload.qubit);
            break;
        
        case InstructionType::T:
            applyT(state_->qubits_state, payload.qubit);
            break;
        
        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::P:
            applyPhaseShift(state_->qubits_state, payload.qubit, payload.param);
            break;

        case InstructionType::RX:
            applyRotateX(state_->qubits_state, payload.qubit, payload.param);
            break;

        case InstructionType::RY:
            applyRotateY(state_->qubits_state, payload.qubit, payload.param);
            break;

        case InstructionType::RZ:
            applyRotateZ(state_->qubits_state, payload.qubit, payload.param);
            break;
        
        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitFourParam& payload)
{
    switch (type)
    {
        case InstructionType::RAXIS:
            applyRotateAroundAxis(
                state_->qubits_state, 
                payload.qubit, 
                payload.params[0],
                payload.params[1], //axis
                payload.params[2], //axis 
                payload.params[3]  //axis
            );
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::SWAP:
            applySwap(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;
        
        case InstructionType::SQRTSWAP:
            applySqrtSwap(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        case InstructionType::CX:
            applyControlledPauliX(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        case InstructionType::CY:
            applyControlledPauliY(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        case InstructionType::CZ:
            applyControlledPauliZ(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        case InstructionType::CH:
            applyControlledHadamard(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        case InstructionType::CS:
            applyControlledS(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;
        
        case InstructionType::CT:
            applyControlledT(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back());
            break;

        
        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::CP:
            applyTwoQubitPhaseShift(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back(), payload.param);
            break;

        case InstructionType::CRX:
            applyControlledRotateX(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back(), payload.param);
            break;

        case InstructionType::CRY:
            applyControlledRotateY(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back(), payload.param);
            break;

        case InstructionType::CRZ:
            applyControlledRotateZ(state_->qubits_state, *payload.qubits.begin(), payload.qubits.back(), payload.param);
            break;

        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitFourParam& payload)
{
    switch (type)
    {
        case InstructionType::CRAXIS:
            applyControlledRotateAroundAxis(
                state_->qubits_state,
                *payload.qubits.begin(),
                payload.qubits.back(),
                payload.params[0],
                payload.params[1], //axis
                payload.params[2], //axis 
                payload.params[3]  //axis
            );
            break;

        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CSWAP:
            applyControlledSwap(state_->qubits_state, payload.qubits[0], payload.qubits[1], payload.qubits[2]);
            break;

        case InstructionType::CSQRTSWAP:
            applyControlledSqrtSwap(state_->qubits_state, payload.qubits[0], payload.qubits[1], payload.qubits[2]);
            break;

        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const PauliNoParam& payload)
{
    switch (type)
    {
        case InstructionType::PAULISTR:
            applyPauliStr(state_->qubits_state, getPauliStr(payload.paulistr));
            break;

        case InstructionType::CPAULISTR:   
            applyControlledPauliStr(state_->qubits_state, payload.qubits[0], getPauliStr(payload.paulistr));
            break;

        case InstructionType::MCPAULISTR:
            applyMultiControlledPauliStr(state_->qubits_state, payload.qubits, getPauliStr(payload.paulistr));
            break;
        

        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const PauliParam& payload)
{
    switch (type)
    {
        case InstructionType::PAULIGADGET:
            applyPauliGadget(state_->qubits_state, getPauliStr(payload.paulistr), payload.param);
            break;

        case InstructionType::NONUNITARYPAULIGADGET:
            applyNonUnitaryPauliGadget(state_->qubits_state, getPauliStr(payload.paulistr), payload.param);
            break;

        case InstructionType::CPAULIGADGET:
            applyControlledPauliGadget(state_->qubits_state, payload.qubits[0], getPauliStr(payload.paulistr), payload.param);
            break;

        case InstructionType::MCPAULIGADGET:
            applyMultiControlledPauliGadget(state_->qubits_state, payload.qubits, getPauliStr(payload.paulistr), payload.param);
            break;

        default:
            unsupported_gate(type, payload);
    }
}


void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const MultiNoParam& payload)
{
    switch (type)
    {
        case InstructionType::MCX:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledPauliX(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCY:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledPauliY(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCZ:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledPauliZ(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCH:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledHadamard(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCS:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledS(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCT:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledT(state_->qubits_state, controls, payload.qubits.back());
            break;
        }

        case InstructionType::MCSWAP:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-2);
            applyMultiControlledSwap(state_->qubits_state, controls, *payload.qubits.end()-1, payload.qubits.back());
            break;
        }

        case InstructionType::MCSQRTSWAP:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-2);
            applyMultiControlledSqrtSwap(state_->qubits_state, controls, *payload.qubits.end()-1, payload.qubits.back());
            break;
        }

        case InstructionType::MX:
        {
            applyMultiQubitNot(state_->qubits_state, payload.qubits);
            break;
        }

        case InstructionType::CMX:
        {
            std::vector<int> targets(payload.qubits.begin()+1, payload.qubits.end());
            applyControlledMultiQubitNot(state_->qubits_state, payload.qubits[0], targets);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const MultiParam& payload)
{
    switch (type)
    {
        case InstructionType::MCRX:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledRotateX(state_->qubits_state, controls, payload.qubits.back(), payload.params[0]);
            break;
        }

        case InstructionType::MCRY:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledRotateY(state_->qubits_state, controls, payload.qubits.back(), payload.params[0]);
            break;
        }

        case InstructionType::MCRZ:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledRotateZ(state_->qubits_state, controls, payload.qubits.back(), payload.params[0]);
            break;
        }

        case InstructionType::MCP:
        {
            applyMultiQubitPhaseShift(state_->qubits_state, payload.qubits, payload.params[0]);
            break;
        }

        case InstructionType::MCRAXIS:
        {
            std::vector<int> controls(payload.qubits.begin(), payload.qubits.end()-1);
            applyMultiControlledRotateAroundAxis(
                state_->qubits_state,
                controls,
                payload.qubits.back(),
                payload.params[0],
                payload.params[1], //axis
                payload.params[2], //axis
                payload.params[3]  //axis
            );
            break;
        }

        case InstructionType::PHASEGADGET:
        {
            applyPhaseGadget(state_->qubits_state, payload.qubits, payload.params[0]);
            break;
        }

        case InstructionType::CPHASEGADGET:
        {
            std::vector<int> targets(payload.qubits.begin() + 1, payload.qubits.end());
            applyControlledPhaseGadget(state_->qubits_state, payload.qubits[0], targets, payload.params[0]);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const NumControlsNoParam& payload)
{
    switch (type)
    {
        case InstructionType::MCMX:
        {
            std::vector<int> controls(payload.qubits.begin(),                           payload.qubits.begin() + payload.num_controls + 1);
            std::vector<int> targets(payload.qubits.begin() + payload.num_controls + 1, payload.qubits.end());
            applyMultiControlledMultiQubitNot(state_->qubits_state, controls, targets);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const NumControlsParam& payload)
{
    switch (type)
    {
        case InstructionType::MCPHASEGADGET:
            {
                std::vector<int> controls(payload.qubits.begin(), payload.qubits.begin() + payload.num_controls + 1);
                std::vector<int> targets(payload.qubits.begin() + payload.num_controls + 1, payload.qubits.end());
                applyMultiControlledPhaseGadget(state_->qubits_state, controls, targets, payload.param);
                break;
            }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const MatrixGate& payload)
{
    switch (type)
    {
        case InstructionType::UNITARY:
        {
            auto cunqa_matrix = payload.matrix;
            CompMatr quest_matrix = createCompMatr(payload.qubits.size());
            // Using this constructor setCompMatr(CompMatr out, std::vector<std::vector<qcomp>> in);
            setCompMatr(quest_matrix, cunqamatrix_to_questmatrix(cunqa_matrix));
            
            applyCompMatr(state_->qubits_state, payload.qubits, quest_matrix); //payload.qubits must be std::vector<int>
            break;
        }
        case InstructionType::CUNITARY:
        {
            auto cunqa_matrix = payload.matrix;
            CompMatr quest_matrix = createCompMatr(payload.qubits.size() - 1);
            // Using this constructor setCompMatr(CompMatr out, std::vector<std::vector<qcomp>> in);
            setCompMatr(quest_matrix, cunqamatrix_to_questmatrix(cunqa_matrix));
            
            std::vector<int> targets(payload.qubits.begin() + 1, payload.qubits.end());
            applyControlledCompMatr(state_->qubits_state, payload.qubits[0], targets, quest_matrix);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
        {
            creg[payload.clbit] =
                static_cast<bool>(applyQubitMeasurement(state_->qubits_state, payload.qubit));
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QuestSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
{
    switch (type)
    {
        case InstructionType::COPY:
        {
            if (payload.l_clbits.size() != payload.r_clbits.size()) {
                throw std::runtime_error(
                    "The number of copied clbits and the number of clbits "
                    "copied on does not match."
                );
            }

            for (size_t i = 0; i < payload.l_clbits.size(); ++i)
                creg[payload.l_clbits[i]] = creg[payload.r_clbits[i]];

            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

JSON QuestSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model){
    //TODO: Implement
    return {};
}

} // End of sim namespace
} // End of cunqa namespace