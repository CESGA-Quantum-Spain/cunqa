
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <optional>

#include "qulacs_simulator_adapter.hpp"

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "cppsim/utility.hpp"

#include "qulacs_utils.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

namespace {
using namespace cunqa;

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

struct LocalCCIDs {
    std::string sendr;
    std::string recvr;

    bool operator==(const LocalCCIDs& other) const {
        return sendr == other.sendr && recvr == other.recvr;
    }
}; // Struct to mimic classical communications when vQPUs deployed with quantum communications

struct LocalIDsHash {
    std::size_t operator()(const LocalCCIDs& local_cc_ids) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(local_cc_ids.sendr);
        std::size_t h2 = std::hash<std::string>{}(local_cc_ids.recvr);
        return h1 ^ (h2 << 1);
    }
};

struct CommunicationQubitsPair {
    int q0;
    int q1;
    bool idle = true;
    std::string sendr_qpu; // QSEND and EXPOSE
    std::string recvr_qpu; // QRECV and RCONTROL
    std::string qcomm_protocol;
    int label;
};

struct TaskState {
    std::string id;
    int local_n_clbits = 0;
    std::vector<constants::CUNQAInstruction>::const_iterator it, end;
    UINT zero_qubit = 0;
    UINT zero_clbit = 0;
    bool finished = false;
    bool blocked_by_teledata = false;
    bool blocked_by_telegate = false;
    bool blocked_by_cc = false;
    bool cat_entangled = false;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::queue<UINT>> qc_meas_td;
    std::unordered_map<std::string, std::queue<UINT>> qc_meas_tg;
    std::vector<CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<UINT>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
};

std::vector<int> find_idle_communication_pairs(GlobalState& G, const size_t n_pairs)
{
    std::vector<int> indices_idle_pairs;
    size_t count = 0;
    for (int index = 0; index < G.communication_pairs.size() && count < n_pairs; index++) {
        if (G.communication_pairs[index].idle) {
            indices_idle_pairs.push_back(index);
            count++;
        } 
    } 

    if (count < n_pairs) 
        return std::vector<int>();

    for (const auto& index : indices_idle_pairs) {
        G.communication_pairs[index].idle = false;
    }

    return indices_idle_pairs;
}

std::vector<int> find_my_communication_pairs(const GlobalState& G, const std::string& sendr, const std::string recvr, const std::string qcomm_protocol, size_t n_pairs = 0)
{
    std::vector<int> comm_pairs;
    size_t count = 0;
    if (n_pairs == 0) n_pairs = G.communication_pairs.size();
    for (int index = 0; index < G.communication_pairs.size(); index++) {
        if (count == n_pairs) return comm_pairs;
        if (!G.communication_pairs[index].idle &&
            G.communication_pairs[index].sendr_qpu == sendr && 
            G.communication_pairs[index].recvr_qpu == recvr &&
            G.communication_pairs[index].qcomm_protocol == qcomm_protocol) {
                comm_pairs.push_back(index);
                count++;
        } 
    } 

    return comm_pairs;
}
 

