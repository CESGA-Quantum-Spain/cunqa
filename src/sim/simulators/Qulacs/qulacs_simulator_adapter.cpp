
#include <complex>
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <bitset>

#include "qulacs_simulator_adapter.hpp"
#include "qulacs_circuit_adapter.hpp"

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "cppsim/utility.hpp"

#include "logger.hpp"

namespace {

UINT measure_adapter(QuantumState& state, UINT target_index)
{
    Random random;
    auto gate0 = gate::P0(target_index);
    auto gate1 = gate::P1(target_index);
    std::vector<QuantumGateBase*> _gate_list = {gate0, gate1};
    double r = random.uniform();

    double sum = 0.;
    double org_norm = state.get_squared_norm();

    auto buffer = state.copy();
    UINT index = 0;
    for (auto gate : _gate_list) {
        gate->update_quantum_state(buffer);
        auto norm = buffer->get_squared_norm() / org_norm;
        sum += norm;
        if (r < sum) {
            state.load(buffer);
            state.normalize(norm);
            break;
        } else {
            buffer->load(&state);
            index++;
        }
    }

    delete gate0;
    delete gate1;
    delete buffer;

    return index;
}

inline ComplexMatrix cunqamatrix_to_qulacsdensematrix(const cunqa::Matrix& cunqa_matrix)
{
    if (cunqa_matrix.empty()) {
        return ComplexMatrix(0, 0);
    }

    size_t rows = cunqa_matrix.size();
    size_t cols = cunqa_matrix[0].size();

    ComplexMatrix qulacs_matrix(rows, cols);

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            const auto& complex_parts = cunqa_matrix[i][j];
            // Convert [real, imag] to std::complex<double>
            qulacs_matrix(i, j) = std::complex<double>(complex_parts[0], 
                                                        complex_parts.size() > 1 ? complex_parts[1] : 0.0);
        }
    }

    return qulacs_matrix;
}


inline SparseComplexMatrix cunqamatrix_to_sparse(const cunqa::Matrix& cunqa_matrix)
{
    if (cunqa_matrix.empty()) {
        return SparseComplexMatrix(0, 0);
    }

    size_t rows = cunqa_matrix.size();
    size_t cols = cunqa_matrix[0].size();

    std::vector<Eigen::Triplet<CPPCTYPE>> triplets;

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            const auto& complex_parts = cunqa_matrix[i][j];
            // Convert [real, imag] to std::complex<double>
            CPPCTYPE val(complex_parts[0], complex_parts.size() > 1 ? complex_parts[1] : 0.0);
            
            if (val != CPPCTYPE(0.0)) {
                triplets.emplace_back(i, j, val);
            }
        }
    }

    SparseComplexMatrix qulacs_sparse(rows, cols);
    qulacs_sparse.setFromTriplets(triplets.begin(), triplets.end());
    return qulacs_sparse;
}


inline ComplexVector cunqadiagonal_to_qulacsdiagonal(const cunqa::DiagonalMatrix& cunqa_diagonal)
{
    ComplexVector qulacs_diagonal(cunqa_diagonal.size());

    for (size_t i = 0; i < cunqa_diagonal.size(); ++i) {
        const auto& complex_parts = cunqa_diagonal[i];
        // Convert [real, imag] to std::complex<double>
        qulacs_diagonal[i] = std::complex<double>(complex_parts[0], complex_parts.size() > 1 ? complex_parts[1] : 0.0);
    }

    return qulacs_diagonal;
}

