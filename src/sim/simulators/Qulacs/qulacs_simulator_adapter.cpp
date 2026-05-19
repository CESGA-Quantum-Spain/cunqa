
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <bitset>

#include "qulacs_simulator_adapter.hpp"

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "cppsim/utility.hpp"

#include "utils/constants.hpp"
#include "utils/json.hpp"

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
        std::copy(cunqa_matrix[i].begin(), cunqa_matrix[i].end(), 
                  qulacs_matrix.row(i).begin());
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
            const auto& val = cunqa_matrix[i][j];
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
        qulacs_diagonal[i] = cunqa_diagonal[i];
    }

    return qulacs_diagonal;
}

inline void update_qulacs_circuit(QuantumCircuit& circuit, const cunqa::Circuit& cunqa_circuit)
{
    for (const auto& instruction : cunqa_circuit.instructions) {

        switch (instruction.type)
        {
        case cunqa::InstructionType::MEASURE:
            // Here we have again the problem of ignoring intermediate measurements on non-dynamic circuits
            break;
        case cunqa::InstructionType::X:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_X_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::Y:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_Y_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::Z:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_Z_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::H:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_H_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::S:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_S_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::SDG:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_Sdag_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::T:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_T_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::TDG:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_Tdag_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::SX:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_sqrtX_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::SXDG:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_sqrtXdag_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::SY:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_sqrtY_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::SYDG:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_sqrtYdag_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::P0:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_P0_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::P1:
        {
            const auto& op = std::get<cunqa::OneQubitNoParam>(instruction.payload);
            circuit.add_P1_gate(op.qubit);
            break;
        }
            
        case cunqa::InstructionType::U1:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_U1_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::RX:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RX_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::RY:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RY_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::RZ:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RZ_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTINVX:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotInvX_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTINVY:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotInvY_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTINVZ:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotInvZ_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTX:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotX_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTY:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotY_gate(op.qubit, op.param);
            break;
        }
        case cunqa::InstructionType::ROTZ:
        {
            const auto& op = std::get<cunqa::OneQubitOneParam>(instruction.payload);
            circuit.add_RotZ_gate(op.qubit, op.param);
            break;
        }

        case cunqa::InstructionType::U2: 
        {
            const auto& op = std::get<cunqa::OneQubitTwoParam>(instruction.payload);
            circuit.add_U2_gate(op.qubit, op.params[0], op.params[1]);
            break;
        }
        case cunqa::InstructionType::U3: 
        {
            const auto& op = std::get<cunqa::OneQubitThreeParam>(instruction.payload);
            circuit.add_U3_gate(op.qubit, op.params[0], op.params[1], op.params[2]);
            break;
        }
        case cunqa::InstructionType::CX:
        {
            const auto& op = std::get<cunqa::TwoQubitNoParam>(instruction.payload);
            circuit.add_CNOT_gate(op.qubits[0], op.qubits[1]);
            break;
        }
        case cunqa::InstructionType::CZ:
        {
            const auto& op = std::get<cunqa::TwoQubitNoParam>(instruction.payload);
            circuit.add_CZ_gate(op.qubits[0], op.qubits[1]);
            break;
        }
        case cunqa::InstructionType::ECR:
        {
            const auto& op = std::get<cunqa::TwoQubitNoParam>(instruction.payload);
            circuit.add_ECR_gate(op.qubits[0], op.qubits[1]);
            break;
        }
        case cunqa::InstructionType::SWAP:
        {
            const auto& op = std::get<cunqa::TwoQubitNoParam>(instruction.payload);
            circuit.add_SWAP_gate(op.qubits[0], op.qubits[1]);
            break;
        }
        case cunqa::InstructionType::FUSEDSWAP:
        {
            const auto& op = std::get<cunqa::FusedSwap>(instruction.payload);
            circuit.add_FusedSWAP_gate(op.qubits[0], op.qubits[1], op.block_size);
            break;
        }
        case cunqa::InstructionType::MULTIPAULI:
        {
            const auto& op = std::get<cunqa::MultiPauli>(instruction.payload);
            std::vector<unsigned int> unsigned_quibits(op.qubits.begin(),op.qubits.end());
            circuit.add_multi_Pauli_gate(unsigned_quibits, op.pauli_id_list);
            break;
        }
        case cunqa::InstructionType::MULTIPAULIROTATION:
        {
            const auto& op = std::get<cunqa::MultiPauli>(instruction.payload);
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < op.qubits.size(); i++) {
                uiqubits.push_back(op.qubits[i]);
            }
            circuit.add_multi_Pauli_rotation_gate(uiqubits, op.pauli_id_list, op.param);
            break;
        }
        case cunqa::InstructionType::UNITARY:
        {
            const auto& op = std::get<cunqa::MatrixGate>(instruction.payload);
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(op.matrix);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < op.qubits.size(); i++) {
                uiqubits.push_back(op.qubits[i]);
            }
            if (op.qubits.size() > 1) {
                circuit.add_dense_matrix_gate(uiqubits, qulacs_matrix);
            } else {
                circuit.add_dense_matrix_gate(uiqubits[0], qulacs_matrix);
            }
            break;
        }
        case cunqa::InstructionType::CUNITARY:
        {
            const auto& op = std::get<cunqa::MatrixGate>(instruction.payload);
            ComplexMatrix qulacs_matrix = cunqamatrix_to_qulacsdensematrix(op.matrix);

            std::vector<TargetQubitInfo> target_qubits;
            for (size_t i = 1; i < op.qubits.size(); i++) {
                target_qubits.emplace_back(op.qubits[i], 0);
            }

            std::vector<ControlQubitInfo> control_qubits = {
                ControlQubitInfo(op.qubits[0], 1)
            };

            auto gate = new QuantumGateMatrix(target_qubits, &qulacs_matrix, control_qubits);
            circuit.add_gate(gate);
            break;
        }
        case cunqa::InstructionType::RANDOMUNITARY:
        {
            const auto& op = std::get<cunqa::RandomUnitary>(instruction.payload);
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < op.qubits.size(); i++) {
                uiqubits.push_back(op.qubits[i]);
            }
            circuit.add_random_unitary_gate(uiqubits, static_cast<UINT>(op.seed));
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << cunqa::INVERTED_INSTRUCTIONS_MAP.at(instruction.type) << "\n";
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

    State(int& n_qubits) : state(n_qubits){ }
};