std::unordered_map<std::string, std::string> execute_shot_(
    QuantumState& state, 
    std::vector<StructuredQuantumTask>& st_qtasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc,
    const size_t& n_comm_qubits
)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (const auto &quantum_task : st_qtasks) {
        TaskState T;
        T.id = quantum_task.id;
        T.local_n_clbits = quantum_task.n_clbits;
        T.zero_qubit = G.n_qubits;
        T.zero_clbit = G.n_clbits;
        T.it = quantum_task.instructions.begin();
        T.end = quantum_task.instructions.end();
        T.blocked_by_teledata = false;
        T.blocked_by_telegate = false;
        T.blocked_by_cc = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.n_qubits;
        G.n_clbits += quantum_task.n_clbits;

    }
    
    // Here we add the communication qubits
    if (n_comm_qubits != 0) {
        G.n_qubits += n_comm_qubits;
        for (int i = 0; i < n_comm_qubits; i+=2) {
            CommunicationQubitsPair cqp = {
                .q0 = G.n_qubits - n_comm_qubits + i,
                .q1 = G.n_qubits - n_comm_qubits + i + 1
            };
            G.communication_pairs.push_back(cqp);
        }
    }

    auto generate_entanglement_ = [&](const size_t n_pairs) {
        std::vector<int> indices = find_idle_communication_pairs(G, n_pairs);

        if (!indices.empty()) {
            for (auto& index : indices) {
                UINT meas1 = measure_adapter(state, G.communication_pairs[index].q1);
                if (meas1) {
                    gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
                }
                UINT meas2 = measure_adapter(state, G.communication_pairs[index].q0);
                if (meas2) {
                    gate::X(G.communication_pairs[index].q0)->update_quantum_state(&state);
                }
                gate::H(G.communication_pairs[index].q0)->update_quantum_state(&state);
                gate::CNOT(G.communication_pairs[index].q0, G.communication_pairs[index].q1)->update_quantum_state(&state);
            }
        }
        
        return indices;
    };


    std::function<void(TaskState&, const std::optional<constants::CUNQAInstruction>&, const std::vector<int>)> apply_next_instr = 
        [&](TaskState& T, const std::optional<constants::CUNQAInstruction>& instruction = std::nullopt, const std::vector<int> comm_indices = {}) 
    {
        const CUNQAInstruction inst = !instruction.has_value() ? *T.it : instruction.value();
        auto inst_type = INSTRUCTIONS_MAP.at(inst.name);

        switch (inst_type)
        {
        case constants::MEASURE:
        {
            UINT measurement = measure_adapter(state, inst.qubits[0] + T.zero_qubit);
            G.creg[inst.clbits[0] + T.zero_clbit] = (measurement == 1);
            break;
        }
        case constants::COPY:
        {
            if(inst.l_clbits.size() != inst.r_clbits.size())
                throw std::runtime_error("The number of copied clbits and the number of clbits "
                                         "copied on does not match.");

            for (size_t i = 0; i < inst.l_clbits.size(); ++i)
                G.creg[inst.l_clbits[i] + T.zero_clbit] = G.creg[inst.r_clbits[i] + T.zero_clbit];
                
            break;
        }
        case constants::ID:
            gate::Identity(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::X:
            gate::X(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::Y:
            gate::Y(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::Z:
            gate::Z(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::H:
            gate::H(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::S:
            gate::S(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SDG:
            gate::Sdag(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::T:
            gate::T(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::TDG:
            gate::Tdag(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SX:
            gate::sqrtX(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SXDG:
            gate::sqrtXdag(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SY:
            gate::sqrtY(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SYDG:
            gate::sqrtYdag(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::P0:
            gate::P0(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::P1:
            gate::P1(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::U1: 
        {
            gate::U1(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::RX: 
        {
            gate::RX(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::RY: 
        {
            gate::RY(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::RZ: 
        {
            gate::RZ(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVX: 
        {
            gate::RotInvX(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVY: 
        {
            gate::RotInvY(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVZ: 
        {
            gate::RotInvZ(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTX: 
        {
            gate::RotX(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTY: 
        {
            gate::RotY(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTZ: 
        {
            gate::RotZ(inst.qubits[0] + T.zero_qubit, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::U2: 
        {
            gate::U2(inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1])->update_quantum_state(&state);
            break;
        }
        case constants::U3: 
        {
            gate::U3(inst.qubits[0] + T.zero_qubit, inst.params[0], inst.params[1], inst.params[2])->update_quantum_state(&state);
            break;
        }
        case constants::CX:
        {
            UINT control;
            if (inst.qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = inst.qubits[0] + T.zero_qubit;
            } 
            gate::CNOT(control, inst.qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::CZ:
        {
            UINT control;
            if (inst.qubits[0] < 0) {
                for (auto& index : comm_indices) {
                    if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[0]) {
                        control = G.communication_pairs[index].q1;
                        break;
                    }
                }
            } else {
                control = inst.qubits[0] + T.zero_qubit;
            } 
            gate::CZ(control, inst.qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::ECR:
        {
            gate::ECR(inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::SWAP:
        {
            gate::SWAP(inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::FUSEDSWAP:
        {
            gate::FusedSWAP(inst.qubits[0] + T.zero_qubit, inst.qubits[1] + T.zero_qubit, inst.block_size[0])->update_quantum_state(&state);
            break;
        }
        case constants::MULTIPAULI:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < inst.qubits.size(); i++) {
                uiqubits.push_back(inst.qubits[i] + T.zero_qubit);
            }
            gate::Pauli(uiqubits, inst.pauli_id_list)->update_quantum_state(&state);
            break;
        }
        case constants::MULTIPAULIROTATION:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < inst.qubits.size(); i++) {
                uiqubits.push_back(inst.qubits[i] + T.zero_qubit);
            }
            gate::PauliRotation(uiqubits, inst.pauli_id_list, inst.params[0])->update_quantum_state(&state);
            break;
        }
        case constants::UNITARY:
        {
            ComplexMatrix qulacs_matrix = sim::cunqamatrix_to_qulacsdensematrix(inst.matrix[0]);

            if (inst.qubits.size() > 1) {
                std::vector<unsigned int> unsigned_qubits;
                for (size_t i = 0; i < inst.qubits.size(); i++) {
                    if (inst.qubits[i] < 0) {
                        for (auto& index : comm_indices) {
                            if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                                unsigned_qubits.push_back(G.communication_pairs[index].q1);
                                break;
                            }
                        }
                    } else {
                        unsigned_qubits.push_back(inst.qubits[i] + T.zero_qubit);
                    }
                }
                gate::DenseMatrix(unsigned_qubits, qulacs_matrix)->update_quantum_state(&state);
            } else {
                gate::DenseMatrix(inst.qubits[0] + T.zero_qubit, qulacs_matrix)->update_quantum_state(&state);
            }
            break;
        }
        case constants::SPARSEMATRIX:
        {
            SparseComplexMatrix qulacs_sparse = sim::cunqamatrix_to_sparse(inst.matrix[0]);

            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < inst.qubits.size(); i++) {
                uiqubits.push_back(inst.qubits[i] + T.zero_qubit);
            }
            gate::SparseMatrix(uiqubits, qulacs_sparse)->update_quantum_state(&state);
            break;
        }
        case constants::DIAGONAL:
        {   
            ComplexVector qulacs_diagonal = sim::cunqadiagonal_to_qulacsdiagonal(inst.diagonal[0]);
            std::vector<unsigned int> unsigned_qubits;
            for (size_t i = 0; i < inst.qubits.size(); i++) {
                if (inst.qubits[i] < 0) {
                    for (auto& index : comm_indices) {
                        if (!G.communication_pairs[index].idle && G.communication_pairs[index].label == inst.qubits[i]) {
                            unsigned_qubits.push_back(G.communication_pairs[index].q1);
                            break;
                        }
                    }
                } else {
                    unsigned_qubits.push_back(inst.qubits[i] + T.zero_qubit);
                }
            }

            gate::DiagonalMatrix(unsigned_qubits, qulacs_diagonal)->update_quantum_state(&state);
            break;
        }
        case constants::RANDOMUNITARY:
        {
            std::vector<unsigned int> uiqubits;
            for (int i = 0; i < inst.qubits.size(); i++) {
                uiqubits.push_back(inst.qubits[i] + T.zero_qubit);
            }
            if (inst.seed != 0) {
                gate::RandomUnitary(uiqubits, inst.seed)->update_quantum_state(&state);
            } else {
                gate::RandomUnitary(uiqubits)->update_quantum_state(&state);
            }
            break;
        }
        case constants::BITFLIPNOISE:
        {
            if (inst.seed != 0) {
                gate::BitFlipNoise(inst.qubits[0], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::BitFlipNoise(inst.qubits[0], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::DEPHASINGNOISE:
        {
            if (inst.seed != 0) {
                gate::DephasingNoise(inst.qubits[0], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::DephasingNoise(inst.qubits[0], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::INDEPENDENTXZNOISE:
        {
            if (inst.seed != 0) {
                gate::IndependentXZNoise(inst.qubits[0], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::IndependentXZNoise(inst.qubits[0], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::DEPOLARIZINGNOISE:
        {
            if (inst.seed != 0) {
                gate::DepolarizingNoise(inst.qubits[0], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::DepolarizingNoise(inst.qubits[0], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::TWOQUBITDEPOLARIZINGNOISE:
        {
            if (inst.seed != 0) {
                gate::TwoQubitDepolarizingNoise(inst.qubits[0], inst.qubits[1], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::TwoQubitDepolarizingNoise(inst.qubits[0], inst.qubits[1], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::AMPLITUDEDAMPINGNOISE:
        {
            if (inst.seed != 0) {
                gate::AmplitudeDampingNoise(inst.qubits[0], inst.params[0], inst.seed)->update_quantum_state(&state);
            } else {
                gate::AmplitudeDampingNoise(inst.qubits[0], inst.params[0])->update_quantum_state(&state);
            }
            break;
        }
        case constants::SEND:
        {
            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = T.id, 
                    .recvr = Ts[inst.qpus[0]].id
                };  
                for (auto& clbit : inst.clbits) {
                    G.local_cc_queue[local_cc_ids].push(G.creg[clbit + T.zero_clbit]);
                }
            } else {
                for (const auto& clbit: inst.clbits) {
                    classical_channel->send_measure(G.creg[clbit + T.zero_clbit], inst.qpus[0]);
                }
            }
            break;
        }
        case constants::RECV:
        {
            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = Ts[inst.qpus[0]].id, 
                    .recvr = T.id
                };
                if (G.local_cc_queue.contains(local_cc_ids) && !G.local_cc_queue.at(local_cc_ids).empty()) {
                    for (const auto& clbit: inst.clbits) {
                        G.creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }
            } else {
                for (const auto& clbit: inst.clbits) {
                    int measurement = classical_channel->recv_measure(inst.qpus[0]);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case constants::CIF:
        {
            if (G.creg[inst.clbits[0] + T.zero_clbit]) {
                for(const auto& sub_inst: inst.instructions) {
                    apply_next_instr(T, sub_inst, {});
                }
            }
            break;
        }
        case constants::QSEND:
        {
            std::vector<int> indices = generate_entanglement_(1);
            if (indices.empty()) {
                T.blocked_by_teledata = true;
                return;
            }
            T.blocked_by_teledata = false;
            int index = indices[0];
            G.communication_pairs[index].qcomm_protocol = "teledata";

            // CX to the entangled pair
            gate::CNOT(inst.qubits[0] + T.zero_qubit, G.communication_pairs[index].q0)->update_quantum_state(&state);

            // H to the sent qubit
            gate::H(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            UINT result = measure_adapter(state, inst.qubits[0] + T.zero_qubit);

            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(measure_adapter(state, G.communication_pairs[index].q0));

            if (result) {
                gate::X(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            }

            // Unlock QRECV
            Ts[inst.qpus[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[index].sendr_qpu = T.id;
            G.communication_pairs[index].recvr_qpu = inst.qpus[0];

            break;
        }
        case constants::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.qpus[0]) || G.qc_meas_td[inst.qpus[0]].empty()) {
                T.blocked_by_teledata = true;
                return;
            }

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();
            std::size_t meas2 = G.qc_meas_td[inst.qpus[0]].front();
            G.qc_meas_td[inst.qpus[0]].pop();

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "teledata", 1);
            int index = indices[0];

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
            }
            if (meas2) {
                gate::Z(G.communication_pairs[index].q1)->update_quantum_state(&state);
            }

            // Swap the value to the desired qubit
            gate::SWAP(G.communication_pairs[index].q1, inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            G.communication_pairs[index].idle = true;
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                std::vector<int> indices = generate_entanglement_(inst.qubits.size());
                if (indices.empty()) {
                    T.blocked_by_telegate = true;
                    return;
                }

                int qid = 0;
                for (auto& index : indices) {
                    G.communication_pairs[index].qcomm_protocol = "telegate";
                    G.communication_pairs[index].label = -(qid + 1);

                    // CX to the entangled pair
                    gate::CNOT(inst.qubits[qid] + T.zero_qubit, G.communication_pairs[index].q0)->update_quantum_state(&state);

                    UINT result = measure_adapter(state, G.communication_pairs[index].q0);

                    G.qc_meas_tg[T.id].push(result);
                    T.cat_entangled = true;
                    T.blocked_by_telegate = true;
                    Ts[inst.qpus[0]].blocked_by_telegate = false;

                    // Update communication pair
                    G.communication_pairs[index].sendr_qpu = T.id;
                    G.communication_pairs[index].recvr_qpu = inst.qpus[0];

                    qid++;
                }
                return;
            } else {
                for (int i = 0; i < inst.qubits.size(); i++) {
                    UINT meas = G.qc_meas_tg[inst.qpus[0]].front();
                    G.qc_meas_tg[inst.qpus[0]].pop();

                    if (meas) {
                        gate::Z(inst.qubits[0] + T.zero_qubit)->update_quantum_state(&state);
                    }
                }

                T.cat_entangled = false;

                std::vector<int> indices = find_my_communication_pairs(G, T.id, inst.qpus[0], "telegate", inst.qubits.size());
                for (auto& index : indices) {
                    G.communication_pairs[index].idle = true;
                }
            }
            break;
        }
        case constants::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.qpus[0]) || G.qc_meas_tg[inst.qpus[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            std::vector<int> indices = find_my_communication_pairs(G, inst.qpus[0], T.id, "telegate");

            for (auto& index : indices) {
                UINT meas2 = G.qc_meas_tg[inst.qpus[0]].front();
                G.qc_meas_tg[inst.qpus[0]].pop();

                if (meas2) {
                    gate::X(G.communication_pairs[index].q1)->update_quantum_state(&state);
                }
            }

            for(const auto& sub_inst: inst.instructions) {
                apply_next_instr(T, sub_inst, indices);
            }

            for (auto& index : indices) {
                gate::H(G.communication_pairs[index].q1)->update_quantum_state(&state);

                UINT result = measure_adapter(state, G.communication_pairs[index].q1);
                G.qc_meas_tg[T.id].push(result);
            }

            Ts[inst.qpus[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << inst.name << "\n";
        } // End switch
    };

    while (!G.ended)
    {
        G.ended = true;
        for (auto& [id, T]: Ts)
        {
            if (T.finished)
                continue;
            else if(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc) {
                G.ended = false;
                continue;
            }

            apply_next_instr(T, std::nullopt, {});

            if (!(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc))
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    std::unordered_map<std::string, std::string> shot_bits;
    for (auto& [id, T]: Ts) {
        std::string bitstring(T.local_n_clbits, '0');
        for (const auto &[bitIndex, value] : G.creg) {
            if (T.zero_clbit <= bitIndex && bitIndex < (T.zero_clbit + T.local_n_clbits)) {
                bitstring[T.local_n_clbits + T.zero_clbit - bitIndex - 1] = value ? '1' : '0';
            }
        }
        shot_bits[id] = bitstring;
    }

    return shot_bits;
}

void update_meas_counter(std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>& meas_counter, const std::unordered_map<std::string, std::string>& shot_bitstrings)
{
    for (const auto& [circ_id, bitstring] : shot_bitstrings) {
        meas_counter[circ_id][bitstring]++;
    }
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON QulacsSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Qulacs usual simulation");
    try {
        auto quantum_task = qc.quantum_tasks[0];

        size_t n_qubits = quantum_task.config.at("num_qubits").get<size_t>();
        auto shots = qc.quantum_tasks[0].config.at("shots").get<size_t>();
        JSON circuit_json = quantum_task.circuit;

        QuantumCircuit circuit(n_qubits);
        update_qulacs_circuit(circuit, circuit_json);

        QuantumState state(n_qubits);
        circuit.update_quantum_state(&state);

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


JSON QulacsSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Qulacs dynamic simulation");
    std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>>  meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();

    std::vector<StructuredQuantumTask> st_qtasks;
    size_t n_qubits = 0;
    for (auto& quantum_task : qc.quantum_tasks) {
        st_qtasks.push_back(from_quantum_task_to_structuredqtask(quantum_task));
        n_qubits += quantum_task.config.at("num_qubits").get<size_t>();
    }

    size_t n_comm_qubits = 0;
    if (qc.quantum_tasks.size() > 1) { // Quantum Communications 
        if (qc.quantum_tasks[0].config.contains("n_communication_qubits")) {
            n_comm_qubits = qc.quantum_tasks[0].config.at("n_communication_qubits").get<size_t>();
            if (n_comm_qubits % 2 != 0) { // Ensure communication qubits always in pairs
                n_comm_qubits++;
            }
        } else {
            n_comm_qubits = 2;
        }

        n_qubits += n_comm_qubits;
    }    

    auto start = std::chrono::high_resolution_clock::now();
#ifdef OPENMP_IN_QC
    if (size(qc.quantum_tasks) > 1) { // Quantum communications 
        #pragma omp parallel
        {
            std::unordered_map<std::string, std::unordered_map<std::string, std::size_t>> local_counter;
            
            QuantumState state(n_qubits);

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                update_meas_counter(local_counter, execute_shot_(state, st_qtasks, classical_channel, allows_qc,n_comm_qubits));
                state.set_zero_state();
            }

            #pragma omp critical
            for (const auto& [id, bitstrings_counter] : local_counter) {
                for (const auto& [bitstring, counts] : bitstrings_counter) {
                    meas_counter[id][bitstring] += counts;
                } 
            }
        }
    } else { // As if OPENMP_IN_QC not enabled
        QuantumState state(n_qubits);
        for (std::size_t i = 0; i < shots; i++) {
            update_meas_counter(meas_counter, execute_shot_(state, st_qtasks, classical_channel, allows_qc,n_comm_qubits));
            state.set_zero_state();
        } // End all shots
    }
#else
    QuantumState state(n_qubits);
    for (std::size_t i = 0; i < shots; i++) {
        update_meas_counter(meas_counter, execute_shot_(state, st_qtasks, classical_channel, allows_qc,n_comm_qubits));
        state.set_zero_state();
    } // End all shots
#endif
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    JSON result_json = {
        {"id_counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

} // End of sim namespace
} // End of cunqa namespace