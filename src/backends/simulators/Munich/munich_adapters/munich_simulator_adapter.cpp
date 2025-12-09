
#include "munich_simulator_adapter.hpp"
#include "munich_helpers.hpp"

#include <unordered_map>
#include <stack>
#include <chrono>
#include <thread>
#include <functional>

#include "quantum_task.hpp"
#include "backends/simulators/simulator_strategy.hpp"
#include "utils/helpers/reverse_bitstring.hpp"

#include "logger.hpp"

using namespace qc;
using namespace cunqa;

namespace {
const std::unordered_map<int, OpType> MUNICH_INSTRUCTIONS_MAP = {
    // MEASURE
    {constants::MEASURE, OpType::Measure},

    // ONE GATE NO PARAM
    {constants::ID, OpType::I},
    {constants::X, OpType::X},
    {constants::Y, OpType::Y},
    {constants::Z, OpType::Z},
    {constants::H, OpType::H},
    {constants::SX, OpType::SX},

    // ONE GATE PARAM
    {constants::RX, OpType::RX},
    {constants::RY, OpType::RY},
    {constants::RZ, OpType::RZ},

    // TWO GATE NO PARAM
    {constants::CX, OpType::X},
    {constants::CY, OpType::Y},
    {constants::CZ, OpType::Z},
    {constants::SWAP, OpType::SWAP},
    {constants::ECR, OpType::ECR},

    // TWO GATE PARAM
    {constants::CRX, OpType::RX},
    {constants::CRY, OpType::RY},
    {constants::CRZ, OpType::RZ}
};

struct TaskState {
    std::string id;
    JSON::const_iterator it, end;
    int zero_qubit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas; // !!!!!!
};

struct GlobalState {
    int n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg, rcreg;
    std::map<std::size_t, bool> cvalues;
    std::unordered_map<std::string, std::stack<int>> qc_meas;
    bool ended = false;
    comm::ClassicalChannel* chan = nullptr;
};

} // End of anonymous namespace

namespace cunqa {
namespace sim {

std::string CircuitSimulatorAdapter::execute_shot_(const std::vector<QuantumTask> &quantum_tasks, comm::ClassicalChannel *classical_channel)
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

    initializeSimulationAdapter(G.n_qubits);

    auto generate_entanglement_ = [&]() {

        // Apply H to the first entanglement qubit
        auto std_op1 = std::make_unique<StandardOperation>(G.n_qubits - 2, OpType::H);
        applyOperationToStateAdapter(std::move(std_op1));

        // Apply a CX to the second one to generate an ent pair
        Control control(G.n_qubits - 2);
        auto std_op2 = std::make_unique<StandardOperation>(control, G.n_qubits - 1, OpType::X);
        applyOperationToStateAdapter(std::move(std_op2));
    };

    std::function<void(TaskState&, const JSON&)> apply_next_instr = [&](TaskState& T, const JSON& instruction = {}) {

        // This is added to be able to add instructions outside the main loop
        
        const JSON& inst = instruction.empty() ? *T.it : instruction;
        //LOGGER_DEBUG("INSTRUCCIÓN: {}", inst.dump());

        // Check if the ifs below are really needed
        if (inst.contains("conditional_reg")) {
            auto v = inst.at("conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.creg[v[0]]) return;
        } else if (inst.contains("remote_conditional_reg") && inst.at("name") != "recv") { // TODO: Cambiar el nombre para el recv y para el resto
            auto v = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.rcreg[v[0]]) return;
        }

        std::vector<int> qubits = inst.at("qubits").get<std::vector<int>>();

        auto inst_type = constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());
        
