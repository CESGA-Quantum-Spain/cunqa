
#include <unordered_map>
#include <stack>
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
struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    unsigned long zero_qubit = 0;
    unsigned long zero_clbit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg;
    std::unordered_map<std::string, std::stack<uint_t>> qc_meas;
    bool ended = false;
};


std::string execute_shot_(
    AER::AerState* state, 
    const std::vector<cunqa::QuantumTask>& quantum_tasks, 
    cunqa::comm::ClassicalChannel* classical_channel
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
        state->apply_reset({G.n_qubits - 1});
        state->apply_reset({G.n_qubits - 2});
        state->apply_h(G.n_qubits - 2);
        state->apply_mcx({G.n_qubits - 2, G.n_qubits - 1});
    };


    std::function<void(TaskState&, const cunqa::JSON&)> apply_next_instr = 
        [&](TaskState& T, const cunqa::JSON& instruction = {}) 
    {
        const cunqa::JSON& inst = instruction.empty() ? *T.it : instruction;

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
            state->apply_mcx({qubits[0] + T.zero_qubit});
            break;
        case cunqa::constants::Y:
            state->apply_mcy({qubits[0] + T.zero_qubit});
            break;
        case cunqa::constants::Z:
            state->apply_mcz({qubits[0] + T.zero_qubit});
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
        case cunqa::constants::CX:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            state->apply_mcx({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::CY:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            state->apply_mcy({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::CZ:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            state->apply_mcz({control, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::ECR:
            // TODO
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
        case cunqa::constants::CRX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
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
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            state->apply_mcrz({control, qubits[1] + T.zero_qubit}, params[0]);
            break;
        }
        case cunqa::constants::SWAP:
        {
            state->apply_mcswap({qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit});
            break;
        }
        case cunqa::constants::SEND:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();   

            for (const auto& clbit: clbits)
                classical_channel->send_measure(G.creg[clbit + T.zero_clbit], qpu_id);
            break;
        }
        case cunqa::constants::RECV:
        {
            auto qpu_id = inst.at("qpus").get<std::vector<std::string>>()[0];
            auto clbits = inst.at("clbits").get<std::vector<int>>();

            state->flush_ops(); // Execute operations to empty the buffer 
            for (const auto& clbit: clbits) {
                int measurement = classical_channel->recv_measure(qpu_id);
                G.creg[clbit + T.zero_clbit] = (measurement == 1);
            }
            break;
        }
        case cunqa::constants::CIF:
        {
            const auto& clbits = inst.at("clbits").get<std::vector<int>>();
            if (G.creg[clbits.at(0) + T.zero_clbit]) {
                for(const auto& sub_inst: inst.at("instructions")) {
                    apply_next_instr(T, sub_inst);
                }
            }
            break;
        }
        case cunqa::constants::QSEND:
        {
            // state->flush_ops();
            //------------- Generate Entanglement ---------------
            state->apply_h(G.n_qubits - 2);
            state->apply_mcx({G.n_qubits - 2, G.n_qubits - 1});
            //----------------------------------------------------

            // CX to the entangled pair
            state->apply_mcx({qubits[0] + T.zero_qubit, G.n_qubits - 2});

            // H to the sent qubit
            state->apply_h(qubits[0] + T.zero_qubit);

            uint_t result = state->apply_measure({qubits[0] + T.zero_qubit});

            G.qc_meas[T.id].push(result);
            G.qc_meas[T.id].push(state->apply_measure({G.n_qubits - 2}));
            state->apply_reset({G.n_qubits - 2, qubits[0] + T.zero_qubit});

            // Unlock QRECV
            Ts[inst.at("qpus")[0]].blocked = false;
            break;
        }
        case cunqa::constants::QRECV:
        {
            // state->flush_ops();
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
                state->apply_mcx({G.n_qubits - 1});
            }
            if (meas2) {
                state->apply_mcz({G.n_qubits - 1});
            }

            // Swap the value to the desired qubit
            state->apply_mcswap({G.n_qubits - 1, qubits[0] + T.zero_qubit});
            state->apply_reset({G.n_qubits - 1});
            break;
        }
        case cunqa::constants::EXPOSE:
        {
            // state->flush_ops();
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                state->apply_mcx({qubits[0] + T.zero_qubit, G.n_qubits - 2});

                uint_t result = state->apply_measure({G.n_qubits - 2});

                G.qc_meas[T.id].push(result);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                uint_t meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    state->apply_mcz({qubits[0] + T.zero_qubit}); 
                }

                T.cat_entangled = false;
            }
            break;
        }
        case cunqa::constants::RCONTROL:
        {
            // state->flush_ops();
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            uint_t meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                state->apply_mcx({G.n_qubits - 1});
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            state->apply_h(G.n_qubits - 1);

            uint_t result = state->apply_measure({G.n_qubits - 1});
            G.qc_meas[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked = false;
            size_t erased_elements = G.qc_meas.erase(inst.at("qpus")[0]); 
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
        run_config_json["seed_simulator"] = quantum_task.config.at("seed");
        Config aer_config(run_config_json);
        Noise::NoiseModel noise_model;

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

JSON AerSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    LOGGER_DEBUG("Aer dynamic simulation");

    std::map<std::string, std::size_t> meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();
    std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();

    AER::AerState state; // Before: AER::AerState* state = new AER::AerState();
    std::string sim_method = (method == "automatic") ? "statevector" : method;
    std::string device = qc.quantum_tasks[0].config.at("device")["device_name"];
    state.configure("method", sim_method);
    state.configure("device", device);
    state.configure("precision", "double");
    state.configure("seed_simulator", std::to_string(qc.quantum_tasks[0].config.at("seed").get<int>()));
    reg_t target_gpus = (device == "GPU") ? qc.quantum_tasks[0].config.at("device")["target_devices"].get<reg_t>() : reg_t();

    unsigned long n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks)
    {
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
    }
    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    reg_t qubit_ids;
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++)
    {
        qubit_ids = state.allocate_qubits(n_qubits);
        state.initialize();
        /* WARNING. The "set_target_gpus" method is particular of CUNQA-Aer fork. Comment it if you are using another Aer version. */
        state.set_target_gpus(target_gpus);
        meas_counter[execute_shot_(&state, qc.quantum_tasks, classical_channel)]++;
        state.clear();
    } // End all shots
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = end - start;
    float time_taken = duration.count();

    //delete state;

    JSON result_json = {
        {"counts", meas_counter},
        {"time_taken", time_taken}};
    return result_json;
}

} // End of sim namespace
} // End of cunqa namespace
