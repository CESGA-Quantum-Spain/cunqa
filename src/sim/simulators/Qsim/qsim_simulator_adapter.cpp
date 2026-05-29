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

using Matrix = std::vector<std::vector<std::complex<double>>>;
qsim::Matrix<float> cunqamatrix_to_qsimmatrix(const Matrix& cunqa_matrix)
{
    size_t n = cunqa_matrix.size();
    if (n == 0) return {};

    qsim::Matrix<float> qsim_mat;
    qsim_mat.resize(2 * n * n);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            const auto& complex_val = cunqa_matrix[i][j];
            
            size_t base_idx = 2 * (n * i + j);
            
            qsim_mat[base_idx]     = static_cast<float>(complex_val.real());
            qsim_mat[base_idx + 1] = static_cast<float>(complex_val.imag());
        }
    }

    return qsim_mat;
}

JSON circuit_to_QSIM(const cunqa::Circuit& circuit)
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

struct QsimSimulatorAdapter::State {
    qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space;
    qsim::SimulatorBasic<qsim::ParallelFor>::State state; 
    qsim::SimulatorBasic<qsim::ParallelFor> simulator;
    std::mt19937 rgen;

    // Constructor
    State(int num_threads, int num_qubits, unsigned seed) 
        : state_space(num_threads),
          state(state_space.Create(num_qubits)),
          simulator(num_threads),
          rgen(seed) { }
};

unsigned getNumThreads() {
    const char* env_value = std::getenv("OMP_NUM_THREADS");
    if (env_value != nullptr) {
        try {
            return std::stoi(env_value);
        } catch (const std::exception&) {
            // Fall back to default if conversion fails
            return 1;
        }
    }
    return 1;
}
unsigned processSeed(int seed){
    unsigned u_seed = (seed == -1) ? 0 : seed;
    return u_seed;
}

QsimSimulatorAdapter::QsimSimulatorAdapter()
    : state_(std::make_unique<State>(getNumThreads(), config.num_qubits, processSeed(config.seed)))
{ }
QsimSimulatorAdapter::~QsimSimulatorAdapter() = default;

void QsimSimulatorAdapter::initialize() {
    state_->state_space.SetStateZero(state_->state);
}

void QsimSimulatorAdapter::clear()
{
    state_->state_space.SetStateZero(state_->state);
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateId1<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::X:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateX<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::Y:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateY<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::Z:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateZ<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::H:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateHd<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::S:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateS<float>::Create(0, payload.qubit), state_->state);
            break;
        }
        
        case InstructionType::T:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateT<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::SX:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateX2<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::SY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateY2<float>::Create(0, payload.qubit), state_->state);
            break;
        }

        case InstructionType::HZ2:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateHZ2<float>::Create(0, payload.qubit), state_->state);
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::RX:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRX<float>::Create(0, payload.qubit, static_cast<float>(payload.param)), state_->state);
            break;
        }
        
        case InstructionType::RY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRY<float>::Create(0, payload.qubit, static_cast<float>(payload.param)), state_->state);
            break;
        }
        
        case InstructionType::RZ:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRZ<float>::Create(0, payload.qubit, static_cast<float>(payload.param)), state_->state);
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID2:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateId2<float>::Create(0, payload.qubits[0], payload.qubits[1]), state_->state);
            break;
        }

        case InstructionType::CX:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateCNot<float>::Create(0, payload.qubits[0], payload.qubits[1]), state_->state);
            break;
        }

        case InstructionType::CZ:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateCZ<float>::Create(0, payload.qubits[0], payload.qubits[1]), state_->state);
            break;
        }

        case InstructionType::SWAP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateSwap<float>::Create(0, payload.qubits[0], payload.qubits[1]), state_->state);
            break;
        }

        case InstructionType::ISWAP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateIS<float>::Create(0, payload.qubits[0], payload.qubits[1]), state_->state);
            break;
        }
                
        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::CP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateCP<float>::Create(0, payload.qubits[0], payload.qubits[1], payload.param), state_->state);
            break;
        }
        
        case InstructionType::GLOBALP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateGPh<float>::Create(0, payload.param), state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload)
{
    switch (type)
    {
        case InstructionType::RXY: //TODO: check if it's actually two-qubit
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRXY<float>::Create(0, payload.qubits[0], payload.params[0], payload.params[1]), state_->state);
            break;
        }

        case InstructionType::FS:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateFS<float>::Create(0, payload.qubits[0], payload.qubits[1], payload.params[0], payload.params[1]), state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const MatrixGate& payload)
{
    switch (type)
    {
        case InstructionType::UNITARY:
        {
            auto cunqa_matrix = payload.matrix;
            qsim::Matrix<float> qsim_matrix = cunqamatrix_to_qsimmatrix(cunqa_matrix);

            if (payload.qubits.size() > 1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateMatrix2<float>::Create(0, payload.qubits[0], payload.qubits[1], std::move(qsim_matrix)), state_->state);
            } else {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateMatrix1<float>::Create(0, payload.qubits[0], std::move(qsim_matrix)), state_->state);
            }
            break;
        }
        case InstructionType::CUNITARY:
        {
            auto cunqa_matrix = payload.matrix;
            size_t dim = cunqa_matrix.size();
            size_t ctrl_dim = 2 * dim;

            Matrix ctrl_cunqa_matrix(ctrl_dim,
                std::vector<std::complex<double>>(ctrl_dim, {0.0, 0.0}));

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
                state_->simulator,
                qsim::GateMatrix2<float>::Create(0, payload.qubits[0], payload.qubits[1], std::move(ctrl_qsim_matrix)),
                state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
        {
            auto measure_result = state_->state_space.Measure({payload.qubit}, state_->rgen, state_->state);
            creg[payload.clbit] = (measure_result.bitstring[0] == 1);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QsimSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
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


JSON QsimSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    //TODO: fix
    return {};
    /* LOGGER_DEBUG("Qsim usual simulation");
    JSON result;
    try {
        auto circuits = std::vector<std::shared_ptr<JSON>{
            std::make_shared<JSON>({
                {"instructions", circuit_to_QSIM(circuit)}
            })
        };

        this->initialize();
        
        std::vector<uint64_t> sample = state_->state_space.Sample(state_->state, config.shots, config.seed);

        result = convert_qsim_result(sample, config.num_clbits);
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator.\n\tTry checking the format of the circuit sent.");
        result = {{"ERROR", std::string(e.what())}};
    } 
    return result; */
}

} // End of sim namespace
} // End of cunqa namespace