inline void update_qulacs_circuit(QuantumCircuit& circuit, const cunqa::QulacsCircuit& cunqa_circuit)
{
    for (const auto& instruction : cunqa_circuit.instructions) {

        switch (cunqa::instruction_type_from_name(instruction.at("name").get<std::string>()))
        {
        case cunqa::InstructionType::MEASURE:
            // Here we have again the problem of ignoring intermediate measurements on non-dynamic circuits
            break;
        case cunqa::InstructionType::X:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_X_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::Y:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_Y_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::Z:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_Z_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::H:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_H_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::S:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_S_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::SDG:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_Sdag_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::T:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_T_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::TDG:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_Tdag_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::SX:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_sqrtX_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::SXDG:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_sqrtXdag_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::SY:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_sqrtY_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::SYDG:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_sqrtYdag_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::P0:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_P0_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::P1:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_P1_gate(qubit);
            break;
        }
            
        case cunqa::InstructionType::U1:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_U1_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::RX:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RX_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::RY:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RY_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::RZ:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RZ_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTINVX:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotInvX_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTINVY:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotInvY_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTINVZ:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotInvZ_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTX:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotX_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTY:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotY_gate(qubit, param);
            break;
        }
        case cunqa::InstructionType::ROTZ:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_RotZ_gate(qubit, param);
            break;
        }

        case cunqa::InstructionType::U2: 
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U2_gate(qubit, params[0], params[1]);
            break;
        }
        case cunqa::InstructionType::U3: 
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            auto params = instruction.at("params").get<std::vector<double>>();
            circuit.add_U3_gate(qubit, params[0], params[1], params[2]);
            break;
        }
        case cunqa::InstructionType::CX:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            circuit.add_CNOT_gate(qubits[0], qubits[1]);
            break;
        }
        case cunqa::InstructionType::CZ:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            circuit.add_CZ_gate(qubits[0], qubits[1]);
            break;
        }
        case cunqa::InstructionType::ECR:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            circuit.add_ECR_gate(qubits[0], qubits[1]);
            break;
        }
        case cunqa::InstructionType::CP:
        {
            // Qulacs has no controlled-phase gate: U1 already is a matrix gate,
            // so the control qubit can be attached to it directly.
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto param = instruction.at("params").at(0).get<double>();
            auto* cp_gate = gate::U1(qubits[1], param);
            cp_gate->add_control_qubit(qubits[0], 1);
            circuit.add_gate(cp_gate);
            break;
        }
        case cunqa::InstructionType::SWAP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            circuit.add_SWAP_gate(qubits[0], qubits[1]);
            break;
        }
        case cunqa::InstructionType::FUSEDSWAP:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            circuit.add_FusedSWAP_gate(qubits[0], qubits[1], instruction.at("block_size").get<int>());
            break;
        }
        case cunqa::InstructionType::MULTIPAULI:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            std::vector<unsigned int> unsigned_quibits(qubits.begin(),qubits.end());
            circuit.add_multi_Pauli_gate(unsigned_quibits, instruction.at("pauli_id_list").get<std::vector<unsigned int>>());
            break;
        }
        case cunqa::InstructionType::MULTIPAULIROTATION:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i]);
            }
            auto param = instruction.at("params").at(0).get<double>();
            circuit.add_multi_Pauli_rotation_gate(uiqubits, instruction.at("pauli_id_list").get<std::vector<unsigned int>>(), param);
            break;
        }
        case cunqa::InstructionType::UNITARY:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            const auto matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<double>>>>();
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i]);
            }
            if (qubits.size() > 1) {
                circuit.add_dense_matrix_gate(uiqubits, qulacs_matrix);
            } else {
                circuit.add_dense_matrix_gate(uiqubits[0], qulacs_matrix);
            }
            break;
        }
        case cunqa::InstructionType::CUNITARY:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            const auto matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<double>>>>();
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(matrix);

            std::vector<TargetQubitInfo> target_qubits;
            for (size_t i = 1; i < qubits.size(); i++) {
                target_qubits.emplace_back(qubits[i], 0);
            }

            std::vector<ControlQubitInfo> control_qubits = {
                ControlQubitInfo(qubits[0], 1)
            };

            auto gate = new QuantumGateMatrix(target_qubits, &qulacs_matrix, control_qubits);
            circuit.add_gate(gate);
            break;
        }
        case cunqa::InstructionType::SPARSEMATRIX:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            const auto matrix = instruction.at("matrix").get<std::vector<std::vector<std::vector<double>>>>();
            SparseComplexMatrix qulacs_sparse = cunqamatrix_to_sparse(matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i]);
            }
            circuit.add_gate(gate::SparseMatrix(uiqubits, qulacs_sparse));
            break;
        }
        case cunqa::InstructionType::DIAGONAL:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            const auto matrix = instruction.at("matrix").get<std::vector<std::vector<double>>>();
            ComplexVector qulacs_diagonal = cunqadiagonal_to_qulacsdiagonal(matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i]);
            }
            circuit.add_gate(gate::DiagonalMatrix(uiqubits, qulacs_diagonal));
            break;
        }
        case cunqa::InstructionType::RANDOMUNITARY:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < qubits.size(); i++) {
                uiqubits.push_back(qubits[i]);
            }
            circuit.add_random_unitary_gate(uiqubits, static_cast<UINT>(instruction.at("seed").get<int>()));
            break;
        }
        case cunqa::InstructionType::ID:
        {
            auto qubit = instruction.at("qubits").get<unsigned int>();
            circuit.add_gate(gate::Identity(qubit));
            break;
        }
        case cunqa::InstructionType::BITFLIPNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::BitFlipNoise(qubits[0], prob, seed));
            break;
        }
        case cunqa::InstructionType::DEPHASINGNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::DephasingNoise(qubits[0], prob, seed));
            break;
        }
        case cunqa::InstructionType::INDEPENDENTXZNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::IndependentXZNoise(qubits[0], prob, seed));
            break;
        }
        case cunqa::InstructionType::DEPOLARIZINGNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::DepolarizingNoise(qubits[0], prob, seed));
            break;
        }
        case cunqa::InstructionType::AMPLITUDEDAMPINGNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::AmplitudeDampingNoise(qubits[0], prob, seed));
            break;
        }
        case cunqa::InstructionType::TWOQUBITDEPOLARIZINGNOISE:
        {
            auto qubits = instruction.at("qubits").get<std::vector<unsigned int>>();
            auto prob = instruction.at("params").at(0).get<double>();
            int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
            circuit.add_gate(gate::TwoQubitDepolarizingNoise(qubits[0], qubits[1], prob, seed));
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << instruction.at("name").get<std::string>() << "\n";
        };
    }
}

