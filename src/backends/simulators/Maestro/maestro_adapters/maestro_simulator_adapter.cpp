
#include <unordered_map>
#include <stack>
#include <chrono>
#include <functional>
#include <cstdlib>

#include "utils/constants.hpp"
#include "utils/helpers/reverse_bitstring.hpp"

#include "maestro_simulator_adapter.hpp"
#include "Simulator.hpp"

#include "logger.hpp"




namespace {
struct TaskState {
    std::string id;
    cunqa::JSON::const_iterator it, end;
    unsigned long zero_qubit = 0;
    bool finished = false;
    bool blocked = false;
    bool cat_entangled = false;
    std::stack<int> telep_meas;
};

struct GlobalState {
    unsigned long n_qubits = 0, n_clbits = 0;
    std::map<std::size_t, bool> creg, rcreg;
    std::map<std::size_t, bool> cvalues;
    std::unordered_map<std::string, std::stack<int>> qc_meas;
    bool ended = false;
    cunqa::comm::ClassicalChannel* chan = nullptr;
};
}

namespace cunqa {
namespace sim {

std::string execute_shot_(Simulator& simulator, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
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
        const unsigned long int q[]{ G.n_qubits - 1, G.n_qubits - 2 };

		simulator.ApplyReset(q, 2);
        // Apply H to the first entanglement qubit
        simulator.ApplyH(G.n_qubits - 2);

        // Apply a CX to the second one to generate an ent pair
        simulator.ApplyCX(G.n_qubits - 2, G.n_qubits - 1);
    };