QulacsSimulatorAdapter::QulacsSimulatorAdapter()
    : state_(std::make_unique<State>(config.num_qubits))
{ }
QulacsSimulatorAdapter::~QulacsSimulatorAdapter() = default;

void QulacsSimulatorAdapter::initialize()
{
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
            gate::U1(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RX:
            gate::RX(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RY:
            gate::RY(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::RZ:
            gate::RZ(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVX:
            gate::RotInvX(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVY:
            gate::RotInvY(payload.qubit, payload.param)->update_quantum_state(&state_->state);
            break;

        case InstructionType::ROTINVZ:
            gate::RotInvZ(payload.qubit, payload.param)->update_quantum_state(&state_->state);
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
            gate::U2(payload.qubit, payload.params[0], payload.params[1])->update_quantum_state(&state_->state);
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
            gate::U3(payload.qubit, payload.params[0], payload.params[1], payload.params[2])->update_quantum_state(&state_->state);
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
            gate::PauliRotation(uiqubits, payload.pauli_id_list, payload.param)->update_quantum_state(&state_->state);
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
            gate::BitFlipNoise(payload.qubit, payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::DEPHASINGNOISE:
            gate::DephasingNoise(payload.qubit, payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::INDEPENDENTXZNOISE:
            gate::IndependentXZNoise(payload.qubit, payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::DEPOLARIZINGNOISE:
            gate::DepolarizingNoise(payload.qubit, payload.params, payload.seed)->update_quantum_state(&state_->state);
            break;

        case InstructionType::AMPLITUDEDAMPINGNOISE:
            gate::AmplitudeDampingNoise(payload.qubit, payload.params, payload.seed)->update_quantum_state(&state_->state);
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
            gate::TwoQubitDepolarizingNoise(payload.qubits[0], payload.qubits[1], payload.params, payload.seed)->update_quantum_state(&state_->state);
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
            creg[payload.clbit] = (measure_adapter(state_->state, payload.qubit) == 1);
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

JSON QulacsSimulatorAdapter::native_execute(const Circuit& circuit, const JSON& noise_model)
{
    LOGGER_DEBUG("Qulacs usual simulation");
    try {
        size_t n_qubits = config.num_qubits;
        auto shots = config.shots;

        QuantumCircuit circuit_qulacs(n_qubits);
        update_qulacs_circuit(circuit_qulacs, circuit);

        QuantumState state(n_qubits);
        circuit_qulacs.update_quantum_state(&state);

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<ITYPE> samples = state.sampling(shots);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts = convert_to_counts(samples, n_qubits);

        JSON result_json = 
        {
            {"counts", counts},
            {"time_taken", time_taken}
        };

        return result_json;

    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Qulacs simulator.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}

} // End of sim namespace
} // End of cunqa namespace