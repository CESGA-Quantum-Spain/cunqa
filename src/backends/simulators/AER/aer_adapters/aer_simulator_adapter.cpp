
#include <unordered_map>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdlib>
#include <vector>

#include "aer_simulator_adapter.hpp"

#include "simulators/circuit_executor.hpp"
#include "framework/config.hpp"
#include "noise/noise_model.hpp"
#include "framework/circuit.hpp"
#include "controllers/controller_execute.hpp"
#include "framework/results/result.hpp"
#include "controllers/aer_controller.hpp"
#include "controllers/state_controller.hpp"
#include "aer_helpers.hpp"

#include "utils/constants.hpp"

#include "logger.hpp"

namespace {

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
};

struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    unsigned long zero_qubit = 0;
    unsigned long zero_clbit = 0;
    bool finished = false;
    bool blocked_by_teledata = false;
    bool blocked_by_telegate = false;
    bool blocked_by_cc = false;
    bool cat_entangled = false;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::stack<uint_t>> qc_meas_td;
    std::unordered_map<std::string, std::stack<uint_t>> qc_meas_tg;
    std::unordered_map<std::string, CommunicationQubitsPair> communication_pairs;
    std::unordered_map<LocalCCIDs, std::queue<uint_t>, LocalIDsHash> local_cc_queue; // To mimic classical communications when executing with quantum communications
    bool ended = false;
};

std::string find_idle_communication_pair(GlobalState& G)
{
    for (auto& [key, comm_pair] : G.communication_pairs) {
        if (comm_pair.idle) {
            comm_pair.idle = false;
            return key;
        } 
    }
    return "NOIDLEPAIRS";
}

std::string find_my_communication_pair(const GlobalState& G, const std::string& sendr, const std::string recvr, const std::string qcomm_protocol)
{
    for (auto& [key, comm_pair] : G.communication_pairs) {
        if (comm_pair.sendr_qpu == sendr && comm_pair.recvr_qpu == recvr && comm_pair.qcomm_protocol == qcomm_protocol) return key;
    }
}