        switch (inst_type) {
        case constants::MEASURE:
        {

            char char_measurement = measureAdapter(qubits[0] + T.zero_qubit);

            std::vector<int> clbits = inst.at("clbits").get<std::vector<int>>();

            G.cvalues[clbits[0] + T.zero_qubit] = (char_measurement == '1');
            G.creg[clbits[0]] = (char_measurement == '1');

            break;
        }
        case constants::X:
        case constants::Y:
        case constants::Z:
        case constants::H:
        case constants::SX:
        case constants::RX:
        case constants::RY:
        case constants::RZ:
        {
            std::unique_ptr<StandardOperation> simple_gate;
            if (inst.contains("params")) {
                auto params = inst.at("params").get<std::vector<double>>();
                simple_gate = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
            } else {
                simple_gate = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            }
            applyOperationToStateAdapter(std::move(simple_gate));
            break;
        }
        case constants::ECR:
        case constants::SWAP:
        {
            qc::Targets targets = {static_cast<unsigned int>(G.n_qubits - 1), static_cast<unsigned int>(qubits[0] + T.zero_qubit)};
            auto two_gate = std::make_unique<qc::StandardOperation>(targets, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::CX:
        case constants::CY:
        case constants::CZ:
        {
            Control control(qubits[0] + T.zero_qubit);
            auto two_gate = std::make_unique<StandardOperation>(control, qubits[1] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type));
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::CRX:
        case constants::CRY:
        case constants::CRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            Control control(qubits[0] + T.zero_qubit);
            auto two_gate = std::make_unique<StandardOperation>(control, qubits[1] + T.zero_qubit, MUNICH_INSTRUCTIONS_MAP.at(inst_type), params);
            applyOperationToStateAdapter(std::move(two_gate));
            break;
        }
        case constants::C_IF_H:
        case constants::C_IF_X:
        case constants::C_IF_Y:
        case constants::C_IF_Z:
        case constants::C_IF_ECR:
        case constants::C_IF_RX:
        case constants::C_IF_RY:
        case constants::C_IF_RZ:
            // TODO: Look how Munich natively applies C_IFs operations
            /* clreg = std::make_pair(conditional_reg[0], 1);
            auto std_op = std::make_unique<StandardOperation>(qubits[1] + T.zero_qubit, OpType::X);
            c_op = std::make_unique<ClassicControlledOperation>(std_op, clreg);
            CCcircsim.CCapplyOperationToState(c_op); */
            break;
        case constants::MEASURE_AND_SEND:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            char char_measurement = measureAdapter(qubits[0] + T.zero_qubit);
            int measurement = char_measurement - '0';
            classical_channel->send_measure(measurement, endpoint[0]);
            break;
        }
        case constants::RECV:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            auto conditional_reg = inst.at("remote_conditional_reg").get<std::vector<size_t>>();
            int measurement = classical_channel->recv_measure(endpoint[0]);
            G.rcreg[conditional_reg[0]] = (measurement == 1);
            LOGGER_DEBUG("El índice {} tiene valor {}", conditional_reg[0], G.rcreg[conditional_reg[0]]);
            break;
        }
        case constants::QSEND:
        {
            generate_entanglement_();

            // CX to the entangled pair
            Control control(qubits[0] + T.zero_qubit);
            auto x = std::make_unique<StandardOperation>(control, G.n_qubits - 2, OpType::X);
            applyOperationToStateAdapter(std::move(x));

            // H to the sent qubit
            auto h = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, OpType::H);
            applyOperationToStateAdapter(std::move(h));

            int result = measureAdapter(qubits[0] + T.zero_qubit) - '0';

            G.qc_meas[T.id].push(result);
            G.qc_meas[T.id].push(measureAdapter(G.n_qubits - 2) - '0');

