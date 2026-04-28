#include <array>
#include <variant>
#include <string_view>

// AER dependecies
#include "simulators/circuit_executor.hpp"
#include "framework/config.hpp"
#include "framework/circuit.hpp"
#include "framework/results/result.hpp"
#include "controllers/state_controller.hpp"
#include "controllers/controller_execute.hpp"
#include "controllers/aer_controller.hpp"
#include "noise/noise_model.hpp"

#include "aer_simulator_adapter.hpp"
#include "utils/json.hpp"

#include "logger.hpp"

namespace {

constexpr std::array<std::string_view, 68> AER_config_KEYS = {{
    "shots",
    "method",
    "precision",
    "enable_truncation",
    "zero_threshold",
    "validation_threshold",
    "max_parallel_threads",
    "max_parallel_experiments",
    "max_parallel_shots",
    "fusion_enable",
    "fusion_verbose",
    "fusion_max_qubit",
    "fusion_threshold",
    "accept_distributed_results",
    "memory",
    "cuStateVec_enable",
    "blocking_qubits",
    "blocking_enable",
    "chunk_swap_buffer_qubits",
    "batched_shots_gpu",
    "batched_shots_gpu_max_qubits",
    "num_threads_per_device",
    "shot_branching_enable",
    "shot_branching_sampling_enable",
    "statevector_parallel_threshold",
    "statevector_sample_measure_opt",
    "stabilizer_max_snapshot_probabilities",
    "extended_stabilizer_sampling_method",
    "extended_stabilizer_metropolis_mixing_time",
    "extended_stabilizer_approximation_error",
    "extended_stabilizer_norm_estimation_samples",
    "extended_stabilizer_norm_estimation_repetitions",
    "extended_stabilizer_parallel_threshold",
    "extended_stabilizer_probabilities_snapshot_samples",
    "matrix_product_state_truncation_threshold",
    "matrix_product_state_max_bond_dimension",
    "mps_sample_measure_algorithm",
    "mps_log_data",
    "mps_swap_direction",
    "chop_threshold",
    "mps_parallel_threshold",
    "mps_omp_threads",
    "mps_lapack",
    "tensor_network_num_sampling_qubits",
    "use_cuTensorNet_autotuning",
    "parameterizations",
    "library_dir",
    "global_phase",
    "_parallel_experiments",
    "_parallel_shots",
    "_parallel_state_update",
    "fusion_allow_kraus",
    "fusion_allow_superop",
    "fusion_parallelization_threshold",
    "_fusion_enable_n_qubits",
    "_fusion_enable_n_qubits_1",
    "_fusion_enable_n_qubits_2",
    "_fusion_enable_n_qubits_3",
    "_fusion_enable_n_qubits_4",
    "_fusion_enable_n_qubits_5",
    "_fusion_enable_diagonal",
    "_fusion_min_qubit",
    "fusion_cost_factor",
    "superoperator_parallel_threshold",
    "unitary_parallel_threshold",
    "memory_blocking_bits",
    "extended_stabilizer_norm_estimation_default_samples",
    "runtime_parameter_bind_enable",
}};

AER::Config config_to_AER(const cunqa::RunConfig& config)
{
    cunqa::JSON AER_config = {
        {"shots", config.shots},
        {"method", config.method},
        {"avoid_parallelization", config.avoid_parallelization},
        {"num_clbits", config.num_clbits},
        {"num_qubits", config.num_qubits}
    };

    // Generic Aer configuration options
    for (auto& [key, value] : config.simulator_specifics.items()) {
        if (std::find(AER_config_KEYS.begin(), AER_config_KEYS.end(), key) != AER_config_KEYS.end()) {
            AER_config[std::string(key)] = value;
        }
    }
    
    // Seed
    if (config.seed != cunqa::NO_SEED)
        AER_config["seed_simulator"] = config.seed;

    // Device (CPU or GPU)
    AER_config["device"] = config.device["device_name"];
    if(AER_config["device"] == "GPU")
        AER_config["target_gpus"] = config.device["target_devices"];
    
    // memory_slots = num_clbits
    AER_config["memory_slots"] = config.num_clbits;;

    // Avoid parallelization. Not recommended.
    if (config.avoid_parallelization)
        AER_config["max_parallel_threads"] = 1;
    
    return AER::Config(AER_config);
}

cunqa::JSON circuit_to_AER(const cunqa::Circuit& circuit)
{
    cunqa::JSON AER_circuit;

    // TODO

    return AER_circuit;
}

void AER_to_results(cunqa::JSON& res, const int& num_clbits) 
{
    cunqa::JSON counts = res.at("results")[0].at("data").at("counts");
    cunqa::JSON modified_counts;

    for (const auto& [key, inner] : counts.items()) {
        // Remove "0x" prefix if present
        std::string hex_key = key;
        if (hex_key.rfind("0x", 0) == 0) {
            hex_key = hex_key.substr(2);
        }

        // Convert hex string to unsigned long long (support up to 100 bits)
        // Use std::bitset<100> for binary conversion
        std::bitset<100> bits(0);
        size_t hex_len = hex_key.length();
        // Convert hex to binary manually
        for (size_t i = 0; i < hex_len; ++i) {
            char c = hex_key[hex_len - 1 - i];
            int value = 0;
            if (c >= '0' && c <= '9') value = c - '0';
            else if (c >= 'a' && c <= 'f') value = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') value = 10 + (c - 'A');
            for (int j = 0; j < 4; ++j) {
                if ((value >> j) & 1) {
                    size_t bit_pos = i * 4 + j;
                    if (bit_pos < 100) bits.set(bit_pos);
                }
            }
        }

        // Get binary string with num_clbits bits, reversed to match Qiskit/AER convention
        std::string binary_string;
        for (int i = num_clbits - 1; i >= 0; --i) {
            binary_string += bits[i] ? '1' : '0';
        }

        modified_counts[binary_string] = inner; 
    }

    res.at("results")[0].at("data").at("counts") = modified_counts;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

struct AerSimulatorAdapter::State {
    AER::AerState aer_state;
};

AerSimulatorAdapter::AerSimulatorAdapter()
    : state_(std::make_unique<State>())
{ }

AerSimulatorAdapter::~AerSimulatorAdapter() = default;

void AerSimulatorAdapter::initialize()
{
    creg = std::vector<bool>(config.num_clbits, false);

    state_->aer_state.set_method((config.method == "automatic") ? "statevector" : config.method);
    state_->aer_state.set_device(config.device.at("device_name").get<std::string>());
    state_->aer_state.set_precision("double");
    config.seed != NO_SEED ? state_->aer_state.set_seed(config.seed) : state_->aer_state.set_random_seed();

    state_->aer_state.allocate_qubits(config.num_qubits);
    state_->aer_state.initialize();
}

void AerSimulatorAdapter::clear()
{
    state_->aer_state.clear();
}

void AerSimulatorAdapter::apply_gate(const OneQubitNoParam& instruction)
{
    auto qubit = static_cast<AER::uint_t>(instruction.qubit);
    switch (instruction.tag)
    {
        case InstructionTag::ID:
            break;

        case InstructionTag::X:
            state_->aer_state.apply_x(qubit);
            break;

        case InstructionTag::Y:
            state_->aer_state.apply_y(qubit);
            break;

        case InstructionTag::Z:
            state_->aer_state.apply_z(qubit);
            break;

        case InstructionTag::H:
            state_->aer_state.apply_h(qubit);
            break;

        case InstructionTag::SX:
            state_->aer_state.apply_mcsx({qubit});
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const OneQubitOneParam& instruction)
{
    AER::reg_t qubit = {static_cast<AER::uint_t>(instruction.qubit)};
    switch (instruction.tag)
    {
        case InstructionTag::RX:
            state_->aer_state.apply_mcrx(qubit, instruction.param);
            break;

        case InstructionTag::RY:
            state_->aer_state.apply_mcry(qubit, instruction.param);
            break;

        case InstructionTag::RZ:
            state_->aer_state.apply_mcrz(qubit, instruction.param);
            break;

        case InstructionTag::GLOBALP:
            state_->aer_state.apply_global_phase(instruction.param);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const OneQubitThreeParam& instruction)
{
    auto qubit = static_cast<AER::uint_t>(instruction.qubit);
    switch (instruction.tag)
    {
        case InstructionTag::U3:
            state_->aer_state.apply_u(
                qubit,
                instruction.params[0],
                instruction.params[1],
                instruction.params[2]
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const TwoQubitNoParam& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::SWAP:
            state_->aer_state.apply_mcswap(qubits);
            break;

        case InstructionTag::CX:
            state_->aer_state.apply_mcx(qubits);
            break;

        case InstructionTag::CY:
            state_->aer_state.apply_mcy(qubits);
            break;

        case InstructionTag::CZ:
            state_->aer_state.apply_mcz(qubits);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const TwoQubitOneParam& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::CRX:
            state_->aer_state.apply_mcrx(qubits, instruction.param);
            break;

        case InstructionTag::CRY:
            state_->aer_state.apply_mcry(qubits, instruction.param);
            break;

        case InstructionTag::CRZ:
            state_->aer_state.apply_mcrz(qubits, instruction.param);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const TwoQubitFourParam& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::CU:
            state_->aer_state.apply_cu(
                qubits,
                instruction.params[0],
                instruction.params[1],
                instruction.params[2],
                instruction.params[3]
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const MulticontrolNoParam& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::MCX:
            state_->aer_state.apply_mcx(qubits);
            break;

        case InstructionTag::MCY:
            state_->aer_state.apply_mcy(qubits);
            break;

        case InstructionTag::MCZ:
            state_->aer_state.apply_mcz(qubits);
            break;

        case InstructionTag::MCSX:
            state_->aer_state.apply_mcsx(qubits);
            break;

        case InstructionTag::MCSWAP:
            state_->aer_state.apply_mcswap(qubits);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const MulticontrolParam& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::MCRX:
            state_->aer_state.apply_mcrx(qubits, instruction.params[0]);
            break;

        case InstructionTag::MCRY:
            state_->aer_state.apply_mcry(qubits, instruction.params[0]);
            break;

        case InstructionTag::MCRZ:
            state_->aer_state.apply_mcrz(qubits, instruction.params[0]);
            break;

        case InstructionTag::MCP:
            state_->aer_state.apply_mcphase(qubits, instruction.params[0]);
            break;

        case InstructionTag::MCU:
            state_->aer_state.apply_mcu(
                qubits,
                instruction.params[0],
                instruction.params[1],
                instruction.params[2],
                instruction.params[3]
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const MatrixGate& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::UNITARY:
        {
            std::vector<complex_t> matrix_data;

            for (const auto& row : instruction.matrix)
                matrix_data.insert(matrix_data.end(), row.begin(), row.end());

            const auto dim = instruction.matrix.size();

            matrix<complex_t> aer_matrix{
                dim,
                dim,
                matrix_data.data()
            };

            state_->aer_state.apply_unitary(qubits, aer_matrix);
            break;
        }

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const DiagonalMatrixGate& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::DIAGONAL:
            state_->aer_state.apply_diagonal_matrix(
                qubits,
                instruction.matrix
            );
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const Measure& instruction)
{
    auto qubit = static_cast<AER::uint_t>(instruction.qubit);
    switch (instruction.tag)
    {
        case InstructionTag::MEASURE:
            creg[instruction.clbit] =
                static_cast<bool>(state_->aer_state.apply_measure({qubit}));
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const Reset& instruction)
{
    AER::reg_t qubits(instruction.qubits.begin(), instruction.qubits.end());
    switch (instruction.tag)
    {
        case InstructionTag::RESET:
            state_->aer_state.apply_reset(qubits);
            break;

        default:
            unsupported_gate(instruction);
    }
}

void AerSimulatorAdapter::apply_gate(const Copy& instruction)
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

JSON AerSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    JSON result;
    try {
        auto circuits = std::vector<std::shared_ptr<AER::Circuit>>{
            std::make_shared<AER::Circuit>(JSON({
                {"instructions", circuit_to_AER(circuit)}
            }))
        };
        auto AER_config = config_to_AER(config);
        AER::Noise::NoiseModel noise_model(noise_model);

        auto AER_result = controller_execute<AER::Controller>(circuits, noise_model, AER_config);
        result = AER_result.to_json();

        AER_to_results(result, config.num_clbits);
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the AER simulator.\n\tTry checking the format of the circuit sent and/or of the noise model.");
        result = {{"ERROR", std::string(e.what())}};
    } 
    return result;
}

} // End of sim namespace
} // End of cunqa namespace