inline cunqa::JSON convert_to_counts(const std::vector<ITYPE>& result, int n_qubits)
{
    std::unordered_map<std::string, size_t> counts;
    size_t max_position = (1 << n_qubits) - 1;

    for (auto& value : result) {
        std::bitset<64> bs(value);
        std::string bitstring = bs.to_string();

        if (n_qubits <= 0) {
            bitstring = "";
        } else if (n_qubits < 64) {
            bitstring = bitstring.substr(64 - n_qubits);
        } 
        counts[bitstring]++;
    }

    cunqa::JSON result_in_counts;
    for (const auto& count : counts) {
        result_in_counts[count.first] = count.second;
    }

    return result_in_counts;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

struct QulacsSimulatorAdapter::State {
    QuantumState state;

    State(const int& n_qubits) : state(n_qubits){ }
};

QulacsSimulatorAdapter::QulacsSimulatorAdapter() = default;
QulacsSimulatorAdapter::~QulacsSimulatorAdapter() = default;

std::unique_ptr<Circuit> QulacsSimulatorAdapter::create_circuit(const JSON& instructions_json) const
{
    return std::make_unique<QulacsCircuit>(instructions_json);
}

void QulacsSimulatorAdapter::initialize()
{
    if (state_ == nullptr)
        state_ = std::make_unique<State>(num_qubits);
    state_->state.set_zero_state();
}

void QulacsSimulatorAdapter::clear()
{
    state_->state.set_zero_state();
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::ID:
            gate::Identity(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::X:
            gate::X(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::Y:
            gate::Y(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::Z:
            gate::Z(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::H:
            gate::H(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::S:
            gate::S(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::SDG:
            gate::Sdag(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::T:
            gate::T(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::TDG:
            gate::Tdag(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::SX:
            gate::sqrtX(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::SXDG:
            gate::sqrtXdag(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::SY:
            gate::sqrtY(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::SYDG:
            gate::sqrtYdag(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::P0:
            gate::P0(payload.qubit)->update_quantum_state(&state_->state);
            break;

        case InstructionType::P1:
            gate::P1(payload.qubit)->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::U1:
            gate::U1(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RX:
            gate::RX(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RY:
            gate::RY(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RZ:
            gate::RZ(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVX:
            gate::RotInvX(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVY:
            gate::RotInvY(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVZ:
            gate::RotInvZ(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTX:
            gate::RotX(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTY:
            gate::RotY(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTZ:
            gate::RotZ(payload.qubit, *payload.param)->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitTwoParam& payload)
{
    switch (type)
    {
        case InstructionType::U2:
            gate::U2(payload.qubit, *payload.params[0], *payload.params[1])->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitThreeParam& payload)
{
    switch (type)
    {
        case InstructionType::U3:
            gate::U3(payload.qubit, *payload.params[0], *payload.params[1], *payload.params[2])->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}


void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoParam& payload)
{
    switch (type)
    {
        case InstructionType::CX:
            gate::CNOT(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state_->state);
            break;

        case InstructionType::CZ:
            gate::CZ(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state_->state);
            break;

        case InstructionType::ECR:
            gate::ECR(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state_->state);
            break;

        case InstructionType::SWAP:
            gate::SWAP(payload.qubits[0], payload.qubits[1])->update_quantum_state(&state_->state);
            break;

        

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitOneParam& payload)
{
    switch (type)
    {
        case InstructionType::CP:
        {
            // Qulacs has no controlled-phase gate: U1 already is a matrix gate,
            // so the control qubit can be attached to it directly.
            std::unique_ptr<QuantumGateMatrix> cp_gate(gate::U1(payload.qubits[1], *payload.param));
            cp_gate->add_control_qubit(payload.qubits[0], 1);
            cp_gate->update_quantum_state(&state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const FusedSwap& payload)
{
    switch (type)
    {
        case InstructionType::FUSEDSWAP:
            gate::FusedSWAP(payload.qubits[0], payload.qubits[1], payload.block_size)->update_quantum_state(&state_->state);
            break;        

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const MultiPauli& payload)
{
    switch (type)
    {
        case InstructionType::MULTIPAULI:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            gate::Pauli(uiqubits, payload.pauli_id_list)->update_quantum_state(&state_->state);
            break;
        }

        case InstructionType::MULTIPAULIROTATION:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            gate::PauliRotation(uiqubits, payload.pauli_id_list, *payload.param)->update_quantum_state(&state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}


void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const MatrixGate& payload)
{
    switch (type)
    {
        case InstructionType::UNITARY:
        {
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(payload.matrix);
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            if (payload.qubits.size() > 1) {
                gate::DenseMatrix(uiqubits, qulacs_matrix)->update_quantum_state(&state_->state);
            } else {
                gate::DenseMatrix(uiqubits[0], qulacs_matrix)->update_quantum_state(&state_->state);
            }
            break;
        }

        case cunqa::InstructionType::CUNITARY:
        {
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(payload.matrix);

            std::vector<TargetQubitInfo> target_qubits;
            for (size_t i = 1; i < payload.qubits.size(); i++) {
                target_qubits.emplace_back(payload.qubits[i], 0);
            }

            std::vector<ControlQubitInfo> control_qubits = {
                ControlQubitInfo(payload.qubits[0], 1)
            };

            auto gate = new QuantumGateMatrix(target_qubits, &qulacs_matrix, control_qubits);
            gate->update_quantum_state(&state_->state);
            break;
        }

        case InstructionType::SPARSEMATRIX:
        {
            SparseComplexMatrix qulacs_sparse = cunqamatrix_to_sparse(payload.matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            gate::SparseMatrix(uiqubits, qulacs_sparse)->update_quantum_state(&state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const DiagonalMatrixGate& payload)
{
    switch (type)
    {
        case InstructionType::DIAGONAL:
        {
            ComplexVector qulacs_diagonal = cunqadiagonal_to_qulacsdiagonal(payload.matrix);
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            gate::DiagonalMatrix(uiqubits, qulacs_diagonal)->update_quantum_state(&state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}


void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const RandomUnitary& payload)
{
    switch (type)
    {
        case InstructionType::RANDOMUNITARY:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < payload.qubits.size(); i++) {
                uiqubits.push_back(payload.qubits[i]);
            }
            gate::RandomUnitary(uiqubits, payload.seed)->update_quantum_state(&state_->state);
            break;
        }

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const OneQubitNoise& payload)
{
    switch (type)
    {
        case InstructionType::BITFLIPNOISE:
            gate::BitFlipNoise(payload.qubit, *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::DEPHASINGNOISE:
            gate::DephasingNoise(payload.qubit, *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::INDEPENDENTXZNOISE:
            gate::IndependentXZNoise(payload.qubit, *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::DEPOLARIZINGNOISE:
            gate::DepolarizingNoise(payload.qubit, *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::AMPLITUDEDAMPINGNOISE:
            gate::AmplitudeDampingNoise(payload.qubit, *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const TwoQubitNoise& payload)
{
    switch (type)
    {
        case InstructionType::TWOQUBITDEPOLARIZINGNOISE:
            gate::TwoQubitDepolarizingNoise(payload.qubits[0], payload.qubits[1], *payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const Measure& payload)
{
    switch (type)
    {
        case InstructionType::MEASURE:
            if(payload.clbit < config.num_clbits) {
                    creg[payload.clbit] =
                        (measure_adapter(state_->state, payload.qubit) == 1U);
                    save_clbit[payload.clbit] = payload.save;
                } else {
                    throw std::runtime_error("Cannot store measurement: classical bit "
                                            "index exceeds the available range.");
                }    
            break;

        default:
            unsupported_gate(type, payload);
    }
}

void QulacsSimulatorAdapter::apply_gate(const InstructionType& type, const Copy& payload)
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

JSON QulacsSimulatorAdapter::native_execute(const Circuit& circuit)
{
    LOGGER_DEBUG("Qulacs native execution");
    try {
        auto& qulacs_adapter_circuit = dynamic_cast<const QulacsCircuit&>(circuit);

        size_t n_qubits = num_qubits;
        auto shots = config.shots;

        QuantumCircuit circuit_qulacs(n_qubits);
        update_qulacs_circuit(circuit_qulacs, qulacs_adapter_circuit);

        QuantumState state(n_qubits);
        circuit_qulacs.update_quantum_state(&state);

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<ITYPE> samples = state.sampling(shots);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts = convert_to_counts(samples, config.num_clbits);

        JSON result_json = 
        {
            {"counts", counts},
            {"time_taken", time_taken}
        };

        return result_json;

    } catch (const std::exception& e) {
        LOGGER_ERROR("Error executing the circuit in the Qulacs simulator.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}

} // End of sim namespace
} // End of cunqa namespace