            // We reset to 0 the qubit sent and the EPR (we cannot use the reset op in DD)
            if (result)
            {
                auto reset_teleported = std::make_unique<StandardOperation>(qubits[0] + T.zero_qubit, OpType::X);
                applyOperationToStateAdapter(std::move(reset_teleported));
                auto reset_epr = std::make_unique<StandardOperation>(G.n_qubits - 2, OpType::X);
                applyOperationToStateAdapter(std::move(reset_epr));
            }

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
            int meas1 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();
            int meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            // Apply, conditioned to the measurement, the X and Z gates
            if (meas1) {
                auto x = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::X);
                applyOperationToStateAdapter(std::move(x));
            }
            if (meas2) {
                auto z = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::Z);
                applyOperationToStateAdapter(std::move(z));
            }

            // Swap the value to the desired qubit
            Targets targets = {static_cast<unsigned int>(G.n_qubits - 1), static_cast<unsigned int>(qubits[0] + T.zero_qubit)};
            auto swap = std::make_unique<StandardOperation>(targets, OpType::SWAP);
            applyOperationToStateAdapter(std::move(swap));

            int result = measureAdapter(G.n_qubits - 1) - '0';
            if (result) {
                auto reset_epr = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::X);
                applyOperationToStateAdapter(std::move(reset_epr));
            }
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                Control control(qubits[0] + T.zero_qubit);
                auto cx = std::make_unique<StandardOperation>(control, G.n_qubits - 2, OpType::X);
                applyOperationToStateAdapter(std::move(cx));

                int result = measureAdapter(G.n_qubits - 2) - '0';

                G.qc_meas[T.id].push(result);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                int meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    auto z = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::Z);
                    applyOperationToStateAdapter(std::move(z));
                }
            }
            break;
        }
        case constants::RCONTROL:
        {
            if (!G.qc_meas.contains(inst.at("qpus")[0])) {
                T.blocked = true;
                return;
            }

            int meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                auto x = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::X);
                applyOperationToStateAdapter(std::move(x));
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            auto h = std::make_unique<StandardOperation>(G.n_qubits - 1, OpType::H);
            applyOperationToStateAdapter(std::move(h));

            int result = measureAdapter(G.n_qubits - 1) - '0';
            G.qc_meas[T.id].push(result);

            Ts[inst.at("qpus")[0]].blocked = false;
            break;
        }
        default:
            std::cerr << "Instruction not suported!" << "\n";
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

    // result is a map from the cbit index to the Boolean value
    std::string result_bits(G.n_clbits, '0');
    for (const auto &[bitIndex, value] : G.cvalues)
    {
        result_bits[G.n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return result_bits;
}


JSON CircuitSimulatorAdapter::simulate(const Backend* backend)
{
    try
    {   
        auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
        auto quantum_task = p_qca->quantum_tasks[0];

        // TODO: Change the format with the free functions
        std::string circuit = quantum_task_to_Munich(quantum_task);
        auto mqt_circuit = std::make_unique<QuantumComputation>(std::move(QuantumComputation::fromQASM(circuit)));

        float time_taken;
        int n_qubits = quantum_task.config.at("num_qubits");

        JSON noise_model_json = backend->config.at("noise_model");
        if (!noise_model_json.empty()) {
            LOGGER_DEBUG("Noise model execution");
            const ApproximationInfo approx_info{noise_model_json["step_fidelity"], noise_model_json["approx_steps"], ApproximationInfo::FidelityDriven};
            StochasticNoiseSimulator sim(std::move(mqt_circuit), approx_info, quantum_task.config["seed"], "APD", noise_model_json["noise_prob"],
                                            noise_model_json["noise_prob_t1"], noise_model_json["noise_prob_multi"]);

            auto start = std::chrono::high_resolution_clock::now();
            auto result = sim.simulate(quantum_task.config["shots"]);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                LOGGER_DEBUG("Result non empty");
                reverse_bitstring_keys_json(result);
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        } else {
            CircuitSimulator sim(std::move(mqt_circuit));

            auto start = std::chrono::high_resolution_clock::now();
            // TODO: Change this to directly call the simulate without creating a new instance?
            auto result = sim.simulate(quantum_task.config["shots"]);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            time_taken = duration.count();

            if (!result.empty()) {
                LOGGER_DEBUG("Result non empty");
                reverse_bitstring_keys_json(result);
                return {{"counts", result}, {"time_taken", time_taken}};
            }
            throw std::runtime_error("QASM format is not correct.");
        }
    }
    catch (const std::exception &e)
    {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Munich simulator.");
        return {{"ERROR", std::string(e.what()) + ". Try checking the format of the circuit sent and/or of the noise model."}};
    }
    return {};
}

JSON CircuitSimulatorAdapter::simulate(comm::ClassicalChannel *classical_channel)
{
    // TODO: Avoid the static casting?
    auto p_qca = static_cast<QuantumComputationAdapter *>(qc.get());
    std::map<std::string, std::size_t> meas_counter;

    // This is for distinguising classical and quantum communications
    // TODO: Make it more clear
    /* if (classical_channel && p_qca->quantum_tasks.size() == 1)
    {
        std::vector<std::string> connect_with = p_qca->quantum_tasks[0].sending_to;
        classical_channel->connect(connect_with);
    } */

    auto shots = p_qca->quantum_tasks[0].config.at("shots").get<std::size_t>();
    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < shots; i++)
    {
        meas_counter[execute_shot_(p_qca->quantum_tasks, classical_channel)]++;
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