    std::function<void(TaskState&, const JSON&)> apply_next_instr = [&](TaskState& T, const JSON& instruction = {}) {

        // This is added to be able to add instructions outside the main loop
        const JSON& inst = instruction.empty() ? *T.it : instruction;

        if (inst.contains("conditional_reg")) {
            auto v = inst.at("conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.creg[v[0]]) return;
        } else if (inst.contains("remote_conditional_reg") && inst.at("name") != "recv") { 
            auto v = inst.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
            if (!G.rcreg[v[0]]) return;
        }

        std::vector<int> qubits = inst.at("qubits").get<std::vector<int>>();
        auto inst_type = constants::INSTRUCTIONS_MAP.at(inst.at("name").get<std::string>());

        switch (inst_type)
        {
        case constants::MEASURE:
        {
            auto clreg = inst.at("clreg").get<std::vector<std::uint64_t>>();
            const unsigned long int q[]{ qubits[0] + T.zero_qubit };
            const unsigned long long int measurement = simulator.Measure(q, 1);
            G.cvalues[qubits[0] + T.zero_qubit] = (measurement == 1);
            if (!clreg.empty())
            {
                G.creg[clreg[0]] = (measurement == 1);
            }
            break;
        }
        case constants::X:
            simulator.ApplyX(qubits[0] + T.zero_qubit);
            break;
        case constants::Y:
            simulator.ApplyY(qubits[0] + T.zero_qubit);
            break;
        case constants::Z:
            simulator.ApplyZ(qubits[0] + T.zero_qubit);
            break;
        case constants::H:
            simulator.ApplyH(qubits[0] + T.zero_qubit);
            break;
        case constants::SX:
            simulator.ApplySX(qubits[0] + T.zero_qubit);
            break;
        case constants::CX:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCX(control, qubits[1] + T.zero_qubit);
            break;
        }
        case constants::CY:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCY(control, qubits[1] + T.zero_qubit);
            break;
        }
        case constants::CZ:
        {
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCZ(control, qubits[1] + T.zero_qubit);
            break;
        }
        case constants::ECR:
            // TODO
            break;
        case constants::RX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            simulator.ApplyRx(qubits[0] + T.zero_qubit, params[0]);
            break;
        }
        case constants::RY:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            simulator.ApplyRy(qubits[0] + T.zero_qubit, params[0]);
            break;
        }
        case constants::RZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            simulator.ApplyRz(qubits[0] + T.zero_qubit, params[0]);
            break;
        }
        case constants::CRX:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCRx(control, qubits[1] + T.zero_qubit, params[0]);
            break;
        }
        case constants::CRY:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCRy(control, qubits[1] + T.zero_qubit, params[0]);
            break;
        }
        case constants::CRZ:
        {
            auto params = inst.at("params").get<std::vector<double>>();
            unsigned long control = (qubits[0] == -1) ? G.n_qubits - 1 : qubits[0] + T.zero_qubit;
            simulator.ApplyCRz(control, qubits[1] + T.zero_qubit, params[0]);
            break;
        }
        case constants::C_IF_H:
        case constants::C_IF_X:
        case constants::C_IF_Y:
        case constants::C_IF_Z:
        case constants::C_IF_CX:
        case constants::C_IF_CY:
        case constants::C_IF_CZ:
        case constants::C_IF_ECR:
        case constants::C_IF_RX:
        case constants::C_IF_RY:
        case constants::C_IF_RZ:
            // Managed by use
            break;
        case constants::SWAP:
        {
            simulator.ApplySwap(qubits[0] + T.zero_qubit, qubits[1] + T.zero_qubit);
            break;
        }
        case constants::MEASURE_AND_SEND:
        {
            auto endpoint = inst.at("qpus").get<std::vector<std::string>>();
            const unsigned long int q[]{ qubits[0] + T.zero_qubit };
            int measurement_as_int = static_cast<int>(simulator.Measure(q, 1));
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
            simulator.ApplyH(G.n_qubits - 2);
			simulator.ApplyCX(G.n_qubits - 2, G.n_qubits - 1);
            //----------------------------------------------------

            // CX to the entangled pair
            simulator.ApplyCX(qubits[0] + T.zero_qubit, G.n_qubits - 2);

            // H to the sent qubit
            simulator.ApplyH(qubits[0] + T.zero_qubit);

            const unsigned long int q1[]{ qubits[0] + T.zero_qubit };
            int measurement_as_int = static_cast<int>(simulator.Measure(q1, 1));
            G.qc_meas[T.id].push(measurement_as_int);

            const unsigned long int q2[]{ G.n_qubits - 2 };
            int aux_meas = static_cast<int>(simulator.Measure(q2, 1));
            G.qc_meas[T.id].push(aux_meas);

            const unsigned long int q3[]{ G.n_qubits - 2, qubits[0] + T.zero_qubit };
            simulator.ApplyReset(q3, 2);

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
                simulator.ApplyX(G.n_qubits - 1);
            }
            if (meas2) {
                simulator.ApplyZ(G.n_qubits - 1);
            }

            // Swap the value to the desired qubit
            simulator.ApplySwap(G.n_qubits - 1, qubits[0] + T.zero_qubit);

            const unsigned long int q[]{ G.n_qubits - 1 };
			simulator.ApplyReset(q, 1);
            break;
        }
        case constants::EXPOSE:
        {
            if (!T.cat_entangled) {
                generate_entanglement_();

                // CX to the entangled pair
                simulator.ApplyCX(qubits[0] + T.zero_qubit, G.n_qubits - 2);

                const unsigned long int q[]{ G.n_qubits - 2 };
                int measurement_as_int = static_cast<int>(simulator.Measure(q, 1));

                G.qc_meas[T.id].push(measurement_as_int);
                T.cat_entangled = true;
                T.blocked = true;
                Ts[inst.at("qpus")[0]].blocked = false;
                return;
            } else {
                int meas = G.qc_meas[inst.at("qpus")[0]].top();
                G.qc_meas[inst.at("qpus")[0]].pop();

                if (meas) {
                    simulator.ApplyZ(qubits[0] + T.zero_qubit);
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

            int meas2 = G.qc_meas[inst.at("qpus")[0]].top();
            G.qc_meas[inst.at("qpus")[0]].pop();

            if (meas2) {
                simulator.ApplyX(G.n_qubits - 1);
            }

            for(const auto& sub_inst: inst.at("instructions")) {
                apply_next_instr(T, sub_inst);
            }

            simulator.ApplyH(G.n_qubits - 1);

            const unsigned long int q[]{ G.n_qubits - 1 };
            int measurement_as_int = static_cast<int>(simulator.Measure(q, 1));
            G.qc_meas[T.id].push(measurement_as_int);

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

JSON MaestroSimulatorAdapter::simulate(const Backend* backend)
{
    LOGGER_DEBUG("Maestro usual simulation");
    const char* HOME = std::getenv("HOME");
    std::string libmaestro_path = std::string(HOME) + "/lib64/libmaestro.so";
    try {
        auto quantum_task = qc.quantum_tasks[0];

        unsigned long n_qbits = quantum_task.config.at("num_qubits").get<unsigned long>();
 
        JSON circuit_json = quantum_task.circuit;
        JSON run_config_json(quantum_task.config);

        SimpleSimulator simulator;
        if (simulator.Init(libmaestro_path.c_str()))
        {
			unsigned long int simulatorHandle = simulator.CreateSimpleSimulator(n_qbits);
            if (simulatorHandle == 0)
            {
                LOGGER_ERROR("Error creating the Maestro simulator.");
                return {{"ERROR", "Unable to create the Maestro simulator."}};
			}

            std::string method = quantum_task.config.at("method").get<std::string>();
            std::string sim_name;

            if (quantum_task.config.contains("simulator"))
                sim_name = quantum_task.config.at("simulator").get<std::string>();

			// -1 for simulator type means both qiskit aer and qcsim
			// -1 for simulation type means automatic, that is... statevector + stabilizer + matrix product state
            int simulatorType = -1; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
            int simulationType = -1; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state

            // TODO: set the method into the estimator
            // also the parameters if any and so on
            if (method != "automatic")
            {
                if (method == "statevector")
                {
                    simulationType = 0;
                }
                else if (method == "matrix_product_state")
                {
                    // matrix_product_state_truncation_threshold
                    // matrix_product_state_max_bond_dimension
                    // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
                    simulationType = 1;
                }
                else if (method == "stabilizer")
                {
                    simulationType = 2;
                }
                else if (method == "tensor_network")
                {
                    // use qcsim for this, qiskit aer is not compiled with tensor network support
                    // in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
                    simulationType = 3;
                }
            }

            if (sim_name == "qiskit" || sim_name == "aer")
            {
				simulatorType = 0; // qiskit aer
            }
            else if (sim_name == "qcsim")
            {
                simulatorType = 1; // qcsim
            }
            else if (sim_name == "gpu" && simulationType != 2 && simulationType != 3) // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
            {
                simulatorType = 4; // gpu
            }
            else if (sim_name == "composite_qiskit")
            {
                simulatorType = 2; // p-blocks qiskit aer
                simulationType = 0; // statevector
            }
            else if (sim_name == "composite_qcsim")
            {
                simulatorType = 3; // p-blocks qcsim
                simulationType = 0; // statevector
			}

            if (simulatorType != -1 || simulationType != -1) // if both unspecified, leave the default
            {
                if (simulatorType == -1 && simulationType != -1) // simulator type not specified
                {
                    // both qiskit aer and qcsim
                    simulator.RemoveAllOptimizationSimulatorsAndAdd(0, simulationType);
                    simulator.AddOptimizationSimulator(1, simulationType);
                }
                else if (simulationType == -1)
                {
					simulator.RemoveAllOptimizationSimulatorsAndAdd(simulatorType, 0); // statevector
                    simulator.RemoveAllOptimizationSimulatorsAndAdd(simulatorType, 1); // mps
                    simulator.RemoveAllOptimizationSimulatorsAndAdd(simulatorType, 2); // stabilizer
                }
                else
                {
                    simulator.RemoveAllOptimizationSimulatorsAndAdd(simulatorType, simulationType);
                }
            }

			char* result = simulator.SimpleExecute(circuit_json.dump().c_str(), run_config_json.dump().c_str());

            if (result)
            {
                JSON maestro_result = JSON::parse(result);
                simulator.FreeResult(result);

                JSON result_json = {
                {"counts", maestro_result.at("counts").get<JSON>()},
                {"time_taken", maestro_result.at("time_taken").get<JSON>()}
                };

				return result_json;
            }
            else
            {
                LOGGER_ERROR("Error executing the circuit in the Maestro simulator.");
                return {{"ERROR", "Unable to execute the circuit in the Maestro simulator."}};
			}
        }
        else
        {
            LOGGER_ERROR("Error initializing the Maestro library.");
            return {{"ERROR", "Unable to initialize the Maestro library."}};
		}
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the Maestro simulator.\n\tTry checking the format of the circuit sent.");
        return {{"ERROR", std::string(e.what())}};
    }

    return {};
}


JSON MaestroSimulatorAdapter::simulate(comm::ClassicalChannel* classical_channel)
{
    LOGGER_DEBUG("Maestro dynamic simulation");
    const char* HOME = std::getenv("HOME");
    std::string libmaestro_path = std::string(HOME) + "/lib64/libmaestro.so";
    std::map<std::string, std::size_t> meas_counter;
    
    auto shots = qc.quantum_tasks[0].config.at("shots").get<std::size_t>();

    unsigned long n_qubits = 0;
    for (auto &quantum_task : qc.quantum_tasks)
    {
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
    }
    if (size(qc.quantum_tasks) > 1)
        n_qubits += 2;

    Simulator simulator;
    if (simulator.Init(libmaestro_path.c_str()))
    {
        std::string method = qc.quantum_tasks[0].config.at("method").get<std::string>();
        // is qcsim or gpu specified?
		// otherwise use qiskit aer by default
        std::string sim_name;
        
        if (qc.quantum_tasks[0].config.contains("simulator"))
            sim_name = qc.quantum_tasks[0].config.at("simulator").get<std::string>();

		int simulatorType = 0; // qiskit aer by default, 1 = qcsim, 2 = p-blocks qiskit aer, 3 = p-blocks qcsim, 4 = gpu
		int simulationType = 0; // statevector by default, 1 = matrix product state, 2 = stabilizer, 3 = matrix product state
        // the p-blocks simulators use statevector only

        if (method == "automatic")
        {
            // TODO: use the estimator to pick the best method
            // need to use the given circuit(s) in quantum_tasks for that, also the number of shots and the usage of multithreading in the simulator (as opposed to using multiple simulators in different threads)!

			// for now pick up the statevector simulator
        }
        else if (method == "statevector")
        {
			simulationType = 0;
        }
        else if (method == "matrix_product_state")
        {
            // matrix_product_state_truncation_threshold
            // matrix_product_state_max_bond_dimension
            // mps_sample_measure_algorithm - if 'mps_probabilities', use MPS 'measure no collapse'
            simulationType = 1;
        }
        else if (method == "stabilizer")
        {
            simulationType = 2;
        }
        else if (method == "tensor_network")
        {
			// use qcsim for this, qiskit aer is not compiled with tensor network support
			// in the future we'll need to discriminate between qcsim and gpu as well, but we don't have yet gpu tensor network support
            simulationType = 3;
        }

        if (sim_name == "qcsim")
        {
            simulatorType = 1; // qcsim
        }
		else if (sim_name == "gpu" && simulationType != 2 && simulationType == 3) // stabilizer and tensor network not supported on gpu (tensor network will be in the future)
        {
            simulatorType = 4; // gpu
        }
        else if (sim_name == "composite_qiskit")
        {
            simulatorType = 2; // p-blocks qiskit aer
			simulationType = 0; // statevector
        }
        else if (sim_name == "composite_qcsim")
        {
            simulatorType = 3; // p-blocks qcsim
            simulationType = 0; // statevector
		}

        if (simulator.CreateSimulator(simulatorType, simulationType))
        {
            auto start = std::chrono::high_resolution_clock::now();
            for (std::size_t i = 0; i < shots; i++)
            {
                simulator.AllocateQubits(n_qubits); // From CUNQA: Maybe allocate after shots and restart the state in each shot for better performance?
                simulator.InitializeSimulator();
                meas_counter[execute_shot_(simulator, qc.quantum_tasks, classical_channel)]++;
                simulator.ClearSimulator();
            } // End all shots
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            float time_taken = duration.count();

            reverse_bitstring_keys_json(meas_counter);
            JSON result_json = {
                {"counts", meas_counter},
                {"time_taken", time_taken} };

            return result_json;
        }
        else
        {
            LOGGER_ERROR("Error creating the Maestro simulator.");
            return { {"ERROR", "Unable to create the Maestro simulator."} };
        }
    }
    else
    {
        LOGGER_ERROR("Error initializing the Maestro library.");
        return { {"ERROR", "Unable to initialize the Maestro library."} };
    }
}


} // End of sim namespace
} // End of cunqa namespace