std::string execute_shot_(
    AER::AerState* state, 
    const std::vector<cunqa::QuantumTask>& quantum_tasks, 
    cunqa::comm::ClassicalChannel* classical_channel,
    const bool allows_qc,
    const size_t& n_comm_qubits
)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (auto &quantum_task : quantum_tasks)
    {
        TaskState T;
        T.id = quantum_task.id;
        T.zero_qubit = G.n_qubits;
        T.zero_clbit = G.n_clbits;
        T.it = quantum_task.circuit.begin();
        T.end = quantum_task.circuit.end();
        T.blocked_by_teledata = false;
        T.blocked_by_telegate = false;
        T.blocked_by_cc = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.config.at("num_qubits").get<int>();
        G.n_clbits += quantum_task.config.at("num_clbits").get<int>();
    }
    
    // Here we add the communication qubits
    if (n_comm_qubits != 0) {
        G.n_qubits += n_comm_qubits;
        for (int i = 0; i < n_comm_qubits; i+=2) {
            CommunicationQubitsPair cqp = {
                .q0 = G.n_qubits - n_comm_qubits + i,
                .q1 = G.n_qubits - n_comm_qubits + i + 1
            };
            G.communication_pairs[std::to_string(i)] = cqp;
        }
    }

    auto generate_entanglement_ = [&]() {
        std::string key = find_idle_communication_pair(G);
        if (key != "NOIDLEPAIRS") {
            state->apply_reset({G.communication_pairs[key].q1});
            state->apply_reset({G.communication_pairs[key].q0});
            state->apply_h(G.communication_pairs[key].q0);
            state->apply_mcx({G.communication_pairs[key].q0, G.communication_pairs[key].q1});
        }

        return key;
    };


    std::function<void(TaskState&, const cunqa::JSON&, const std::string&)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}, const std::string comm_pair_key = "") 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;
        std::string inst_name = inst.at("name").get<std::string>();

        std::vector<int> qubits;
        if (inst.contains("qubits"))
            qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = cunqa::constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case cunqa::constants::MEASURE:
        {
            uint_t measurement = state->apply_measure({qubits[0] + T.zero_qubit});
            auto clbits = inst.at("clbits").get<std::vector<int>>();
            G.creg[clbits[0] + T.zero_clbit] = (measurement == 1);
            break;
        }
        case cunqa::constants::COPY:
        {
            auto l_clbits = inst.at("l_clbits").get<std::vector<int>>();
            auto r_clbits = inst.at("r_clbits").get<std::vector<int>>();

            if(l_clbits.size() != r_clbits.size())
                throw std::runtime_error("The number of copied clbits and the number of clbits "
                                         "copied on does not match.");

            for (size_t i = 0; i < l_clbits.size(); ++i)
                G.creg[l_clbits[i] + T.zero_clbit] = G.creg[r_clbits[i] + T.zero_clbit];
                
            break;
        }
        case cunqa::constants::X:
            state->apply_x(qubits[0] + T.zero_qubit);
            break;
        case cunqa::constants::Y:
            state->apply_y(qubits[0] + T.zero_qubit);
            break;
        case cunqa::constants::Z:
            state->apply_z(qubits[0] + T.zero_qubit);
            break;
        case cunqa::constants::H:
            state->apply_h(qubits[0] + T.zero_qubit);
            break;
        case cunqa::constants::SX:
            state->apply_mcsx({qubits[0] + T.zero_qubit});
            break;
        case cunqa::constants::RESET:
            state->apply_reset({qubits[0] + T.zero_qubit});
            break;
        case cunqa::constants::RX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            state->apply_mcrx({qubits[0] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::RY:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            state->apply_mcry({qubits[0] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::RZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            state->apply_mcrz({qubits[0] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::U3:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            state->apply_u(qubits[0] + T.zero_qubit, params[0], params[1], params[2]);
            break;
        }
        case cunqa::constants::CX:
        {
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_mcx({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::CY:
        {
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_mcy({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::CZ:
        {
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_mcz({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::CRX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_mcrx({control, qubits[1] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::CRY:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            state->apply_mcry({control, qubits[1] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::CRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_mcrz({control, qubits[1] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::CU:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[0] + T.zero_qubit;
            state->apply_cu({control, qubits[1] + T.zero_qubit}, params[0], params[1], params[2], params[3]);
            break;
        }
        case cunqa::constants::SWAP:
        {
            state->apply_mcswap({qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::MCX:
        {
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcx(unsigned_qubits);
            break;
        }
        case cunqa::constants::MCY:
        {
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcy(unsigned_qubits);
            break;
        }
        case cunqa::constants::MCZ:
        {
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcz(unsigned_qubits);
            break;
        }
        case cunqa::constants::MCSX:
        {
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcsx(unsigned_qubits);
            break;
        }
        case cunqa::constants::MCP:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcphase(unsigned_qubits, params[0]);
            break;
        }
        case cunqa::constants::MCRX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcrx(unsigned_qubits, params[0]);
            break;
        }
        case cunqa::constants::MCRY:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcry(unsigned_qubits, params[0]);
            break;
        }
        case cunqa::constants::MCRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcrz(unsigned_qubits, params[0]);
            break;
        }
        case cunqa::constants::MCU:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcu(unsigned_qubits, params[0], params[1], params[2], params[3]);
            break;
        }
        case cunqa::constants::MCSWAP:
        {
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_mcswap(unsigned_qubits);
            break;
        }
        case cunqa::constants::GLOBALP:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            state->apply_global_phase(params[0]);
            break;
        }
        case cunqa::constants::UNITARY:
        {
            auto cunqa_matrix = inst.at("matrix").get<std::vector<CunqaAerMatrix>>()[0];
            AerComplexVector matrix_data;
            cunqa::sim::convert_cunqa_matrix_to_complex_vector(cunqa_matrix, matrix_data);
            size_t dim = cunqa_matrix.size();
            matrix<complex_t> aer_matrix(dim, dim, matrix_data.data());
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_unitary(unsigned_qubits, aer_matrix);
            break;
        }
        case cunqa::constants::DIAGONAL:
        {
            auto cunqa_diagonal = inst.at("matrix").get<std::vector<CunqaAerDiagonalMatrix>>()[0];
            AER::cvector_t aer_diagonal;
            cunqa::sim::convert_cunqadiagonal_to_aerdiagonal(cunqa_diagonal, aer_diagonal);
            reg_t unsigned_qubits;
            for (size_t i = 0; i < qubits.size(); i++) {
                unsigned_qubits.push_back((qubits[i] == -1) ? G.communication_pairs[comm_pair_key].q1 : qubits[i] + T.zero_qubit);
            }
            state->apply_diagonal_matrix(unsigned_qubits, aer_diagonal);
            break;
        }
        case cunqa::constants::MULTIPLEXER:
        {
            LOGGER_ERROR("Multiplexer instruction is not supported in CUNQA-AER");
            break;
        }
        case cunqa::constants::SEND:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();   

            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = T.id, 
                    .recvr = Ts[qpu_id].id
                };  
                for (auto& clbit : clbits) {
                    G.local_cc_queue[local_cc_ids].push(G.creg[clbit + T.zero_clbit]);
                }
            } else {
                for (const auto& clbit: clbits) {
                    classical_channel->send_measure(G.creg[clbit + T.zero_clbit], qpu_id);
                }
            }
            break;
        }
        case cunqa::constants::RECV:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();

            if (allows_qc) {
                LocalCCIDs local_cc_ids = {
                    .sendr = Ts[qpu_id].id, 
                    .recvr = T.id
                };
                if (G.local_cc_queue.contains(local_cc_ids) && !G.local_cc_queue.at(local_cc_ids).empty()) {
                    state->flush_ops(); // Execute operations to empty the buffer 
                    for (const auto& clbit: clbits) {
                        G.creg[clbit + T.zero_clbit] = (G.local_cc_queue.at(local_cc_ids).front() == 1);
                        G.local_cc_queue.at(local_cc_ids).pop();
                    }
                    T.blocked_by_cc = false;
                } else {
                    T.blocked_by_cc = true;
                }   
            } else {
                state->flush_ops(); // Execute operations to empty the buffer 
                for (const auto& clbit: clbits) {
                    int measurement = classical_channel->recv_measure(qpu_id);
                    G.creg[clbit + T.zero_clbit] = (measurement == 1);
                }
            }
            break;
        }
        case cunqa::constants::CIF:
        {
            const auto& clbits = inst.at("clbits").get<std::vector<int>>();
            if (G.creg[clbits.at(0) + T.zero_clbit]) {
                for(const auto& sub_inst: inst.at("instructions")) {
                    apply_next_instr(T, sub_inst, "");
                }
            }
            break;
        }
        case cunqa::constants::QSEND:
        {
            std::string key = generate_entanglement_();
            if (key == "NOIDLEPAIRS") {
                T.blocked_by_teledata = true;
                return;
            }
            T.blocked_by_teledata = false;
            G.communication_pairs[key].qcomm_protocol = "teledata";

            // CX to the entangled pair
            state->apply_mcx({qubits[0] + T.zero_qubit, G.communication_pairs[key].q0});

            // H to the sent qubit
            state->apply_h(qubits[0] + T.zero_qubit);

            uint_t result = state->apply_measure({qubits[0] + T.zero_qubit});
            G.qc_meas_td[T.id].push(result);
            G.qc_meas_td[T.id].push(state->apply_measure({G.communication_pairs[key].q0}));

            if (result) {
                state->apply_reset({qubits[0] + T.zero_qubit});
            }

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked_by_teledata = false;

            // Update communication pair
            G.communication_pairs[key].sendr_qpu = T.id;
            G.communication_pairs[key].recvr_qpu = inst.at("qpus")[0].get<std::string>();

            break;
        }
        case cunqa::constants::QRECV:
        {
            if (!G.qc_meas_td.contains(inst.at("qpus")[0])) {
                T.blocked_by_teledata = true;
                return;
            }
            if (T.blocked_by_teledata) return;

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();
            std::size_t meas2 = G.qc_meas_td[inst.at("qpus")[0]].top();
            G.qc_meas_td[inst.at("qpus")[0]].pop();

            std::string key = find_my_communication_pair(G, inst.at("qpus")[0], T.id, "teledata");

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                state->apply_x(G.communication_pairs[key].q1);
            }
            if (meas2) {
                state->apply_z(G.communication_pairs[key].q1);
            }

            // Swap the value to the desired qubit
            state->apply_mcswap({G.communication_pairs[key].q1, qubits[0] + T.zero_qubit});

            G.communication_pairs[key].idle = true;
            break;
        }
        case cunqa::constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                std::string key = generate_entanglement_();
                if (key == "NOIDLEPAIRS") {
                    T.blocked_by_telegate = true;
                    return;
                }
                G.communication_pairs[key].qcomm_protocol = "telegate";

                // CX to the entangled pair
                state->apply_mcx({qubits[0] + T.zero_qubit, G.communication_pairs[key].q0});

                uint_t result = state->apply_measure({G.communication_pairs[key].q0});

                G.qc_meas_tg[T.id].push(result);
                T.cat_entangled = true;
                T.blocked_by_telegate = true;
                Ts[inst.at("qpus")[0]].blocked_by_telegate = false;

                // Update communication pair
                G.communication_pairs[key].sendr_qpu = T.id;
                G.communication_pairs[key].recvr_qpu = inst.at("qpus")[0].get<std::string>();
                return;
            } else {
                uint_t meas = G.qc_meas_tg[inst.at("qpus")[0]].top();
                G.qc_meas_tg[inst.at("qpus")[0]].pop();

                if (meas) {
                    state->apply_z(qubits[0] + T.zero_qubit); 
                }

                T.cat_entangled = false;

                std::string key = find_my_communication_pair(G, T.id, inst.at("qpus")[0], "telegate");
                G.communication_pairs[key].idle = true;
            }
            break;
        }
        case cunqa::constants::RCONTROL:
        {
            if (!G.qc_meas_tg.contains(inst.at("qpus")[0]) || G.qc_meas_tg[inst.at("qpus")[0]].empty()) {
                T.blocked_by_telegate = true;
                return;
            }
            if (T.blocked_by_telegate) return;

            uint_t meas2 = G.qc_meas_tg[inst.at("qpus")[0]].top();
            G.qc_meas_tg[inst.at("qpus")[0]].pop();

            std::string key = find_my_communication_pair(G, inst.at("qpus")[0], T.id, "telegate");

            if (meas2) {
                state->apply_mcx({G.communication_pairs[key].q1});
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst, key);
            }

            state->apply_h(G.communication_pairs[key].q1);

            uint_t result = state->apply_measure({G.communication_pairs[key].q1});
            G.qc_meas_tg[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked_by_telegate = false;
            T.blocked_by_telegate = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!\nInstruction that failed: " << inst.dump(4) << "\n";
        } // End switch
    };

    while (!G.ended)
    {
        G.ended = true;
        for (auto& [id, T]: Ts)
        {
            if (T.finished)
                continue;
            else if (T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc) {
                G.ended = false;
                continue;
            }

            apply_next_instr(T, {}, "");

            if (!(T.blocked_by_teledata || T.blocked_by_telegate || T.blocked_by_cc))
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    std::string result_bits(G.n_clbits, '0');
    for (const auto &[bitIndex, value] : G.creg)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
}

} // End of anonymous namespace

namespace cunqa {
namespace sim {

JSON AerSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Aer usual simulation");
    try {
        auto quantum_task = qc.quantum_tasks[0];

        auto aer_quantum_task = quantum_task_to_AER(quantum_task);
        int n_clbits = quantum_task.config.at("num_clbits");
        JSON circuit_json = aer_quantum_task.circuit;

        Circuit circuit(circuit_json);
        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        JSON run_config_json(aer_quantum_task.config);
        if (quantum_task.config.contains("seed")) {
            run_config_json["seed_simulator"] = quantum_task.config.at("seed");
        }
        Config aer_config(run_config_json);
        Noise::NoiseModel noise_model(backend->config.at("noise_model"));

        Result result = controller_execute<Controller>(circuits, noise_model, aer_config);

        JSON result_json = result.to_json();
        convert_standard_results_Aer(result_json, n_clbits);

        return result_json;

    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the AER simulator.\n\tTry checking the format of the circuit sent and/or of the noise model.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}

AER::AerState get_configured_aer_state(const JSON& config);
JSON AerSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel, const bool allows_qc)
{
    LOGGER_DEBUG("Aer dynamic simulation");

    std::map<std::string, std::size_t> meas_counter;
    
    JSON qt_config = qc.quantum_tasks[0].config;
    auto shots = qt_config.at("shots").get<std::size_t>();
    std::string device = qt_config.at("device")["device_name"];
    reg_t target_gpus = (device == "GPU") ? qt_config.at("device")["target_devices"].get<reg_t>() : reg_t();

    size_t n_qubits = 0;
    for (auto& quantum_task : qc.quantum_tasks) {
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
            std::map<std::string, std::size_t> local_counter;

            AER::AerState state = get_configured_aer_state(qt_config);

            #pragma omp for
            for (std::size_t i = 0; i < shots; i++) {
                reg_t qubit_ids = state.allocate_qubits(n_qubits);
                state.initialize();
                /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
                state.set_target_gpus(target_gpus);
                local_counter[execute_shot_(&state, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
                state.clear();
            }

            #pragma omp critical
            for (auto& [key, val] : local_counter)
                meas_counter[key] += val;
        }
    } else { // As if OPENMP_IN_QC not enabled
        AER::AerState state = get_configured_aer_state(qt_config);
        reg_t qubit_ids;
        for (std::size_t i = 0; i < shots; i++) {
            qubit_ids = state.allocate_qubits(n_qubits);
            state.initialize();
            /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
            state.set_target_gpus(target_gpus);
            meas_counter[execute_shot_(&state, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
            state.clear();
        } // End all shots
    }
#else
    AER::AerState state = get_configured_aer_state(qt_config);
    reg_t qubit_ids;
    for (std::size_t i = 0; i < shots; i++) {
        qubit_ids = state.allocate_qubits(n_qubits);
        state.initialize();
        /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
        state.set_target_gpus(target_gpus);
        meas_counter[execute_shot_(&state, qc.quantum_tasks, classical_channel, allows_qc, n_comm_qubits)]++;
        state.clear();
    } // End all shots
#endif
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();


    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

AER::AerState get_configured_aer_state(const JSON& config)
{
    AER::AerState state;

    std::string method = config.at("method").get<std::string>();
    std::string sim_method = (method == "automatic") ? "statevector" : method;
    std::string device = config.at("device")["device_name"];
    state.configure("method", sim_method);
    state.configure("device", device);
    state.configure("precision", "double");
    if (config.contains("seed")) {
        state.configure("seed_simulator", std::to_string(config.at("seed").get<int>()));
    }

    return state;
}

} // End of sim namespace
} // End of cunqa namespace
