
#include "munich_simulator_adapter.hpp"
#include "munich_circuit_adapter.hpp"

#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <thread>
#include <functional>

#include "CircuitSimulator.hpp"
#include "StochasticNoiseSimulator.hpp"

#include "circuit.hpp"
#include "logger.hpp"

using namespace qc;

namespace {

const std::unordered_map<cunqa::InstructionType, OpType> MUNICH_INSTRUCTIONS_MAP = {
    // MEASURE
    {cunqa::InstructionType::MEASURE, OpType::Measure},

    // ONE QUBIT NO PARAM
    {cunqa::InstructionType::ID, OpType::I},
    {cunqa::InstructionType::X, OpType::X},
    {cunqa::InstructionType::Y, OpType::Y},
    {cunqa::InstructionType::Z, OpType::Z},
    {cunqa::InstructionType::H, OpType::H},
    {cunqa::InstructionType::S, OpType::S},
    {cunqa::InstructionType::SDG, OpType::Sdg},
    {cunqa::InstructionType::SX, OpType::SX},
    {cunqa::InstructionType::SXDG, OpType::SXdg},
    {cunqa::InstructionType::T, OpType::T},
    {cunqa::InstructionType::TDG, OpType::Tdg},
    {cunqa::InstructionType::V, OpType::V},
    {cunqa::InstructionType::VDG, OpType::Vdg},

    // ONE QUBIT ONE PARAM
    {cunqa::InstructionType::RX, OpType::RX},
    {cunqa::InstructionType::RY, OpType::RY},
    {cunqa::InstructionType::RZ, OpType::RZ},
    {cunqa::InstructionType::GLOBALP, OpType::GPhase},
    {cunqa::InstructionType::P, OpType::P},
    {cunqa::InstructionType::U1, OpType::P},

    // ONE QUBIT TWO PARAM
    {cunqa::InstructionType::U2, OpType::U2},

    // ONE QUBIT THREE PARAM 
    {cunqa::InstructionType::U3, OpType::U},

    // TWO QUBIT NO PARAM
    {cunqa::InstructionType::CX, OpType::X},
    {cunqa::InstructionType::CY, OpType::Y},
    {cunqa::InstructionType::CZ, OpType::Z},
    {cunqa::InstructionType::CH, OpType::H},
    {cunqa::InstructionType::CSX, OpType::SX},
    {cunqa::InstructionType::CS, OpType::S},
    {cunqa::InstructionType::CSDG, OpType::Sdg},
    {cunqa::InstructionType::SWAP, OpType::SWAP},
    {cunqa::InstructionType::ISWAP, OpType::iSWAP},
    {cunqa::InstructionType::ECR, OpType::ECR},
    {cunqa::InstructionType::DCX, OpType::DCX},

    // TWO QUBIT ONE PARAM
    {cunqa::InstructionType::CU1, OpType::P},
    {cunqa::InstructionType::CP, OpType::P},
    {cunqa::InstructionType::CRX, OpType::RX},
    {cunqa::InstructionType::CRY, OpType::RY},
    {cunqa::InstructionType::CRZ, OpType::RZ},
    {cunqa::InstructionType::RXX, OpType::RXX},
    {cunqa::InstructionType::RYY, OpType::RYY},
    {cunqa::InstructionType::RZZ, OpType::RZZ},
    {cunqa::InstructionType::RZX, OpType::RZX},
    {cunqa::InstructionType::XXMYY, OpType::XXminusYY},
    {cunqa::InstructionType::XXPYY, OpType::XXplusYY},

    // TWO QUBITS TWO PARAMS
    {cunqa::InstructionType::CU2, OpType::U2},

    // TWO QUBITS THREE PARAMS
    {cunqa::InstructionType::CU3, OpType::U},

    // THREE QUBITS NO PARAMS
    {cunqa::InstructionType::CSWAP, OpType::SWAP},
    
    // MULTICONTROLED NO PARAM
    {cunqa::InstructionType::MCX, OpType::X},

    // MULTICONTROLED PARAM
    {cunqa::InstructionType::MCP, OpType::P},

    // SPECIAL
    {cunqa::InstructionType::RESET, OpType::Reset},
    //{cunqa::InstructionType::BARRIER, OpType::Barrier},

};
 
inline void cunqa_circuit_to_mqt_circuit(const cunqa::MunichCircuit& circuit, QuantumComputation& mqt_circuit) 
{ 
    cunqa::InstructionType inst_type;
    for (auto& instruction : circuit.instructions) {

        std::vector<unsigned int> qubits = instruction.at("qubits").get<std::vector<unsigned int>>();

        switch (cunqa::instruction_type_from_name(instruction.at("name").get<std::string>()))
        {
            case cunqa::InstructionType::MEASURE:
            {
                mqt_circuit.emplace_back(std::make_unique<NonUnitaryOperation>(
                    instruction.at("qubits").get<std::vector<Qubit>>()[0], 
                    instruction.at("clbits").get<std::vector<Bit>>()[0]));
                break;
            }
            case cunqa::InstructionType::ID:
            case cunqa::InstructionType::X:
            case cunqa::InstructionType::Y:
            case cunqa::InstructionType::Z:
            case cunqa::InstructionType::H:
            case cunqa::InstructionType::S:
            case cunqa::InstructionType::SDG:
            case cunqa::InstructionType::SX:
            case cunqa::InstructionType::SXDG:
            case cunqa::InstructionType::T:
            case cunqa::InstructionType::TDG:
            case cunqa::InstructionType::V:
            case cunqa::InstructionType::VDG:
            {
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
                break;
            }
            case cunqa::InstructionType::RX:
            case cunqa::InstructionType::RY:
            case cunqa::InstructionType::RZ:
            case cunqa::InstructionType::GLOBALP:
            case cunqa::InstructionType::P:
            case cunqa::InstructionType::U1:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::U2:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::U3:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::U:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::ECR:
            case cunqa::InstructionType::SWAP:
            case cunqa::InstructionType::ISWAP:
            case cunqa::InstructionType::DCX:
            {
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
                break;
            }
            case cunqa::InstructionType::CX:
            case cunqa::InstructionType::CY:
            case cunqa::InstructionType::CZ:
            case cunqa::InstructionType::CH:
            case cunqa::InstructionType::CSX:
            case cunqa::InstructionType::CS:
            case cunqa::InstructionType::CSDG:
            case cunqa::InstructionType::CSWAP:
            {
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
                break;
            }
            case cunqa::InstructionType::RXX:
            case cunqa::InstructionType::RYY:
            case cunqa::InstructionType::RZZ:
            case cunqa::InstructionType::RZX:
            case cunqa::InstructionType::XXMYY:
            case cunqa::InstructionType::XXPYY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::CP:
            case cunqa::InstructionType::CRX:
            case cunqa::InstructionType::CRY:
            case cunqa::InstructionType::CRZ:
            case cunqa::InstructionType::CU1:
            case cunqa::InstructionType::CU2:
            case cunqa::InstructionType::CU3:
            case cunqa::InstructionType::CU:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(qubits[0], qubits[1], MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::MCX:
            {
                Controls controls(qubits.begin(), qubits.end() - 1);
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(controls, qubits.back(), MUNICH_INSTRUCTIONS_MAP.at(inst_type)));
                break;
            }
            case cunqa::InstructionType::MCP:
            {
                Controls controls(qubits.begin(), qubits.end() - 1);
                auto params = instruction.at("params").get<std::vector<double>>();
                mqt_circuit.emplace_back(std::make_unique<StandardOperation>(controls, qubits.back(), MUNICH_INSTRUCTIONS_MAP.at(inst_type), params));
                break;
            }
            case cunqa::InstructionType::RESET:
            {
                for (auto& qubit : qubits) {
                    mqt_circuit.reset(qubit);
                }
                break;
            }
            default:
            {
                LOGGER_ERROR("Gate {} not supported.", instruction.at("name").get<std::string>());
                break;
            }
        } // end switch 
    } // end for
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

std::unique_ptr<QuantumComputation> getQuantumComputation(int nQubits, int nClbits, int seed) {
    return std::make_unique<QuantumComputation>(nQubits, nClbits, seed); 
}

struct MunichSimulatorAdapter::State : public CircuitSimulator{
    using CircuitSimulator::CircuitSimulator; // Inherit all constructors from CircuitSimulator
    ~State() = default;

    inline void initializeSimulationAdapter(std::size_t nQubits) { initializeSimulation(nQubits); }
    inline void applyOperationToStateAdapter(std::unique_ptr<qc::Operation>&& op) { applyOperationToState(op); }
    inline void applyResetAdapter(NonUnitaryOperation& op) { reset(&op); }
    inline char measureAdapter(dd::Qubit i) { return measure(i); }
};

MunichSimulatorAdapter::MunichSimulatorAdapter()
    : state_(std::make_unique<State>(std::move(getQuantumComputation(num_qubits, config.num_clbits, config.seed))))
{ }
MunichSimulatorAdapter::~MunichSimulatorAdapter() = default;

std::unique_ptr<Circuit> MunichSimulatorAdapter::create_circuit(const JSON& instructions_json) const
{
    return std::make_unique<MunichCircuit>(instructions_json);
}

void MunichSimulatorAdapter::initialize() {
    const char* num_threads_char = std::getenv("OMP_NUM_THREADS");
    unsigned num_threads = 1;
    if (num_threads_char != nullptr) {
        num_threads = std::stoi(num_threads_char);
    }
    state_->initializeSimulationAdapter(num_qubits);

    size_t seed = config.seed;
}

void MunichSimulatorAdapter::clear()
{
    state_->initializeSimulationAdapter(num_qubits);
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
        case InstructionType::X:
        case InstructionType::Y:
        case InstructionType::Z:
        case InstructionType::H:
        case InstructionType::S:
        case InstructionType::SDG:
        case InstructionType::SX:
        case InstructionType::SXDG:
        case InstructionType::T:
        case InstructionType::TDG:
        case InstructionType::V:
        case InstructionType::VDG:
        {
            auto one_gate = std::make_unique<StandardOperation>(payload.qubit, MUNICH_INSTRUCTIONS_MAP.at(type));
            state_->applyOperationToStateAdapter(std::move(one_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::RX:
        case InstructionType::RY:
        case InstructionType::RZ:
        case InstructionType::GLOBALP:
        case InstructionType::P:
        case InstructionType::U1:
        case InstructionType::U2:
        case InstructionType::U3:
        case InstructionType::U:
        {
            std::vector<double> params = {*payload.param};
            auto param_one_gate = std::make_unique<StandardOperation>(payload.qubit, MUNICH_INSTRUCTIONS_MAP.at(type), params);
            state_->applyOperationToStateAdapter(std::move(param_one_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ECR:
        case InstructionType::SWAP:
        case InstructionType::ISWAP:
        case InstructionType::DCX:
        {
            Targets targets = {static_cast<unsigned int>(payload.qubits[0]), static_cast<unsigned int>(payload.qubits[1])};
            auto two_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(type));
            state_->applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case InstructionType::CX:
        case InstructionType::CY:
        case InstructionType::CZ:
        case InstructionType::CH:
        case InstructionType::CSX:
        case InstructionType::CS:
        case InstructionType::CSDG:
        case InstructionType::CSWAP:
        {
            Control control(payload.qubits[0]);
            auto two_gate = std::make_unique<StandardOperation>(control, payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type));
            state_->applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::RXX:
        case InstructionType::RYY:
        case InstructionType::RZZ:
        case InstructionType::RZX:
        {
            std::vector<double> params = {*payload.param};
            Targets targets = {static_cast<unsigned int>(payload.qubits[0]), static_cast<unsigned int>(payload.qubits[1])};
            auto two_1param_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(type), params);
            state_->applyOperationToStateAdapter(std::move(two_1param_gate));
            break;
        }
        case InstructionType::CP:
        case InstructionType::CRX:
        case InstructionType::CRY:
        case InstructionType::CRZ:
        case InstructionType::CU1:
        {
            std::vector<double> params = {*payload.param};
            auto two_1param_gate = std::make_unique<StandardOperation>(payload.qubits[0], payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type), params);
            state_->applyOperationToStateAdapter(std::move(two_1param_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitTwoParam& payload)
{
    switch (type)
    {
        case InstructionType::XXMYY:
        case InstructionType::XXPYY:
        {
            std::vector<double> vec_params;
            vec_params.reserve(payload.params.size());
            for (double* const ptr : payload.params) {
                vec_params.push_back(*ptr);
            }
            Targets targets = {static_cast<unsigned int>(payload.qubits[0]), static_cast<unsigned int>(payload.qubits[1])};

            auto two_2param_gate = std::make_unique<StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(type), vec_params);
            state_->applyOperationToStateAdapter(std::move(two_2param_gate));
            break;
        }
        case InstructionType::CU2:
        {
            std::vector<double> vec_params;
            vec_params.reserve(payload.params.size());
            for (double* const ptr : payload.params) {
                vec_params.push_back(*ptr);
            }
            auto two_2param_gate = std::make_unique<StandardOperation>(payload.qubits[0], payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type), vec_params);
            state_->applyOperationToStateAdapter(std::move(two_2param_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitThreeParam& payload)
{
    switch (type)
    {
        case InstructionType::CU3:
        {
            std::vector<double> vec_params;
            vec_params.reserve(payload.params.size());
            for (double* const ptr : payload.params) {
                vec_params.push_back(*ptr);
            }
            auto two_3param_gate = std::make_unique<StandardOperation>(payload.qubits[0], payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type), vec_params);
            state_->applyOperationToStateAdapter(std::move(two_3param_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitFourParam& payload)
{
    switch (type)
    {
        case InstructionType::CU:
        {
            std::vector<double> vec_params;
            vec_params.reserve(payload.params.size());
            for (double* const ptr : payload.params) {
                vec_params.push_back(*ptr);
            }
            auto two_4param_gate = std::make_unique<StandardOperation>(payload.qubits[0], payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type), vec_params);
            state_->applyOperationToStateAdapter(std::move(two_4param_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const ThreeQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CSWAP:
        {
            Control control(payload.qubits[0]);
            auto two_gate = std::make_unique<StandardOperation>(control, payload.qubits[1], MUNICH_INSTRUCTIONS_MAP.at(type));
            state_->applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const MultiNoParam& payload)
{
    switch (type)
    {
        case InstructionType::MCX:
        {
            Controls controls(payload.qubits.begin(), payload.qubits.end() - 1);
            auto m_gate = std::make_unique<StandardOperation>(controls, payload.qubits.back(), MUNICH_INSTRUCTIONS_MAP.at(type));
            state_->applyOperationToStateAdapter(std::move(m_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const MultiParam& payload)
{
    switch (type)
    {
        case InstructionType::MCP:
        {
            Controls controls(payload.qubits.begin(), payload.qubits.end() - 1);
            std::vector<double> vec_params;
            vec_params.reserve(payload.params.size());
            for (double* const ptr : payload.params) {
                vec_params.push_back(*ptr);
            }
            auto mparam_gate = std::make_unique<StandardOperation>(controls, payload.qubits.back(), MUNICH_INSTRUCTIONS_MAP.at(type), vec_params);
            state_->applyOperationToStateAdapter(std::move(mparam_gate));
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const Reset& payload)
{
    switch (type)
    {
        case InstructionType::RESET:
        {
            std::vector<unsigned int> target_qubits(payload.qubits.begin(), payload.qubits.end());
            NonUnitaryOperation reset(target_qubits); // Reset is the default of NonUnitaryOperation
            state_->applyResetAdapter(reset);
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
        {   
            creg[payload.clbit] =
                static_cast<bool>(state_->measureAdapter(payload.qubit));
            
            break;
        }
        
        default:
            unsupported_gate(type, payload);
    }
}

void MunichSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
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


JSON MunichSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    LOGGER_DEBUG("Munich native_execute");

    // TODO: Change the format with the free functions
    try {   
        auto mqt_circuit = std::make_unique<QuantumComputation>(num_qubits, config.num_clbits, seed); 

        auto& munich_adapter_circuit = dynamic_cast<const MunichCircuit&>(circuit);

        cunqa_circuit_to_mqt_circuit(munich_adapter_circuit, *mqt_circuit);
        
        float time_taken;
        if (!noise_model.empty()) {
            const ApproximationInfo approx_info{noise_model["step_fidelity"], noise_model["approx_steps"], ApproximationInfo::FidelityDriven};
            StochasticNoiseSimulator sim(std::move(mqt_circuit), approx_info, seed, "APD", noise_model["noise_prob"],
                                            noise_model["noise_prob_t1"], noise_model["noise_prob_multi"]); // "APD" selects all errors: Amplitude damping, Depolarization and Phase flip

            auto start = std::chrono::high_resolution_clock::now();
            auto result = sim.simulate(config.shots);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        } else {
            CircuitSimulator sim(std::move(mqt_circuit));

            auto start = std::chrono::high_resolution_clock::now();
            // TODO: Change this to directly call the simulate without creating a new instance?
            auto result = sim.simulate(config.shots);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        }
    } catch (const std::exception &e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator");   //: {}", circuit.instructions.dump());
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent."}};
    }
    return {}; // To avoid no-return warning
}

} // End of sim namespace
} // End of cunqa namespace