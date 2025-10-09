
#include <unordered_map>
#include <stack>
#include <chrono>

#include "maestro_simulator_adapter.hpp"

#include "utils/constants.hpp"

#include "logger.hpp"

#include "Simulator.hpp"


namespace cunqa {
namespace sim {


std::string execute_shot_(Simulator& simulator, const std::vector<QuantumTask>& quantum_tasks, comm::ClassicalChannel* classical_channel)
{
    std::vector<JSON::const_iterator> its;
    std::vector<JSON::const_iterator> ends;
    std::vector<bool> finished;
    std::unordered_map<std::string, bool> blocked;
    std::vector<unsigned long> zero_qubit;
    std::vector<unsigned long> zero_clbit;
    unsigned long n_qubits = 0;
    unsigned long n_clbits = 0;

    for (auto& quantum_task : quantum_tasks)
    {
        zero_qubit.push_back(n_qubits);
        zero_clbit.push_back(n_clbits);
        its.push_back(quantum_task.circuit.begin());
        ends.push_back(quantum_task.circuit.end());
        n_qubits += quantum_task.config.at("num_qubits").get<unsigned long>();
        n_clbits += quantum_task.config.at("num_clbits").get<unsigned long>();
        blocked[quantum_task.id] = false;
        finished.push_back(false);
    }

    std::string resultString(n_clbits, '0');
    if (size(quantum_tasks) > 1)
        n_qubits += 2;

    std::vector<unsigned long> qubits;
    std::map<std::size_t, bool> classic_values;
    std::map<std::size_t, bool> classic_reg;
    std::map<std::size_t, bool> r_classic_reg;
    std::unordered_map<std::string, std::stack<std::size_t>> qc_meas;

    bool ended = false;
	bool lock_ent_qubits = false;
    while (!ended)
    {
        ended = true;
        for (size_t i = 0; i < its.size(); ++i)
        {
            if (finished[i] || blocked[quantum_tasks[i].id])
                continue;

            auto& instruction = *its[i];
            qubits = instruction.at("qubits").get<std::vector<unsigned long>>();
            switch (constants::INSTRUCTIONS_MAP.at(instruction.at("name")))
            {
            case constants::ID:
				break;
            case constants::MEASURE:
            {
                auto clreg = instruction.at("clreg").get<std::vector<std::uint64_t>>();
				const unsigned long int q[]{ qubits[0] + zero_qubit[i] };
                const unsigned long long int measurement = simulator.Measure(q, 1);
                classic_values[qubits[0] + zero_qubit[i]] = (measurement == 1);
                if (!clreg.empty())
                {
                    classic_reg[clreg[0]] = (measurement == 1);
                }
                break;
            }
            case constants::X:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyX(qubits[0] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyX(qubits[0] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyX(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::Y:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyY(qubits[0] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyY(qubits[0] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyY(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::Z:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyZ(qubits[0] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyZ(qubits[0] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyZ(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::H:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyH(qubits[0] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyH(qubits[0] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyH(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::SX:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplySX(qubits[0] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplySX(qubits[0] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplySX(qubits[0] + zero_qubit[i]);
                }
                break;
            }
            case constants::SWAP:
                {
                    if (instruction.contains("conditional_reg")) {
                        auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                        if (classic_reg[conditional_reg[0]]) {
                            simulator.ApplySwap(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                        }
                    }
                    else if (instruction.contains("remote_conditional_reg")) {
                        auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                        if (r_classic_reg[conditional_reg[0]]) {
                            simulator.ApplySwap(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                        }
                    }
                    else {
                        simulator.ApplySwap(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                    break;
                }
            case constants::CX:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCX(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCX(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyCX(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                }
                break;
            }
            case constants::CY:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCY(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCY(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyCY(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                }
                break;
            }
            case constants::CZ:
            {
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCZ(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCZ(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                    }
                }
                else {
                    simulator.ApplyCZ(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i]);
                }
                break;
            }
            case constants::ECR:
                // TODO
                break;
			case constants::CECR:
                // TODO
                break;
            // also others might miss, like RXX, RXY and so on...
            case constants::RX:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRx(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRx(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyRx(qubits[0] + zero_qubit[i], params[0]);
                }
                break;
            }
            case constants::RY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRy(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRy(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyRy(qubits[0] + zero_qubit[i], params[0]);
                }
                break;
            }
            case constants::RZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRz(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyRz(qubits[0] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyRz(qubits[0] + zero_qubit[i], params[0]);
                }
                break;
            }
            case constants::CRX:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRx(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRx(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyCRx(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                }
                break;
            }
            case constants::CRY:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRy(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRy(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyCRy(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                }
                break;
            }
            case constants::CRZ:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                if (instruction.contains("conditional_reg")) {
                    auto conditional_reg = instruction.at("conditional_reg").get<std::vector<std::uint64_t>>();
                    if (classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRz(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else if (instruction.contains("remote_conditional_reg")) {
                    auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                    if (r_classic_reg[conditional_reg[0]]) {
                        simulator.ApplyCRz(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                    }
                }
                else {
                    simulator.ApplyCRz(qubits[0] + zero_qubit[i], qubits[1] + zero_qubit[i], params[0]);
                }
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
                // Already managed on each individual gate
                break;
            case constants::MEASURE_AND_SEND:
            {
                auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                const unsigned long int q[]{ qubits[0] + zero_qubit[i] };
                int measurement_as_int = static_cast<int>(simulator.Measure(q, 1));
                classical_channel->send_measure(measurement_as_int, endpoint[0]);
                break;
            }
            case cunqa::constants::RECV:
            {
                auto endpoint = instruction.at("qpus").get<std::vector<std::string>>();
                auto conditional_reg = instruction.at("remote_conditional_reg").get<std::vector<std::uint64_t>>();
                int measurement = classical_channel->recv_measure(endpoint[0]);
                r_classic_reg[conditional_reg[0]] = (measurement == 1);
                break;
            }
            case constants::QSEND:
            {
				if (lock_ent_qubits) // if the entanglement qubits are in use by another quantum task, wait for them to be free
                    continue;

				const unsigned long entSrc = n_qubits - 2;
				const unsigned long entTgt = n_qubits - 1;
				const unsigned long srcQubit = qubits[0] + zero_qubit[i];

                //------------- Generate Entanglement ---------------
				simulator.ApplyH(entSrc);
				simulator.ApplyCX(entSrc, entTgt);
                //----------------------------------------------------

                // CX to the entangled pair
				simulator.ApplyCX(srcQubit, entSrc);

                // H to the sent qubit
				simulator.ApplyH(srcQubit);

                const unsigned long int q[]{ srcQubit };
				const unsigned long long int result = simulator.Measure(q, 1);

                qc_meas[quantum_tasks[i].id].push(result);

				const unsigned long int q2[]{ entSrc };
                qc_meas[quantum_tasks[i].id].push(simulator.Measure(q2, 1));
				
                const unsigned long int q3[]{ entSrc, srcQubit };
				simulator.ApplyReset(q3, 2);

                // Unlock QRECV
                blocked[instruction.at("qpus")[0]] = false;

				// lock entanglement qubits, avoid other quantum tasks (except the receiving one) to use them
				lock_ent_qubits = true;
                break;
            }
            case constants::QRECV:
            {
                if (!qc_meas.contains(instruction.at("qpus")[0]))
                {
                    blocked[quantum_tasks[i].id] = true;
                    continue;
                }

                // Receive the measurements from the sender
                const std::size_t measSrc = qc_meas[instruction.at("qpus")[0]].top();
                qc_meas[instruction.at("qpus")[0]].pop();
                const std::size_t measEntSrc = qc_meas[instruction.at("qpus")[0]].top();
                qc_meas[instruction.at("qpus")[0]].pop();

                const unsigned long entTgt = n_qubits - 1;
                const unsigned long tgtQubit = qubits[0] + zero_qubit[i];

                // Apply, conditioned to the measurement, the X and Z gates
                if (measSrc)
                {
                    simulator.ApplyX(entTgt);
                }
                if (measEntSrc)
                {
                    simulator.ApplyZ(entTgt);
                }

                // Swap the value to the desired qubit
                simulator.ApplySwap(entTgt, tgtQubit);

                const unsigned long int q[]{ entTgt };
				simulator.ApplyReset(q, 1);

				// Unlock the entanglement qubits for other quantum tasks
				lock_ent_qubits = false;
                break;
            }
            default:
                std::cerr << "Instruction not suported!" << "\n";
            } // End switch  

            ++its[i];
            if (its[i] != ends[i])
                ended = false;
            else
                finished[i] = true;
        }

    } // End one shot

    // result is a map from the cbit index to the Boolean value
    for (const auto& [bitIndex, value] : classic_values)
    {
        resultString[n_clbits - bitIndex - 1] = value ? '1' : '0';
    }

    return resultString;

}


JSON MaestroSimulatorAdapter::simulate(const Backend* backend)
{
    try {
        auto quantum_task = qc.quantum_tasks[0];

        unsigned long n_qbits = quantum_task.config.at("num_qubits").get<unsigned long>();
 
        JSON circuit_json = quantum_task.circuit;
        JSON run_config_json(quantum_task.config);

        SimpleSimulator simulator;
        if (simulator.Init("/mnt/netapp1/Store_CESGA/home/cesga/acarballido/repos/api-simulator/maestro.so"))
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
                JSON result_json = JSON::parse(result);
                simulator.FreeResult(result);

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
    if (simulator.Init("maestro.so"))
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
                simulator.AllocateQubits(n_qubits);
                simulator.InitializeSimulator();
                meas_counter[execute_shot_(simulator, qc.quantum_tasks, classical_channel)]++;
                simulator.ClearSimulator();
            } // End all shots
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            float time_taken = duration.count();

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
