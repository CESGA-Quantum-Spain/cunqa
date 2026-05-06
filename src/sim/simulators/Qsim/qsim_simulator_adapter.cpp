#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>
#include <random>

#include "qsim_simulator_adapter.hpp"

#include "seqfor.h"
#include "parfor.h"
#include <gates_qsim.h>
#include <gate_appl.h>
#include <simulator_basic.h>
/* #include <simulator_avx.h>
#include <simulator_sse.h> */

#include "utils/constants.hpp"

#include "logger.hpp"

namespace {
using namespace cunqa;

qsim::Matrix<float> cunqamatrix_to_qsimmatrix(const CUNQAMatrix& cunqa_matrix)
{
    size_t n = cunqa_matrix.size();
    if (n == 0) return {};

    qsim::Matrix<float> qsim_mat;
    qsim_mat.resize(2 * n * n);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            const auto& complex_val = cunqa_matrix[i][j];
            
            size_t base_idx = 2 * (n * i + j);

            if (complex_val.size() >= 2) {
                qsim_mat[base_idx]     = static_cast<float>(complex_val[0]); // Real
                qsim_mat[base_idx + 1] = static_cast<float>(complex_val[1]); // Imag
            } else if (complex_val.size() == 1) {
                qsim_mat[base_idx]     = static_cast<float>(complex_val[0]);
                qsim_mat[base_idx + 1] = 0.0f;
            }
        }
    }

    return qsim_mat;
}

cunqa::JSON circuit_to_QSIM(const cunqa::Circuit& circuit)
{
    cunqa::JSON QSIM_circuit;

    // TODO

    return QSIM_circuit;
}

JSON convert_qsim_result(const std::vector<uint64_t>& sample, const int n_qubits) {
    std::unordered_map<uint64_t, int> counts;
    for (uint64_t v : sample)
        counts[v]++;

    JSON result_json;
    for (const auto& [value, count] : counts) {
        std::string bitstring(n_qubits, '0');
        for (int i = 0; i < n_qubits; ++i)
            bitstring[n_qubits - 1 - i] = ((value >> i) & 1) ? '1' : '0';

        result_json[bitstring] = count;
    }
    return result_json;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

QsimSimulatorAdapter::QuestSimulatorAdapter() = default;
QsimSimulatorAdapter::~QuestSimulatorAdapter() = default;

void QsimSimulatorAdapter::initialize() {
    const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
    unsigned num_threads = 1;
    if (num_threads_char != nullptr) {
        num_threads = std::stoi(num_threads_char);
    }
    qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
    qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(config.num_qubits); 
    state_space.SetStateZero(state);
    qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);

    if (config.seed != -1) {
        std::mt19937 rgen(config.seed);
    } else {
        std::mt19937 rgen(0);
    }
}

void QsimSimulatorAdapter::clear()
{
    state_space.SetStateZero(state);
}

void QsimSimulatorAdapter::apply_gate(const OneQubitNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::ID:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId1<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::X:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::Y:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::Z:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::H:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::S:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateS<float>::Create(0, intruction.qubit), state);
            break;
        
        case InstructionTag::T:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateT<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::SX:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX2<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::SY:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY2<float>::Create(0, intruction.qubit), state);
            break;

        case InstructionTag::HZ2:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHZ2<float>::Create(0, intruction.qubit), state);
            break;
        
        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const OneQubitOneParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::RX: 
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRX<float>::Create(0, instruction.qubit, static_cast<float>(instruction.param)), state);
            break;
        
        case InstructionTag::RY:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRY<float>::Create(0, instruction.qubit, static_cast<float>(instruction.param)), state);
            break;
        
        case InstructionTag::RZ: 
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRZ<float>::Create(0, instruction.qubit, static_cast<float>(instruction.param)), state);
            break;
        
        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const TwoQubitNoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::ID2:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId2<float>::Create(0, instruction.qubits[0], instruction.qubits[1]), state);
            break;

        case InstructionTag::CX:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, instruction.qubits[0], instruction.qubits[1]), state);
            break;

        case InstructionTag::CZ:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCZ<float>::Create(0, instruction.qubits[0], instruction.qubits[1]), state);
            break;

        case InstructionTag::SWAP:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateSwap<float>::Create(0, instruction.qubits[0], instruction.qubits[1]), state);
            break;

        case InstructionTag::ISWAP:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateIS<float>::Create(0, instruction.qubits[0], instruction.qubits[1]), state);
            break;
                
        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const TwoQubitOneParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::CP:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCP<float>::Create(0, instruction.qubits[0], instruction.qubits[1], instruction.param), state);
            break;
        
        case InstructionTag::GLOBALP:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateGPh<float>::Create(0, instruction.param), state);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const TwoQubitTwoParam& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::RXY: 
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRXY<float>::Create(0, instruction.qubits[0], instruction.params[0], instruction.params[1]), state);
            break;

        case InstructionTag::FS:
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateFS<float>::Create(0, instruction.qubits[0], instruction.qubits[1], instruction.params[0], instruction.params[1]), state);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const MatrixGate& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::UNITARY:
        {
            auto cunqa_matrix = instruction.matrix;
            qsim::Matrix<float> qsim_matrix = cunqamatrix_to_qsimmatrix(cunqa_matrix);

            if (qubits.size() > 1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix2<float>::Create(0, instruction.qubits[0], instruction.qubits[1], std::move(qsim_matrix)), state);
            } else {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix1<float>::Create(0, instruction.qubits[0], std::move(qsim_matrix)), state);
            }
            break;
        }
        case constants::CUNITARY:
        {
            auto cunqa_matrix = instruction.matrix;
            size_t dim = cunqa_matrix.size();
            size_t ctrl_dim = 2 * dim;

            CUNQAMatrix ctrl_cunqa_matrix(ctrl_dim,
                std::vector<std::vector<double>>(ctrl_dim, {0.0, 0.0}));

            for (size_t i = 0; i < dim; i++) {
                ctrl_cunqa_matrix[i][i] = {1.0, 0.0};
            }

            for (size_t i = 0; i < dim; i++) {
                for (size_t j = 0; j < dim; j++) {
                    ctrl_cunqa_matrix[dim + i][dim + j] = cunqa_matrix[i][j]; 
                }
            }

            qsim::Matrix<float> ctrl_qsim_matrix = cunqamatrix_to_qsimmatrix(ctrl_cunqa_matrix);

            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(
                simulator,
                qsim::GateMatrix2<float>::Create(0, instruction.qubits[0], instruction.qubits[1], std::move(ctrl_qsim_matrix)),
                state);
            break;
        }

        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const Measure& instruction)
{
    switch (instruction.tag)
    {
        case InstructionTag::MEASURE:
            creg[instruction.clbit] =
                static_cast<bool>(state_space.Measure({instruction.qubit}, rgen, state));
            break;

        default:
            unsupported_gate(instruction);
    }
}

void QsimSimulatorAdapter::apply_gate(const Copy& instruction)
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


JSON QsimSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    LOGGER_DEBUG("Qsim usual simulation");
    JSON result;
    try {
        auto circuits = std::vector<std::shared_ptr<JSON>{
            std::make_shared<JSON>({
                {"instructions", circuit_to_QSIM(circuit)}
            })
        };

        this->initialize();
        
        std::vector<uint64_t> sample = state_space.Sample(state, config.shots, config.seed);

        result = convert_qsim_result(sample, config.num_clbits);
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator.\n\tTry checking the format of the circuit sent.");
        result = {{"ERROR", std::string(e.what())}};
    } 
    return result;
}

} // End of sim namespace
} // End of cunqa namespace