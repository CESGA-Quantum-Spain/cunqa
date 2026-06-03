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
#include "qsim_circuit_adapter.hpp"

#include "seqfor.h"
#include "parfor.h"
#include <gates_qsim.h>
#include <gate_appl.h>
#include <simulator_basic.h>
/* #include <simulator_avx.h>
#include <simulator_sse.h> */

#include "utils/constants.hpp"
#include "utils/json.hpp"
#include "logger.hpp"

namespace {

using Matrix = std::vector<std::vector<std::vector<double>>>;
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
            
            qsim_mat[base_idx]     = static_cast<float>(complex_val[0]);
            qsim_mat[base_idx + 1] = static_cast<float>(complex_val[1]);
        }
    }

    return qsim_mat;
}

cunqa::JSON circuit_to_QSIM(const cunqa::Circuit& circuit)
{
    cunqa::JSON QSIM_circuit;

    // TODO: Circuit execution with Qsim not supported yet

    return QSIM_circuit;
}

void update_qsim_state(const cunqa::JSON& circuit_json, qsim::SimulatorBasic<qsim::ParallelFor>& simulator, qsim::SimulatorBasic<qsim::ParallelFor>::State& state)
{
    for (const auto& instruction : circuit_json) {
        auto inst_type = cunqa::instruction_type_from_name(instruction.at("name").get<std::string>());

        switch (inst_type) {
        case cunqa::InstructionType::MEASURE:
            LOGGER_DEBUG("Measure in Qsim usual simulation performed by sampling. Skiping.");
            break;
        case cunqa::InstructionType::ID:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId1<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::X:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::Y:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::Z:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateZ<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::H:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHd<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::S:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateS<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::T:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateT<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::SX:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateX2<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::SY:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateY2<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::HZ2:
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateHZ2<float>::Create(0, qubit), state);
            break;
        }
        case cunqa::InstructionType::RX: 
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            auto param = instruction.at("params").get<float>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRX<float>::Create(0, qubit, param), state);
            break;
        }
        case cunqa::InstructionType::RY: 
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            auto param = instruction.at("params").get<float>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRY<float>::Create(0, qubit, param), state);
            break;
        }
        case cunqa::InstructionType::RZ: 
        {
            auto qubit = instruction.at("qubits").get<unsigned>();
            auto param = instruction.at("params").get<float>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRZ<float>::Create(0, qubit, param), state);
            break;
        }
        case cunqa::InstructionType::ID2:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateId2<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case cunqa::InstructionType::CX:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCNot<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case cunqa::InstructionType::CZ:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCZ<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case cunqa::InstructionType::SWAP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateSwap<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case cunqa::InstructionType::ISWAP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateIS<float>::Create(0, qubits[0], qubits[1]), state);
            break;
        }
        case cunqa::InstructionType::CP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            auto param = instruction.at("params").get<float>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateCP<float>::Create(0, qubits[0], qubits[1], param), state);
            break;
        }
        case cunqa::InstructionType::RXY: 
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateRXY<float>::Create(0, qubits[0], params[0], params[1]), state);
            break;
        }
        case cunqa::InstructionType::FS:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateFS<float>::Create(0, qubits[0], qubits[1], params[0], params[1]), state);
            break;
        }
        case cunqa::InstructionType::GLOBALP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            auto params = instruction.at("params").get<std::vector<float>>();
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateGPh<float>::Create(0, params[0]), state);
            break;
        }
        case cunqa::InstructionType::UNITARY:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned>>();
            auto cunqa_matrix = instruction.at("matrix").get<std::vector<cunqa::Matrix>>()[0];
            qsim::Matrix<float> qsim_matrix = cunqamatrix_to_qsimmatrix(cunqa_matrix);

            if (qubits.size() > 1) {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix2<float>::Create(0, qubits[0], qubits[1], std::move(qsim_matrix)), state);
            } else {
                qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(simulator, qsim::GateMatrix1<float>::Create(0, qubits[0], std::move(qsim_matrix)), state);
            }
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << instruction_type_name(inst_type) << "\n";
        };
    }

}

cunqa::JSON convert_qsim_result(const std::vector<uint64_t>& sample, const int n_qubits) {
    std::unordered_map<uint64_t, int> counts;
    for (uint64_t v : sample)
        counts[v]++;

    cunqa::JSON result_json;
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
    State(int num_threads, const int& num_qubits, unsigned seed) 
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
    : state_(std::make_unique<State>(getNumThreads(), num_qubits, processSeed(config.seed)))
{ }
QsimSimulatorAdapter::~QsimSimulatorAdapter() = default;

std::unique_ptr<Circuit> QsimSimulatorAdapter::create_circuit(const JSON& instructions_json) const
{
    return std::make_unique<QsimCircuit>(instructions_json);
}

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
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRX<float>::Create(0, payload.qubit, static_cast<float>(*payload.param)), state_->state);
            break;
        }
        
        case InstructionType::RY:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRY<float>::Create(0, payload.qubit, static_cast<float>(*payload.param)), state_->state);
            break;
        }
        
        case InstructionType::RZ:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRZ<float>::Create(0, payload.qubit, static_cast<float>(*payload.param)), state_->state);
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
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateCP<float>::Create(0, payload.qubits[0], payload.qubits[1], *payload.param), state_->state);
            break;
        }
        
        case InstructionType::GLOBALP:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateGPh<float>::Create(0, *payload.param), state_->state);
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
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateRXY<float>::Create(0, payload.qubits[0], *payload.params[0], *payload.params[1]), state_->state);
            break;
        }

        case InstructionType::FS:
        {
            qsim::ApplyGate<qsim::SimulatorBasic<qsim::ParallelFor>, qsim::GateQSim<float>>(state_->simulator, qsim::GateFS<float>::Create(0, payload.qubits[0], payload.qubits[1], *payload.params[0], *payload.params[1]), state_->state);
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
            if(payload.clbit < config.num_clbits) {
                auto measure_result = state_->state_space.Measure({payload.qubit}, state_->rgen, state_->state);
                creg[payload.clbit] =
                        (measure_result.bitstring[0] == 1);
                save_clbit[payload.clbit] = payload.save;
            } else {
                throw std::runtime_error("Cannot store measurement: classical bit "
                                            "index exceeds the available range.");
            }    
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


JSON QsimSimulatorAdapter::native_execute(const Circuit& circuit)
{
    LOGGER_DEBUG("Qsim native execute");
    try {
        auto& qsim_adapter_circuit = dynamic_cast<const QsimCircuit&>(circuit);

        auto shots = config.shots;
        const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
        unsigned num_threads = 1;
        if (num_threads_char != nullptr) {
            num_threads = std::stoi(num_threads_char);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        qsim::StateSpaceBasic<qsim::ParallelFor, float> state_space(num_threads);
        qsim::SimulatorBasic<qsim::ParallelFor>::State state = state_space.Create(num_qubits); 
        state_space.SetStateZero(state);
        qsim::SimulatorBasic<qsim::ParallelFor> simulator(num_threads);
        
        update_qsim_state(qsim_adapter_circuit.instructions, simulator, state);
        std::vector<uint64_t> results = state_space.Sample(state, shots, config.seed);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts_json = convert_qsim_result(results, config.num_clbits);
        
        JSON result_json = {
            {"counts", counts_json},
            {"time_taken", time_taken}};

        return result_json;
    } 
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Qsim simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return JSON();
}

} // End of sim namespace
} // End of cunqa namespace