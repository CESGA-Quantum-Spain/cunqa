
#include <unordered_map>
#include <stack>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "qulacs_simulator_adapter.hpp"
#include "qulacs_utils.hpp"

#include "cppsim/circuit.hpp"
#include "cppsim/gate_factory.hpp"
#include "cppsim/utility.hpp"

#include "utils/constants.hpp"
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

namespace {

UINT measure_adapter(QuantumState& state, UINT target_index)
{
    Random random;
    auto gate0 = gate::P0(target_index);
    auto gate1 = gate::P1(target_index);
    std::vector<QuantumGateBase*> _gate_list = {gate0, gate1};
    delete gate0;
    delete gate1;
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
    delete buffer;

    return index;
}

void reset_qubit(QuantumState& state, UINT target_index)
{
    UINT measurement = measure_adapter(state, target_index);
    if (measurement == 1)
        gate::X(target_index)->update_quantum_state(&state);
}


struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    UINT zero_qubit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg, rcreg;
    std::map<std::size_t, bool> cvalues;
    std::unordered_map<std::string, std::stack<UINT>> qc_meas;
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
};
}


namespace cunqa {
namespace sim {
 
std::string execute_shot_(QuantumState state, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
{
    std::unordered_map<std::string, TaskState> Ts;
    GlobalState G;

    for (auto &quantum_task : quantum_tasks)
    {
        TaskState T;
        T.id = quantum_task.id;
        T.zero_qubit = G.n_qubits;
        T.it = quantum_task.circuit.begin();
        T.end = quantum_task.circuit.end();
        T.blocked = false;
        T.finished = false;
        Ts[quantum_task.id] = T;
        
        G.n_qubits += quantum_task.config.at("num_qubits").get<int>();
        G.n_clbits += quantum_task.config.at("num_clbits").get<int>();
    }
    
    // Here we add the two communication qubits
    if (size(quantum_tasks) > 1)
        G.n_qubits += 2;

    auto generate_entanglement_ = [&]() {
        reset_qubit(state, G.n_qubits - 1);
        reset_qubit(state, G.n_qubits - 2);
        // Apply H to the first entanglement qubit
        gate::H(G.n_qubits - 2)->update_quantum_state(&state);

        // Apply a CX to the second one to generate an ent pair
        gate::CNOT(G.n_qubits - 2, G.n_qubits - 1)->update_quantum_state(&state);
    };


    std::function<void(TaskState&, const JSON&)> apply_next_instr = [&](TaskState& T, const JSON& instruction = {}) {

        // This is added to be able to add instructions outside the main loop
        const JSON& inst = instruction.empty() ? *T.it : instruction;

        if (inst.contains("conditional_reg")) {
            auto v = inst.at("conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.creg[v[0]]) return;
        } else if (inst.contains("remote_conditional_reg") && inst.at("name") != "recv") { // TODO: Cambiar el nombre para el recv y para el resto
            auto v = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.rcreg[v[0]]) return;
        }

        auto qubits = inst.at("qubits").get<std::vector<UINT>>();
        auto inst_type = constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case constants::MEASURE:
        {
            UINT measurement = measure_adapter(state, qubits[0] + T.zero_qubit);
            std::vector<int> clbits = inst.at("clbits").get<std::vector<int>>();
            G.cvalues[clbits[0] + T.zero_qubit] = (measurement == 1);
            G.creg[clbits[0]] = (measurement == 1);
            break;
        }
        case constants::X:
            gate::X(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::Y:
            gate::Y(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::Z:
            gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::H:
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SDAG:
            gate::Sdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::T:
            gate::T(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::TDAG:
            gate::Tdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SX:
            gate::sqrtX(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SQRTXDAG:
            gate::sqrtXdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SQRTY:
            gate::sqrtY(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SQRTYDAG:
            gate::sqrtYdag(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::P0:
            gate::P0(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::P1:
            gate::P1(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::U1: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::U1(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::U2: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::U2(qubits[0] + T.zero_qubit, params[0], params[1])->update_quantum_state(&state);
            break;
        }
        case constants::U3: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::U3(qubits[0] + T.zero_qubit, params[0], params[1], params[2])->update_quantum_state(&state);
            break;
        }
        case constants::RX: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::RY: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::RZ: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVX: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RotInvX(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVY: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RotInvY(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::ROTINVZ: 
        {
            auto params = instruction.at("params").get<std::vector<double>>();
            gate::RotInvZ(qubits[0] + T.zero_qubit, params[0])->update_quantum_state(&state);
            break;
        }
        case constants::CX:
        {
            UINT control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            gate::CNOT(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::CZ:
        {
            UINT control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            gate::CZ(control, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        }
        case constants::ECR:
            gate::ECR(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::SWAP:
            gate::SWAP(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit)->update_quantum_state(&state);
            break;
        case constants::MEASURE_AND_SEND:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            UINT measurement = measure_adapter(state, qubits[0] + T.zero_qubit);
            int measurement_as_int = static_cast<int>(measurement);
            classical_channel->send_measure(measurement_as_int, endpoint[0]); 
            break;
        }
        case cunqa::constants::RECV:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            auto conditional_reg = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            int measurement = classical_channel->recv_measure(endpoint[0]);
            G.rcreg[conditional_reg[0]] = (measurement == 1);
            break;
        }
        case constants::QSEND:
        {
            //------------- Generate Entanglement ---------------
            gate::H(G.n_qubits - 2)->update_quantum_state(&state);
            gate::CNOT(G.n_qubits - 2, G.n_qubits - 1)->update_quantum_state(&state);
            //----------------------------------------------------

            // CX to the entangled pair
            gate::CNOT(qubits[0] + T.zero_qubit, G.n_qubits - 2)->update_quantum_state(&state);

            // H to the sent qubit
            gate::H(qubits[0] + T.zero_qubit)->update_quantum_state(&state);

            UINT result0 = measure_adapter(state, qubits[0] + T.zero_qubit);
            UINT result1 = measure_adapter(state, G.n_qubits - 2);

            G.qc_meas[T.id].push(result0);
            G.qc_meas[T.id].push(result1);
            reset_qubit(state, G.n_qubits - 2);
            reset_qubit(state, qubits[0] + T.zero_qubit);

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked = false;
            break;
        }
        case constants::QRECV:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            // Receive the measurements from the sender
            std::size_t meas1 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();
            std::size_t meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                gate::X(G.n_qubits - 1)->update_quantum_state(&state);
            }
            if (meas2) {
                gate::Z(G.n_qubits - 1)->update_quantum_state(&state);
            }

            // Swap the value to the desired qubit
            gate::SWAP(G.n_qubits - 1, qubits[0] + T.zero_qubit)->update_quantum_state(&state);
            reset_qubit(state, G.n_qubits - 1);
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                gate::CNOT(qubits[0] + T.zero_qubit, G.n_qubits - 2)->update_quantum_state(&state);

                UINT result = measure_adapter(state, G.n_qubits - 2);

                G.qc_meas[T.id].push(result);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                UINT meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    gate::Z(qubits[0] + T.zero_qubit)->update_quantum_state(&state);
                }

                T.cat_entangled = false;
            }
            break;
        }
        case constants::RCONTROL:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            UINT meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                gate::X(G.n_qubits - 1)->update_quantum_state(&state);
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            gate::H(G.n_qubits - 1)->update_quantum_state(&state);

            UINT result = measure_adapter(state, G.n_qubits - 1);
            G.qc_meas[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked = false;
            size_t erased_elements = G.qc_meas.erase(inst.at("qpus")[0]); 
            break;
        }
        default:
            std::cerr << "Instruction not suported!" << "\n" << "Instruction that failed: " << inst.dump(4) << "\n";
        } // End switch
    };

    while (!G.ended)
    {
        G.ended = true;
        for (auto& [id, T]: Ts)
        {
            if (T.finished || T.blocked)
                continue;

            apply_next_instr(T, {});

            if (!T.blocked)
                ++T.it;

            if (T.it != T.end)
                G.ended = false;
            else
                T.finished = true;
        }

    } // End one shot

    std::string result_bits(G.n_clbits, '0');
    for (const auto &[bitIndex, value] : G.cvalues)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
}


JSON QulacsSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Inside Qulacs usual simulation");
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
        std::vector<ITYPE> sample = state.sampling(shots);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        float time_taken = duration.count();

        JSON counts = convert_to_counts(sample, n_qubits);

        JSON result_json = 
        {
            {"counts", counts},
            {"time_taken", time_taken}
        };

        LOGGER_DEBUG("result_json: {}", result_json.dump());

        return result_json;

    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Qulacs simulator.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}


JSON QulacsSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    LOGGER_DEBUG("Inside Qulacs dynamic simulation");

    std::map<std::string, std::size_t> meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

    unsigned long n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks)
    {
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
    }
    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    QuantumState state(n_qubits);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++)
    {
        meas_counter[execute_shot_(state, qc.quantum_tasks, classical_channel)]++;
        state.set_zero_state();
    } // End all shots
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    reverse_bitstring_keys_json(meas_counter);
    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

} // End of sim namespace
} // End of cunqa namespace