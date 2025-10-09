/**
 * @file SimpleDisconnectedNetwork.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * A simple network class, implementing a network containing a buch of hosts, no communication (classical or quantum) among them.
 *
 * Can execute circuits locally on each host.
 */

#pragma once

#ifndef _SIMPLE_NETWORK_H_
#define _SIMPLE_NETWORK_H_

#include "SimpleHost.h"
#include "SimpleController.h"
#include "QubitRegister.h"

#include "NetworkJob.h"
#include "../Estimators/SimulatorsEstimatorInterface.h"

namespace Network {

	/**
	 * @class SimpleDisconnectedNetwork
	 * @brief The simple network implementation.
	 *
	 * The simple network implementation.
	 * A simple network class, implementing a network containing a buch of hosts, no communication (classical or quantum) among them.
	 *
	 * @tparam Time The time type used for execution times.
	 * @sa INetwork
	 * @sa SimpleController
	 * @sa SimpleHost
	 */
	template<typename Time = Types::time_type, class Controller = SimpleController<Time>> class SimpleDisconnectedNetwork : public INetwork<Time> {
	public:
		using BaseClass = INetwork<Time>; /**< The base class type. */
		using ExecuteResults = typename BaseClass::ExecuteResults; /**< The execute results type. */

		/**
		 * @brief The constructor.
		 *
		 * Constructs the simple network object, creating the network hosts and the controller.
		 *
		 * @param qubits The number of qubits for each host.
		 * @param cbits The number of classical bits for each host.
		 */
		SimpleDisconnectedNetwork(const std::vector<Types::qubit_t>& qubits = {}, const std::vector<size_t>& cbits = {})
		{
			if (!qubits.empty()) CreateNetwork(qubits, cbits);
		}

		/**
		 * @brief Creates the network hosts and controller.
		 *
		 * Creates the network hosts and controller with the specified number of qubits and classical bits.
		 *
		 * @param qubits The number of qubits for each host.
		 * @param cbits The number of classical bits for each host.
		 */
		void CreateNetwork(const std::vector<Types::qubit_t>& qubits, const std::vector<size_t>& cbits)
		{
			size_t qubitsOffset = 0;
			size_t cbitsOffset = 0;

			for (size_t i = 0; i < qubits.size(); ++i)
			{
				const size_t numQubits = qubits[i];
				const size_t numBits = (i < cbits.size() ? cbits[i] : 0);
				hosts.emplace_back(std::make_shared<SimpleHost<Time>>(i, qubitsOffset, numQubits, cbitsOffset, numBits));
				qubitsOffset += numQubits;
				cbitsOffset += numBits;
			}

			for (size_t i = 0; i < hosts.size(); ++i)
			{
				std::static_pointer_cast<SimpleHost<Time>>(hosts[i])->SetEntangledQubitId(qubitsOffset);
				std::static_pointer_cast<SimpleHost<Time>>(hosts[i])->SetEntangledQubitMeasurementBit(cbitsOffset);
				++qubitsOffset;
				++cbitsOffset;
			}

			controller = std::make_shared<Controller>();
		}


		/**
		 * @brief Execute the circuit on the network.
		 *
		 * Execute the circuit on the network, using the controller for distributing the operations to the hosts.
		 * Ensure the quantum computing simulator has been created before calling this.
		 *
		 * @param circuit The circuit to execute.
		 * @sa Circuits::Circuit
		 */
		void Execute(const std::shared_ptr<Circuits::Circuit<Time>>& circuit) override
		{
			const auto recreate = recreateIfNeeded;

			auto simType = Simulators::SimulatorType::kQCSim;
			auto method = Simulators::SimulationType::kMatrixProductState;
			size_t numQubits = 2;
			if (simulator)
			{
				simType = simulator->GetType();
				method = simulator->GetSimulationType();
				numQubits = simulator->GetNumberOfQubits();
			}

			recreateIfNeeded = false;

			const auto res = RepeatedExecute(circuit, 1);

			recreateIfNeeded = recreate;

			// put the results in the state
			if (!res.empty())
			{
				const auto& first = *res.begin();
				GetState().SetResultsInOrder(first.first);
			}

			if (recreate && (!simulator || (simulator && (simType != simulator->GetType() || method != simulator->GetSimulationType() || simulator->GetNumberOfQubits() != numQubits))))
				CreateSimulator(simType, method);
		}



		/**
		 * @brief Execute the circuit on the specified host.
		 *
		 * Execute the circuit on the specified host.
		 * The circuit must fit on the host, otherwise an exception is thrown.
		 * The circuit will be mapped on the specified host, if its qubits start with indexing from 0 (if already mapped, the qubits won't be altered).
		 *
		 * @param circuit The circuit to execute.
		 * @param hostId The id of the host to execute the circuit on.
		 * @sa Circuits::Circuit
		 */
		void ExecuteOnHost(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId) override
		{
			const auto recreate = recreateIfNeeded;

			auto simType = Simulators::SimulatorType::kQCSim;
			auto method = Simulators::SimulationType::kMatrixProductState;
			size_t numQubits = 2;
			if (simulator)
			{
				simType = simulator->GetType();
				method = simulator->GetSimulationType();
				numQubits = simulator->GetNumberOfQubits();
			}

			recreateIfNeeded = false;

			const auto res = RepeatedExecuteOnHost(circuit, hostId, 1);

			recreateIfNeeded = recreate;

			// put the results in the state
			if (!res.empty())
			{
				const auto& first = *res.begin();
				GetState().SetResultsInOrder(first.first);
			}

			if (recreate && (!simulator || (simulator && (simType != simulator->GetType() || method != simulator->GetSimulationType() || simulator->GetNumberOfQubits() != numQubits))))
				CreateSimulator(simType, method);
		}


		/**
		 * @brief Execute the circuit on the network and return the expectation values for the specified Pauli strings.
		 *
		 * Execute the circuit on the network, using the controller for distributing the operations to the hosts and return the expectation values for the specified Pauli strings.
		 * The base class functionality is used for circuit distribution, but then the distributed circuit is converted to netqasm.
		 * Ensure the quantum computing simulator and the netqasm virtual machines have been created before calling this.
		 *
		 * @param circuit The circuit to execute.
		 * @param paulis The Pauli strings to measure the expectations for.
		 * @sa Circuits::Circuit
		 */
		std::vector<double> ExecuteExpectations(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, const std::vector<std::string>& paulis) override
		{
			const auto recreate = recreateIfNeeded;

			auto simType = Simulators::SimulatorType::kQCSim;
			auto method = Simulators::SimulationType::kMatrixProductState;
			size_t numQubits = 2;
			if (simulator)
			{
				simType = simulator->GetType();
				method = simulator->GetSimulationType();
				numQubits = simulator->GetNumberOfQubits();
			}

			recreateIfNeeded = false;

			pauliStrings = &paulis;
			const auto res = RepeatedExecute(circuit, 1);
			pauliStrings = nullptr;

			recreateIfNeeded = recreate;

			// put the results in the state
			if (!res.empty())
			{
				const auto& first = *res.begin();
				GetState().SetResultsInOrder(first.first);
			}

			std::vector<double> expectations(paulis.size());
			if (simulator)
			{
				// translate the pauli strings to the mapped order of qubits
				const size_t numOps = simulator->GetNumberOfQubits();

				auto optimiser = controller->GetOptimiser();
				if (optimiser)
				{
					// convert the classical state results back to the expected order
					const auto& qubitsMap = optimiser->GetQubitsMap();

					for (size_t i = 0; i < paulis.size(); ++i)
					{
						std::string translated(numOps, 'I');

						for (size_t j = 0; j < paulis[i].size(); ++j)
						{
							const auto pos = qubitsMap.find(j);
							if (pos != qubitsMap.end())
								translated[pos->second] = paulis[i][j];
							else
								translated[j] = paulis[i][j];
						}

						expectations[i] = simulator->ExpectationValue(translated);
					}
				}
				else
				{
					for (size_t i = 0; i < paulis.size(); ++i)
						expectations[i] = simulator->ExpectationValue(paulis[i]);
				}
			}

			if (recreate && (!simulator || simType != simulator->GetType() || method != simulator->GetSimulationType() || simulator->GetNumberOfQubits() != numQubits))
				CreateSimulator(simType, method);

			return expectations;
		}


		/**
		 * @brief Execute the circuit on the specified host and return the expectation values for the specified Pauli strings.
		 *
		 * Execute the circuit on the specified host and return the expectation values for the specified Pauli strings.
		 * The circuit must fit on the host, otherwise an exception is thrown.
		 * The circuit will be mapped on the specified host, if its qubits start with indexing from 0 (if already mapped, the qubits won't be altered).
		 *
		 * @param circuit The circuit to execute.
		 * @param hostId The id of the host to execute the circuit on.
		 * @param paulis The Pauli strings to measure the expectations for.
		 * @sa Circuits::Circuit
		 */
		std::vector<double> ExecuteOnHostExpectations(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId, const std::vector<std::string>& paulis)
		{
			const auto recreate = recreateIfNeeded;

			auto simType = Simulators::SimulatorType::kQCSim;
			auto method = Simulators::SimulationType::kMatrixProductState;
			size_t numQubits = 2;
			if (simulator)
			{
				simType = simulator->GetType();
				method = simulator->GetSimulationType();
				numQubits = simulator->GetNumberOfQubits();
			}

			recreateIfNeeded = false;
			pauliStrings = &paulis;
			const auto res = RepeatedExecuteOnHost(circuit, hostId, 1);
			pauliStrings = nullptr;

			recreateIfNeeded = recreate;

			// put the results in the state
			if (!res.empty())
			{
				const auto& first = *res.begin();
				GetState().SetResultsInOrder(first.first);
			}

			//for (const auto& m : qubitsMapOnHost)
			//	std::cout << "Mapping qubit " << m.first << " to " << m.second << std::endl;

			const size_t offsetBase = qubitsMapOnHost.size();

			std::vector<double> expectations(paulis.size(), 1.);
			if (simulator)
			{
				// translate the pauli strings to the mapped order of qubits
				const size_t numOps = simulator->GetNumberOfQubits();

				// convert the pauli strings to the actual qubits order
				for (size_t i = 0; i < paulis.size(); ++i)
				{
					std::string translated(std::max(numOps, paulis[i].size()), 'I');

					size_t offset = offsetBase;

					for (size_t j = 0; j < paulis[i].size(); ++j)
					{
						auto pos = qubitsMapOnHost.find(j);
						if (pos != qubitsMapOnHost.end())
							translated[pos->second] = paulis[i][j];
						else
						{
							translated[offset] = paulis[i][j];
							++offset;
						}
					}

					//std::cout << "Translated pauli string: " << translated << std::endl;

					expectations[i] = simulator->ExpectationValue(translated);
				}
			}

			if (recreate && (!simulator || simType != simulator->GetType() || method != simulator->GetSimulationType() || simulator->GetNumberOfQubits() != numQubits))
				CreateSimulator(simType, method);

			return expectations;
		}


		/**
		 * @brief Execute the circuit on the network, repeatedly.
		 *
		 * Execute the circuit on the network, distributing the operations to the hosts, repeating the execution 'shots' times.
		 * The way the circuit is distributed to the hosts depends on the specific interface implementations.
		 *
		 * @param circuit The circuit to execute.
		 * @param shots The number of times to repeat the execution.
		 * @return A map with the results of the execution, where the key is the qubit id and the value is the number of times the qubit was measured to be 1.
		 * @sa Circuits::Circuit
		 */
		ExecuteResults RepeatedExecute(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t shots = 1000) override
		{
			if (!controller || !circuit) return {};

			distCirc = controller->DistributeCircuit(BaseClass::getptr(), circuit);

#ifdef _DEBUG
			for (auto q : distCirc->AffectedQubits())
			{
				if (q >= GetNumQubits())
				{
					std::cout << "This is a distributed circuit, using entanglement or cutting" << std::endl;
					break;
				}
			}
#endif

			if (!simulator || distCirc->empty()) return {};

			auto simType = simulator->GetType();
			if (distCirc->HasOpsAfterMeasurements() && (
#ifndef NO_QISKIT_AER
				simType == Simulators::SimulatorType::kCompositeQiskitAer ||
#endif
				simType == Simulators::SimulatorType::kCompositeQCSim))
				distCirc->MoveMeasurementsAndResets();

			auto method = simulator->GetSimulationType();

			const auto saveSimType = simType;
			const auto saveMethod = method;

			if (GetOptimizeSimulator() && distCirc->IsClifford() && method != Simulators::SimulationType::kStabilizer
				// this is for the gpu simulator, as it doesn't support stabilizer
#ifdef __linux__
				&& simType != Simulators::SimulatorType::kGpuSim
#endif
				)
			{
				method = Simulators::SimulationType::kStabilizer;

				if (simType == Simulators::SimulatorType::kCompositeQCSim)
					simType = Simulators::SimulatorType::kQCSim;
#ifndef NO_QISKIT_AER					
				else if (simType == Simulators::SimulatorType::kCompositeQiskitAer)
					simType = Simulators::SimulatorType::kQiskitAer;
#endif
			}

			ExecuteResults res;
			const size_t nrQubits = GetNumQubits() + GetNumNetworkEntangledQubits();
			const size_t nrCbitsResults = GetNumClassicalBits();

			maxBondDim = simulator->GetConfiguration("matrix_product_state_max_bond_dimension");
			singularValueThreshold = simulator->GetConfiguration("matrix_product_state_truncation_threshold");
			mpsSample = simulator->GetConfiguration("mps_sample_measure_algorithm");

			// do that only if the optimization for simulator is on and the estimator is available, ortherwise an 'optimal' simulator won't be created
			if (optimizeSimulator && simulatorsEstimator)
			{
				simulator->Clear();
				GetState().Clear();
			}

			std::vector<bool> executed;
			auto optSim = ChooseBestSimulator(distCirc, shots, nrQubits, nrQubits, nrCbitsResults, simType, method, executed);

			lastSimulatorType = simType;
			lastMethod = method;

			size_t nrThreads = GetMaxSimulators();

#ifdef __linux__
			if (simType == Simulators::SimulatorType::kGpuSim)
				nrThreads = 1;
#endif
			if (method == Simulators::SimulationType::kStatevector && !distCirc->HasOpsAfterMeasurements())
				nrThreads = 1;


			nrThreads = std::min(nrThreads, std::max<size_t>(shots, 1ULL));

			std::mutex resultsMutex;

			const auto dcirc = distCirc;

			if (nrThreads > 1)
			{
				// since it's going to execute on multiple threads, free the memory from the network's simulator and state, it's going to use other ones, created in the threads
				// if optimization already exists, it will be cloned in the threads, otherwise a new one will be created in the threads
				if (!optimizeSimulator || !simulatorsEstimator) // otherwise it was already cleared
				{
					simulator->Clear();
					GetState().Clear();
				}

				const size_t cntPerThread = std::max<size_t>(shots / nrThreads, 1ULL);

				threadsPool.Resize(nrThreads);
				threadsPool.SetFinishLimit(shots);

				while (shots > 0)
				{
					const size_t curCnt = std::min(cntPerThread, shots);

					shots -= curCnt;

					auto job = std::make_shared<ExecuteJob<Time>>(dcirc, res, curCnt, nrQubits, nrQubits, nrCbitsResults, simType, method, resultsMutex);
					job->optimiseMultipleShotsExecution = GetOptimizeSimulator();

					if (optSim) {
						job->optSim = optSim->Clone();
						job->executedGates = executed;
					}
					else
					{
						job->maxBondDim = maxBondDim;
						job->mpsSample = mpsSample;
						job->singularValueThreshold = singularValueThreshold;
					}

					threadsPool.AddRunJob(std::move(job));
				}

				threadsPool.WaitForFinish();
				threadsPool.Stop();
			}
			else
			{
				const size_t curCnt = shots;

				auto job = std::make_shared<ExecuteJob<Time>>(dcirc, res, curCnt, nrQubits, nrQubits, nrCbitsResults, simType, method, resultsMutex);
				job->optimiseMultipleShotsExecution = GetOptimizeSimulator();

				if (optSim) {
					optSim->SetMultithreading(true);
					job->optSim = optSim;
					job->executedGates = executed;
				}
				else
				{
					if (simulator && method == saveMethod && simType == saveSimType)
					{
						optSim = simulator;
						job->optSim = optSim;
						simulator = nullptr;
					}
					job->maxBondDim = maxBondDim;
					job->mpsSample = mpsSample;
					job->singularValueThreshold = singularValueThreshold;
				}

				job->DoWorkNoLock();
				if (!recreateIfNeeded)
					simulator = job->optSim;
			}

			if (recreateIfNeeded)
				CreateSimulator(saveSimType, saveMethod);

			ConvertBackResults(res);

			return res;
		}



		/**
		 * @brief Execute the circuit on the specified host, repeatedly.
		 *
		 * Execute the circuit on the specified host, repeating the execution 'shots' times.
		 * The circuit must fit on the host, otherwise an exception is thrown.
		 * The circuit will be mapped on the specified host, if its qubits start with indexing from 0 (if already mapped, the qubits won't be altered).
		 *
		 * @param circuit The circuit to execute.
		 * @param hostId The id of the host to execute the circuit on.
		 * @param shots The number of times to repeat the execution.
		 * @return A map with the results of the execution, where the key is the qubit id and the value is the number of times the qubit was measured to be 1.
		 * @sa Circuits::Circuit
		 */
		ExecuteResults RepeatedExecuteOnHost(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId, size_t shots = 1000) override
		{
			if (!circuit || hostId >= GetNumHosts()) return {};

			size_t nrQubits = 0;
			size_t nrCbits = 0;

			std::shared_ptr<Circuits::Circuit<Time>> optCircuit;
			if (GetController()->GetOptimizeCircuit()) {
				optCircuit = std::static_pointer_cast<Circuits::Circuit<Time>>(circuit->Clone());
				optCircuit->Optimize();
			}
			const auto reverseQubitsMap = MapCircuitOnHost(GetController()->GetOptimizeCircuit() ? optCircuit : circuit, hostId, nrQubits, nrCbits, true);
			if (nrCbits == 0) nrCbits = nrQubits;

			if (!simulator || distCirc->empty()) return {};

			auto simType = simulator->GetType();

			maxBondDim = simulator->GetConfiguration("matrix_product_state_max_bond_dimension");
			singularValueThreshold = simulator->GetConfiguration("matrix_product_state_truncation_threshold");
			mpsSample = simulator->GetConfiguration("mps_sample_measure_algorithm");

			if (distCirc->HasOpsAfterMeasurements() && (
#ifndef NO_QISKIT_AER
				simType == Simulators::SimulatorType::kCompositeQiskitAer ||
#endif
				simType == Simulators::SimulatorType::kCompositeQCSim))
				distCirc->MoveMeasurementsAndResets();

			auto method = simulator->GetSimulationType();
			const auto saveSimType = simType;
			const auto saveMethod = method;

			if (GetOptimizeSimulator() && distCirc->IsClifford() && method != Simulators::SimulationType::kStabilizer
				// this is for the gpu simulator, as it doesn't support stabilizer
#ifdef __linux__
				&& simType != Simulators::SimulatorType::kGpuSim
#endif				
				)
			{
				method = Simulators::SimulationType::kStabilizer;

				if (simType == Simulators::SimulatorType::kCompositeQCSim)
					simType = Simulators::SimulatorType::kQCSim;
#ifndef NO_QISKIT_AER					
				else if (simType == Simulators::SimulatorType::kCompositeQiskitAer)
					simType = Simulators::SimulatorType::kQiskitAer;
#endif
			}

			ExecuteResults res;

			// since it's going to execute on multiple threads, free the memory from the network's simulator and state, it's going to use other ones, created in the threads
			simulator->Clear();
			GetState().Clear();

			std::vector<bool> executed;
			auto optSim = ChooseBestSimulator(distCirc, shots, nrQubits, nrCbits, nrCbits, simType, method, executed);

			lastSimulatorType = simType;
			lastMethod = method;

			size_t nrThreads = GetMaxSimulators();

#ifdef __linux__
			if (simType == Simulators::SimulatorType::kGpuSim)
				nrThreads = 1;
			else
#endif
				if (method == Simulators::SimulationType::kStatevector && !distCirc->HasOpsAfterMeasurements())
					nrThreads = 1;

			nrThreads = std::min(nrThreads, std::max<size_t>(shots, 1ULL));

			// WARNING: be sure to not put this above ChooseBestSimulator, as that one can change the shots value!

			std::mutex resultsMutex;

			const auto dcirc = distCirc;

			if (nrThreads > 1)
			{
				// this rounds up, rounding down is better
				//const size_t cntPerThread = static_cast<size_t>((shots - 1) / nrThreads + 1);
				const size_t cntPerThread = std::max<size_t>(shots / nrThreads, 1ULL);

				threadsPool.Resize(nrThreads);
				threadsPool.SetFinishLimit(shots);

				while (shots > 0)
				{
					const size_t curCnt = std::min(cntPerThread, shots);
					shots -= curCnt;

					auto job = std::make_shared<ExecuteJob<Time>>(dcirc, res, curCnt, nrQubits, nrCbits, nrCbits, simType, method, resultsMutex);
					job->optimiseMultipleShotsExecution = GetOptimizeSimulator();

					if (optSim) {
						job->optSim = optSim->Clone();
						job->executedGates = executed;
					}
					else {
						job->maxBondDim = maxBondDim;
						job->mpsSample = mpsSample;
						job->singularValueThreshold = singularValueThreshold;
					}

					threadsPool.AddRunJob(std::move(job));
				}

				threadsPool.WaitForFinish();
				threadsPool.Stop();
			}
			else
			{
				const size_t curCnt = shots;

				auto job = std::make_shared<ExecuteJob<Time>>(dcirc, res, curCnt, nrQubits, nrCbits, nrCbits, simType, method, resultsMutex);
				job->optimiseMultipleShotsExecution = GetOptimizeSimulator();

				if (optSim) {
					optSim->SetMultithreading(true);
					job->optSim = optSim;
					job->executedGates = executed;
				}
				else {
					job->maxBondDim = maxBondDim;
					job->mpsSample = mpsSample;
					job->singularValueThreshold = singularValueThreshold;
				}

				job->DoWorkNoLock();
				if (!recreateIfNeeded)
					simulator = job->optSim;
			}

			if (recreateIfNeeded)
				CreateSimulator(saveSimType, saveMethod);

			if (!reverseQubitsMap.empty()) ConvertBackResults(res, reverseQubitsMap);

			return res;
		}

		/**
		 * @brief Get the number of gates that span more than one host.
		 *
		 * Get the number of gates that span more than one host for the given circuit.
		 *
		 * @param circuit The circuit to check.
		 * @return The number of gates that need distribution or cutting.
		 */
		size_t GetNumberOfGatesDistributedOrCut(const std::shared_ptr<Circuits::Circuit<Time>>& circuit) const override
		{
			if (!circuit) return 0;

			size_t distgates = 0;

			for (const auto& op : circuit->GetOperations())
				if (!IsLocalOperation(op)) ++distgates;

			return distgates;
		}

		/**
		 * @brief Schedule and execute circuits on the network.
		 *
		 * Execute the circuits on the network, scheduling their execution and distributing the operations to the hosts.
		 * The way the circuits are distributed to the hosts depends on the specific interface implementations.
		 * The way they are scheduled depends on the network scheduler and parametrization.
		 *
		 * @param circuits The circuits to execute, along with the number of shots.
		 * @return A vector of maps with the results of each circuit execution, where the key is the state as a vector of bools and the value is the number of times it was measured.
		 * @sa Circuits::Circuit
		 * @sa ExecuteCircuit
		 */
		std::vector<ExecuteResults> ExecuteScheduled(const std::vector<Schedulers::ExecuteCircuit<Time>>& circuits) override
		{
			// create a default one if not set
			if (!GetScheduler())
			{
				CreateScheduler();

				if (!GetScheduler()) return {};
			}

			return GetScheduler()->ExecuteScheduled(circuits);
		}


		/**
		 * @brief Create the simulator for the network.
		 *
		 * Creates the simulator for the network.
		 * Call this only after the network topology has been set up.
		 * Should create the simulator with the proper number of qubits for the whole network and also set up a 'classical state' for the whole network
		 * and distribute the qubits and cbits to the hosts.
		 * The nrQubits parameter is used internally to allocate a simulator for a single host only - if a circuit is executed on a single host.
		 * Let it to the default value - 0 - to allocate the number of qubits for the whole network.
		 *
		 * @param simType The type of the simulator to create.
		 * @param simExecType The type of the simulation - statevector, composite, matrix product state, stabilizer, tensor network...
		 * @param nrQubits The number of qubits to allocate for the simulator. Default is 0 - allocate the number of qubits for the whole network.
		 * @sa Simulators::SimulatorType
		 */
		void CreateSimulator(Simulators::SimulatorType simType =
			Simulators::SimulatorType::kQCSim
			, Simulators::SimulationType simExecType = Simulators::SimulationType::kMatrixProductState, size_t nrQubits = 0
		) override
		{
			classicalState.Clear();
			classicalState.AllocateBits(GetNumClassicalBits() + GetNumNetworkEntangledQubits());

			simulator = Simulators::SimulatorsFactory::CreateSimulator(simType, simExecType);

			if (simulator)
			{
				if (!maxBondDim.empty()) simulator->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
				if (!singularValueThreshold.empty()) simulator->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
				if (!mpsSample.empty()) simulator->Configure("mps_sample_measure_algorithm", mpsSample.c_str());

				simulator->AllocateQubits(nrQubits == 0 ? GetNumQubits() + GetNumNetworkEntangledQubits() : nrQubits);
				simulator->Initialize();
			}
		}

		/**
		 * @brief Configures the network.
		 *
		 * This function is called to configure the network (for example the simulator(s) used by the network.
		 *
		 * @param key The key of the configuration option.
		 * @param value The value of the configuration.
		 */
		void Configure(const char* key, const char* value) override
		{
			if (!key || !value) return;

			if (std::string("matrix_product_state_max_bond_dimension") == key)
				maxBondDim = value;
			else if (std::string("matrix_product_state_truncation_threshold") == key)
				singularValueThreshold = value;
			else if (std::string("mps_sample_measure_algorithm") == key)
				mpsSample = value;
			else if (std::string("max_simulators") == key)
				maxSimulators = std::stoull(value);

			if (simulator)
				simulator->Configure(key, value);
		}

		/**
		 * @brief Get the simulator for the network.
		 *
		 * Get the simulator for the network.
		 *
		 * @return The simulator for the network.
		 * @sa Simulators::ISimulator
		 */
		std::shared_ptr<Simulators::ISimulator> GetSimulator() const override
		{
			return simulator;
		}


		/**
		 * @brief Get the classical state of the network.
		 *
		 * Gets a reference to the classical state of the network.
		 *
		 * @return The classical state of the network.
		 * @sa Circuits::ClassicalState
		 */
		Circuits::OperationState& GetState() override
		{
			return classicalState;
		}

		/**
		 * @brief Create the scheduler for the network.
		 *
		 * Creates the scheduler for the network.
		 * Call this only after the network topology has been set up.
		 * Should create the scheduler and set the network for it and any other necessary parameters.
		 *
		 * @param simType The type of the scheduler to create.
		 * @sa SchedulerType
		 */
		void CreateScheduler(SchedulerType schType = SchedulerType::kNoEntanglementQubitsParallel) override
		{
			if (!controller) return;

			controller->CreateScheduler(BaseClass::getptr(), schType);
		}

		/**
		 * @brief Get the scheduler for the network.
		 *
		 * Get the scheduler for the network.
		 *
		 * @return The scheduler for the network.
		 * @sa Schedulers::IScheduler
		 */
		std::shared_ptr<Schedulers::IScheduler<Time>> GetScheduler() const override
		{
			if (!controller) return nullptr;

			return controller->GetScheduler();
		}

		/**
		 * @brief Get the host with the specified id.
		 *
		 * Get a smart pointer to the host that has the specified id.
		 *
		 * @param hostId The id of the host to get.
		 * @return A smart pointer to the host that has the specified id.
		 * @sa IHost
		 */
		const std::shared_ptr<IHost<Time>> GetHost(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return nullptr;

			return hosts[hostId];
		}

		/**
		 * @brief Get the controller for the network.
		 *
		 * Gets a smart pointer to the controller for the network.
		 *
		 * @return The controller for the network.
		 * @sa IController
		 */
		const std::shared_ptr<IController<Time>> GetController() const override
		{
			return controller;
		}

		/**
		 * @brief Get the number of hosts in the network.
		 *
		 * Get the number of hosts in the network, excluding the controller.
		 *
		 * @return The number of hosts in the network.
		 */
		size_t GetNumHosts() const override
		{
			return hosts.size();
		}

		/**
		 * @brief Get the number of qubits in the network.
		 *
		 * Get the number of qubits in the network, excluding the qubits used for entanglement between the hosts.
		 *
		 * @return The number of qubits in the network.
		 */
		size_t GetNumQubits() const override
		{
			size_t res = 0;

			for (const auto& host : hosts)
				res += host->GetNumQubits();

			return res;
		}

		/**
		 * @brief Get the number of qubits in the network for the specified host.
		 *
		 * Get the number of qubits in the network for the specified host, excluding the qubits used for entanglement between the hosts.
		 *
		 * @param hostId The id of the host to get the number of qubits for.
		 * @return The number of qubits in the network for the specified host.
		 */
		size_t GetNumQubitsForHost(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return 0;

			return hosts[hostId]->GetNumQubits();
		}

		/**
		 * @brief Get the number of qubits used for entanglement between hosts.
		 *
		 * Get the number of qubits used for entanglement between hosts in the network.
		 * For the simple network it's one qubit per host.
		 *
		 * @return The number of qubits used for entanglement between hosts.
		 */
		size_t GetNumNetworkEntangledQubits() const override
		{
			size_t res = 0;

			for (const auto& host : hosts)
				res += host->GetNumNetworkEntangledQubits();

			return res;
		}

		/**
		 * @brief Get the number of qubits used for entanglement between hosts for the specified host.
		 *
		 * Get the number of qubits used for entanglement between hosts in the network for the specified host.
		 * For the simple network it's a single qubit.
		 *
		 * @param hostId The id of the host to get the number of qubits used for entanglement between hosts for.
		 * @return The number of qubits used for entanglement between hosts for the specified host.
		 */
		size_t GetNumNetworkEntangledQubitsForHost(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return 0;

			return hosts[hostId]->GetNumNetworkEntangledQubits();
		}

		/**
		 * @brief Get the number of classical bits in the network.
		 *
		 * Get the number of classical bits in the network, excluding the classical bits used for measurement of entanglement qubits between the hosts.
		 *
		 * @return The number of classical bits in the network.
		 */
		size_t GetNumClassicalBits() const override
		{
			size_t res = 0;

			for (const auto& host : hosts)
				res += host->GetNumClassicalBits();

			return res;
		}

		/**
		 * @brief Get the number of classical bits in the network for the specified host.
		 *
		 * Get the number of classical bits in the network for the specified host, excluding the classical bits used for measurement of entanglement qubits between the hosts.
		 *
		 * @param hostId The id of the host to get the number of classical bits for.
		 * @return The number of classical bits in the network for the specified host.
		 */
		size_t GetNumClassicalBitsForHost(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return 0;

			return hosts[hostId]->GetNumClassicalBits();
		}

		/**
		 * @brief Get the hosts in the network.
		 *
		 * Gets the hosts in the network.
		 *
		 * @return A vector with the hosts in the network.
		 * @sa IHost
		 * @sa SimpleHost
		 */
		std::vector<std::shared_ptr<IHost<Time>>>& GetHosts()
		{
			return hosts;
		}

		/**
		 * @brief Set the network controller host.
		 *
		 * Sets the network controller host.
		 * @param[in] cntrl The controller host.
		 * @sa IController
		 */
		void SetController(const std::shared_ptr<IController<Time>>& cntrl)
		{
			controller = cntrl;
		}


		/**
		 * @brief Sends a packet between two hosts.
		 *
		 * Sends a packet between the two specified hosts.
		 * It's not used in the simple network, because sending/receiving the classical information is done using a shared classical state for the simulator.
		 *
		 * @param fromHostId The id of the host to send the packet from.
		 * @param toHostId The id of the host to send the packet to.
		 * @param packet The packet to send.
		 * @return False.
		 */
		bool SendPacket(size_t fromHostId, size_t toHostId, const std::vector<uint8_t>& packet) override
		{
			return false;
		}

		/**
		 * @brief Get the type of the network.
		 *
		 * Get the type of the network.
		 * @return The type of the network.
		 * @sa NetworkType
		 */
		NetworkType GetType() const override
		{
			return NetworkType::kSimpleDisconnectedNetwork;
		}

		/**
		 * @brief Check if the circuit operation is local.
		 *
		 * Check if the specified circuit operation is local. A local operation is an operation that is executed on a single host.
		 * This does not include the operations that also operate on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is local, false otherwise.
		 */
		bool IsLocalOperation(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			const auto qubits = op->AffectedQubits();

			if (qubits.empty()) return true;

			size_t firstQubit = qubits[0];

			for (size_t q = 1; q < qubits.size(); ++q)
				if (!AreQubitsOnSameHost(firstQubit, qubits[q])) return false;

			return true;
		}

		/**
		 * @brief Check if the circuit operation is distributed.
		 *
		 * Check if the specified circuit operation is distributed. A distributed operation is an operation that is executed on multiple hosts.
		 * This does not include the operations that also operate on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is distributed, false otherwise.
		 */
		bool IsDistributedOperation(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			const auto qubits = op->AffectedQubits();

			if (qubits.empty()) return false;

			// grab the first qubit that is on a host (skip over network entangled qubits)
			size_t firstQubit = qubits[0];
			size_t q = 1;
			for (; IsNetworkEntangledQubit(firstQubit) && q < qubits.size(); ++q)
				firstQubit = qubits[q];

			// check to see one of the other qubits is on a different host, but ignore the network entangled qubits
			for (; q < qubits.size(); ++q)
				if (!IsNetworkEntangledQubit(qubits[q]) && !AreQubitsOnSameHost(firstQubit, qubits[q])) return true;

			return false;
		}

		/**
		 * @brief Check if the circuit operation operates on the entanglement qubits between hosts.
		 *
		 * Check if the specified circuit operation operates on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation operates on the entanglement qubits between hosts, false otherwise.
		 */
		bool OperatesWithNetworkEntangledQubit(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			const auto qubits = op->AffectedQubits();

			if (qubits.empty()) return false;

			for (size_t q = 0; q < qubits.size(); ++q)
				if (IsNetworkEntangledQubit(qubits[q])) return true;

			return false;
		}

		/**
		 * @brief Checks if a gate is an entangling gate.
		 *
		 * An entangling gate is a gate that operates on two qubits that are used for entanglement between hosts.
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is an entangling gate, false otherwise.
		 */
		bool IsEntanglingGate(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			if (op->GetType() != Circuits::OperationType::kGate) return false;
			const auto qubits = op->AffectedQubits();
			if (qubits.size() != 2) return false;

			return IsNetworkEntangledQubit(qubits[0]) && IsNetworkEntangledQubit(qubits[1]);
		}

		/**
		 * @brief Checks if a gate expects a classical bit from another host.
		 *
		 * It must be a conditional gate, conditioned on a classical bit from another host.
		 * Use it on already distributed gates, not on the original circuit.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation needs a classical bit from another host, false otherwise.
		 */
		bool ExpectsClassicalBitFromOtherHost(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			if (!op->IsConditional()) return false;

			const auto qubits = op->AffectedQubits();

			const std::shared_ptr<Circuits::IConditionalOperation<Time>> condOp = std::static_pointer_cast<Circuits::IConditionalOperation<Time>>(op);
			const auto& classicalBits = condOp->GetCondition()->GetBitsIndices();

			if (qubits.empty() && classicalBits.empty()) throw std::runtime_error("No classical bits specified!"); // this would be odd!

			// consider it on the host where it has the first qubit (or bit, if there are no qubits)

			const size_t hostId = GetHostIdForAnyQubit(qubits[0]);

			// now check the classical bits
			for (const auto bit : classicalBits)
				if (hostId != GetHostIdForClassicalBit(bit)) return true;

			return false;
		}

		/**
		 * @brief Get the host id where the classical control bit resides for a conditioned gate.
		 *
		 * It must be a conditional gate, conditioned on a qubit from another host.
		 * Use it on already distributed gates, not on the original circuit.
		 *
		 * @param op The operation to find the host of the control bit.
		 * @return The host id.
		 */
		size_t GetHostIdForClassicalControl(const std::shared_ptr<Circuits::IOperation<Time>>& op) const override
		{
			if (!op->IsConditional()) throw std::runtime_error("Operation is not conditional!");

			std::shared_ptr<Circuits::IConditionalOperation<Time>> condOp = std::static_pointer_cast<Circuits::IConditionalOperation<Time>>(op);
			const auto classicalBits = condOp->AffectedBits();

			if (classicalBits.empty()) throw std::runtime_error("No classical bits specified!");

			return GetHostIdForClassicalBit(classicalBits[0]);
		}

		/**
		 * @brief Check if the specified qubits are on the same host.
		 *
		 * Check if the specified qubits are on the same host. This does not include the qubits used for entanglement between the hosts.
		 *
		 * @return True if the specified qubits are on the same host, false otherwise.
		 */
		bool AreQubitsOnSameHost(size_t qubitId1, size_t qubitId2) const override
		{
			for (const auto& host : hosts)
			{
				const bool present1 = host->IsQubitOnHost(qubitId1);
				const bool present2 = host->IsQubitOnHost(qubitId2);

				if (present1 && present2) return true;
				else if (present1 || present2) return false;
			}

			return false;
		}

		/**
		 * @brief Check if the specified classical bits are on the same host.
		 *
		 * Check if the specified classical are on the same host. This does not include the classical bits used for measuring the qubits used for entanglement between the hosts.
		 *
		 * @return True if the specified qubits are on the same host, false otherwise.
		 */
		bool AreClassicalBitsOnSameHost(size_t bitId1, size_t bitId2) const override
		{
			for (const auto& host : hosts)
			{
				const bool present1 = host->IsClassicalBitOnHost(bitId1);
				const bool present2 = host->IsClassicalBitOnHost(bitId2);

				if (present1 && present2) return true;
				else if (present1 || present2) return false;
			}

			return false;
		}

		/**
		 * @brief Check if the specified qubit and classical bit are on the same host.
		 *
		 * Check if the specified qubit and classical bit are on the same host. This does not include the qubits used for entanglement between the hosts and the classical bits used for their measurement.
		 *
		 * @return True if the specified qubit and classical bit are on the same host, false otherwise.
		 */
		bool AreQubitAndClassicalBitOnSameHost(size_t qubitId, size_t bitId) const override
		{
			for (const auto& host : hosts)
			{
				const bool present1 = host->IsQubitOnHost(qubitId);
				const bool present2 = host->IsClassicalBitOnHost(bitId);

				if (present1 && present2) return true;
				else if (present1 || present2) return false;
			}

			return false;
		}

		/**
		 * @brief Get the host id for the specified qubit.
		 *
		 * Get the host id for the specified qubit, excluding the qubits used for entanglement between hosts.
		 *
		 * @param qubitId The id of the qubit to get the host id for.
		 * @return The host id for the specified qubit.
		 */
		size_t GetHostIdForQubit(size_t qubitId) const override
		{
			for (const auto& host : hosts)
				if (host->IsQubitOnHost(qubitId)) return host->GetId();

			return std::numeric_limits<size_t>::max();
		}

		/**
		 * @brief Get the host id for the specified qubit used for entanglement between hosts.
		 *
		 * Get the host id for the specified qubit used for entanglement between hosts.
		 *
		 * @param qubitId The id of the qubit used for entanglement between hosts to get the host id for.
		 * @return The host id for the specified qubit used for entanglement between hosts.
		 */
		size_t GetHostIdForEntangledQubit(size_t qubitId) const override
		{
			for (const auto& host : hosts)
				if (host->IsEntangledQubitOnHost(qubitId)) return host->GetId();

			return std::numeric_limits<size_t>::max();
		}

		/**
		 * @brief Get the host id for the specified qubit.
		 *
		 * Get the host id for the squbit, including the qubits used for entanglement between hosts.
		 *
		 * @param qubitId The id of the qubit to get the host id for.
		 * @return The host id for the specified qubit.
		 */
		size_t GetHostIdForAnyQubit(size_t qubitId) const override
		{
			if (IsNetworkEntangledQubit(qubitId))
				return GetHostIdForEntangledQubit(qubitId);

			return GetHostIdForQubit(qubitId);
		}

		/**
		 * @brief Get the host id for the specified classical bit.
		 *
		 * Get the host id for the specified classical bit, excluding the one(s) used for measurement of entanglement qubits between the hosts.
		 *
		 * @param classicalBitId The id of the classical bit to get the host id for.
		 * @return The host id for the specified classical bit.
		 */
		size_t GetHostIdForClassicalBit(size_t classicalBitId) const override
		{
			for (const auto& host : hosts)
				if (host->IsClassicalBitOnHost(classicalBitId)) return host->GetId();

			return std::numeric_limits<size_t>::max();
		}

		/**
		 * @brief Get the qubit ids for the specified host.
		 *
		 * Get the qubit ids for the specified host.
		 *
		 * @param hostId The id of the host to get the qubit ids for.
		 * @return A vector with the qubit ids.
		 */
		std::vector<size_t> GetQubitsIds(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return std::vector<size_t>();

			return hosts[hostId]->GetQubitsIds();
		}

		/**
		 * @brief Get the qubit ids used for entanglement between hosts for the specified host.
		 *
		 * Get the qubit ids used for entanglement between hosts  for the specified host.
		 *
		 * @param hostId The id of the host to get the qubit ids for.
		 * @return A vector with the qubit ids.
		 */
		std::vector<size_t> GetNetworkEntangledQubitsIds(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return std::vector<size_t>();

			return hosts[hostId]->GetNetworkEntangledQubitsIds();
		}

		/**
		 * @brief Get the classical bit ids for the specified host.
		 *
		 * Get the classical bit ids for the specified host, excluding the one(s) used for measurement of entanglemen qubits between the hosts.
		 *
		 * @param hostId The id of the host to get the classical bit ids for.
		 * @return A vector with the classical bit ids.
		 */
		std::vector<size_t> GetClassicalBitsIds(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return std::vector<size_t>();

			return hosts[hostId]->GetClassicalBitsIds();
		}

		/**
		 * @brief Get the classical bit ids used for measurement of entanglement qubits between the hosts for the specified host.
		 *
		 * Get the classical bit ids used for measurement of entanglement qubits between the hosts for the specified host.
		 *
		 * @param hostId The id of the host to get the classical bit ids for.
		 * @return A vector with the classical bit ids.
		 */
		std::vector<size_t> GetEntangledQubitMeasurementBitIds(size_t hostId) const override
		{
			if (hostId >= hosts.size())
				return std::vector<size_t>();

			return hosts[hostId]->GetEntangledQubitMeasurementBitIds();
		}

		/**
		 * @brief Check if the specified qubit id is for a qubit used for entanglement between hosts.
		 *
		 * Check if the specified qubit id is for a qubit used for entanglement between hosts.
		 *
		 * @param qubitId The id of the qubit to check.
		 * @return True if the specified qubit id is for a qubit used for entanglement between hosts, false otherwise.
		 */
		bool IsNetworkEntangledQubit(size_t qubitId) const override
		{
			return qubitId >= GetNumQubits();
		}


		/**
		 * @brief Check if the specified qubit used for entanglement between hosts is busy.
		 *
		 * Check if the specified qubit used for entanglement between hosts is busy.
		 * It's not used in the simple network, because it doesn't allow quantum communication between hosts.
		 *
		 * @param qubitId The id of the qubit to check.
		 * @return True if the specified qubit used for entanglement between hosts is busy, false otherwise.
		 */
		bool IsEntanglementQubitBusy(size_t qubitId) const override
		{
			return false;
		}

		/**
		 * @brief Check if any of the two specified qubits used for entanglement between hosts are busy.
		 *
		 * Check if any of the two specified qubits used for entanglement between hosts are busy.
		 * This is used to check if the qubits are free in order to use them for creating an entanglement between hosts.
		 * It's not used in the simple network, because it doesn't allow quantum communication between hosts.
		 *
		 * @param qubitId1 The id of the first qubit to check.
		 * @param qubitId2 The id of the second qubit to check.
		 * @return True if any of the two specified qubits used for entanglement between hosts are busy, false otherwise.
		 */
		bool AreEntanglementQubitsBusy(size_t qubitId1, size_t qubitId2) const override
		{
			return false;
		}

		/**
		 * @brief Mark the pair of the specified qubits used for entanglement between hosts as busy.
		 *
		 * Mark the pair of the specified qubits used for entanglement between hosts as busy.
		 * This is used to mark the qubits as busy when they are used for creating an entanglement between hosts.
		 * It's not used in the simple network, because it doesn't allow quantum communication between hosts.
		 *
		 * @param qubitId1 The id of the first qubit to mark as busy.
		 * @param qubitId2 The id of the second qubit to mark as busy.
		 */
		void MarkEntangledQubitsBusy(size_t qubitId1, size_t qubitId2) override
		{
			throw std::runtime_error("Entanglement between hosts is not supported in the simple network");
		}

		/**
		 * @brief Mark the specified qubit used for entanglement between hosts as free.
		 *
		 * Mark the specified qubit used for entanglement between hosts as free.
		 * This is used to mark the qubits as free when they are not used anymore for an entanglement between hosts.
		 * It's not used in the simple network, because it doesn't allow quantum communication between hosts.
		 *
		 * @param qubitId The id of the qubit to mark as free.
		 */
		void MarkEntangledQubitFree(size_t qubitId) override
		{
			throw std::runtime_error("Entanglement between hosts is not supported in the simple network");
		}

		/**
		 * @brief Clear all entanglements between hosts in the network.
		 *
		 * Clear all entanglements between hosts in the network.
		 * This marks all qubits used for entanglement between hosts as free.
		 * If the entanglements are explicitely coordinated in the network, all pairs of entangled qubits are released.
		 * It's not used in the simple network, because it doesn't allow quantum communication between hosts.
		 */
		void ClearEntanglements() override
		{
			throw std::runtime_error("Entanglement between hosts is not supported in the simple network");
		}

		/**
		 * @brief Get the distributed circuit.
		 *
		 * Get the distributed circuit.
		 * Execute() must be called first, otherwise the return would be nullptr.
		 *
		 * @return The distributed circuit.
		 * @sa Circuits::Circuit
		 */
		std::shared_ptr<Circuits::Circuit<Time>> GetDistributedCircuit() const override
		{
			return distCirc;
		}

		/**
		 * @brief Get the last used simulator type.
		 *
		 * Get the last used simulator type.
		 *
		 * @return The simulator type that was used last time.
		 */
		Simulators::SimulatorType GetLastSimulatorType() const override
		{
			return lastSimulatorType;
		}

		/**
		 * @brief Get the last used simulation type.
		 *
		 * Get the last used simulation type.
		 *
		 * @return The simulation type that was used last time.
		 */
		Simulators::SimulationType GetLastSimulationType() const override
		{
			return lastMethod;
		}

		/**
		 * @brief Get the maximum number of simulators that can be used in the network.
		 *
		 * Get the maximum number of simulators that can be used in the network.
		 * This is used to limit the number of simulators (and corresponding threads) that can be used in the network.
		 *
		 * @return The maximum number of simulators that can be used in the network.
		 */
		size_t GetMaxSimulators() const override
		{
			return maxSimulators;
		}

		/**
		 * @brief Set the maximum number of simulators that can be used in the network.
		 *
		 * Set the maximum number of simulators that can be used in the network.
		 * This is used to limit the number of simulators (and corresponding threads) that can be used in the network.
		 *
		 * @param maxSimulators The maximum number of simulators that can be used in the network.
		 */
		void SetMaxSimulators(size_t val) override
		{
			if (val < 1) val = 1;
			else if (val > (size_t)QC::QubitRegisterCalculator<>::GetNumberOfThreads()) val = (size_t)QC::QubitRegisterCalculator<>::GetNumberOfThreads();

			maxSimulators = val;
		}



		/**
		 * @brief Allows using an optimized simulator.
		 *
		 * If set, allows changing the simulator with an optimized one.
		 * States/amplitudes are not available in such a case, disable if you need them.
		 *
		 * @param optimize If true, the simulator will be optimized if possible.
		 */
		void SetOptimizeSimulator(bool optimize = true) override
		{
			optimizeSimulator = optimize;
		}


		/**
		 * @brief Returns the 'optimize' flag.
		 *
		 * Returns the flag set by SetOptimizeSimulator().
		 *
		 * @return The 'optimize' flag.
		 */
		bool GetOptimizeSimulator() const override
		{
			return optimizeSimulator;
		}


		/**
		 * @brief Get the optimizations simulators set.
		 *
		 * Get the optimization simulators set.
		 * To be used internally, will not be exposed from the library.
		 *
		 * @return The simulators set.
		 */
		const typename BaseClass::SimulatorsSet& GetSimulatorsSet() const override
		{
			return simulatorsForOptimizations;
		}

		/**
		 * @brief Adds a simulator to the simulators optimization set.
		 *
		 * Adds a simulator (if not already present) to the simulators optimization set.
		 *
		 * @param type The type of the simulator to add.
		 * @param kind The kind of the simulation to add.
		 */
		void AddOptimizationSimulator(Simulators::SimulatorType type, Simulators::SimulationType kind) override
		{
			simulatorsForOptimizations.insert({ type, kind });
		}

		/**
		 * @brief Removes a simulator from the simulators optimization set.
		 *
		 * Removes a simulator from the simulators optimization set, if it exists.
		 *
		 * @param type The type of the simulator to remove.
		 * @param kind The kind of the simulation to remove.
		 */
		void RemoveOptimizationSimulator(Simulators::SimulatorType type, Simulators::SimulationType kind) override
		{
			simulatorsForOptimizations.erase({ type, kind });
		}

		/**
		 * @brief Removes all simulators from the simulators optimization set and adds the one specified.
		 *
		 * Removes all simulators from the simulators optimization set and adds the one specified.
		 *
		 * @param type The type of the simulator to add.
		 * @param kind The kind of the simulation to add.
		 */
		void RemoveAllOptimizationSimulatorsAndAdd(Simulators::SimulatorType type, Simulators::SimulationType kind) override
		{
			simulatorsForOptimizations.clear();
			simulatorsForOptimizations.insert({ type, kind });
		}

		/**
		 * @brief Checks if a simulator exists in the optimization set.
		 *
		 * Checks if a simulator exists in the optimization set.
		 *
		 * @param type The type of the simulator to check.
		 * @param kind The kind of the simulation to check.
		 * @return True if the simulator exists in the optimization set, false otherwise.
		 */
		bool OptimizationSimulatorExists(Simulators::SimulatorType type, Simulators::SimulationType kind) const override
		{
			if (simulatorsForOptimizations.empty()) return true;

			return simulatorsForOptimizations.find({ type, kind }) != simulatorsForOptimizations.end();
		}

		/**
		 * @brief Clone the network.
		 *
		 * Clone the network in a pristine state.
		 * @return A shared pointer to the cloned network.
		 */
		std::shared_ptr<INetwork<Time>> Clone() const override
		{
			const size_t numHosts = GetNumHosts();

			std::vector<Types::qubit_t> qubits(numHosts);
			std::vector<size_t> cbits(numHosts);

			for (size_t h = 0; h < numHosts; ++h)
			{
				qubits[h] = GetNumQubitsForHost(h);
				cbits[h] = GetNumClassicalBitsForHost(h);
			}

			const auto cloned = std::make_shared<SimpleDisconnectedNetwork<Time, Controller>>(qubits, cbits);

			cloned->maxBondDim = maxBondDim;
			cloned->singularValueThreshold = singularValueThreshold;
			cloned->mpsSample = mpsSample;

			//cloned->optimizeSimulator = optimizeSimulator;
			//cloned->simulatorsForOptimizations = simulatorsForOptimizations;

			if (GetSimulator())
				cloned->CreateSimulator(GetSimulator()->GetType(), GetSimulator()->GetSimulationType());

			return cloned;
		}

		std::shared_ptr<Simulators::ISimulator> ChooseBestSimulator(const std::shared_ptr<Circuits::Circuit<Time>>& dcirc, size_t& counts, size_t nrQubits, size_t nrCbits, size_t nrResultCbits, 
			Simulators::SimulatorType& simType, Simulators::SimulationType& method, std::vector<bool>& executed, bool multithreading = false, bool dontRunCircuitStart = false) const override
		{
			if (!optimizeSimulator || !simulatorsEstimator)
				return nullptr;

			// when multithreading is set to true it means it needs a multithreaded simulator

			std::vector<std::pair<Simulators::SimulatorType, Simulators::SimulationType>> simulatorTypes;

			const bool checkTensorNetwork = method == Simulators::SimulationType::kTensorNetwork;

			// the others are to be picked between statevector, composite, tensor networks and mps, for now at least
			// for tensor networks in the future it's worth checking different contractors!!!!
			// 
				// clifford was decided at higher level
			if (method == Simulators::SimulationType::kStabilizer)
			{
				// compare qcsim with qiskit aer if qiskit aer is available, let the best one win
				if (OptimizationSimulatorExists(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kStabilizer))
					simulatorTypes.emplace_back(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kStabilizer);

#ifndef NO_QISKIT_AER
				// if the number of shots is too small, probably it's not worth it, it's going to be better to just execute them multithreading
				if (OptimizationSimulatorExists(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kStabilizer))
					simulatorTypes.emplace_back(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kStabilizer);
#endif
			}

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kStatevector))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kStatevector);

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kCompositeQCSim, Simulators::SimulationType::kStatevector))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kCompositeQCSim, Simulators::SimulationType::kStatevector);

			if (checkTensorNetwork && OptimizationSimulatorExists(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kTensorNetwork))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kTensorNetwork);

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kMatrixProductState) &&
				(nrQubits <= 4 || !maxBondDim.empty()))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kQCSim, Simulators::SimulationType::kMatrixProductState);

#ifndef NO_QISKIT_AER
			// tensor networks are out of the picture for now for qiskit aer, since they are available with cuda library, and work only on linux (obviously when compiled properly and if there is the right hw an driver installed)

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kStatevector))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kStatevector);

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kCompositeQiskitAer, Simulators::SimulationType::kStatevector))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kCompositeQiskitAer, Simulators::SimulationType::kStatevector);

			if (OptimizationSimulatorExists(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kMatrixProductState) &&
				(nrQubits <= 4 || !maxBondDim.empty()))
				simulatorTypes.emplace_back(Simulators::SimulatorType::kQiskitAer, Simulators::SimulationType::kMatrixProductState);
#endif

#ifdef __linux__
			if (Simulators::SimulatorsFactory::IsGpuLibraryAvailable()) {
				if (OptimizationSimulatorExists(Simulators::SimulatorType::kGpuSim, Simulators::SimulationType::kStatevector))
					simulatorTypes.emplace_back(Simulators::SimulatorType::kGpuSim, Simulators::SimulationType::kStatevector);
				if (OptimizationSimulatorExists(Simulators::SimulatorType::kGpuSim, Simulators::SimulationType::kMatrixProductState))
					simulatorTypes.emplace_back(Simulators::SimulatorType::kGpuSim, Simulators::SimulationType::kMatrixProductState);
			}
#endif

			if (simulatorTypes.empty()) return nullptr;
			else if (simulatorTypes.size() == 1)
			{
				simType = simulatorTypes[0].first;
				method = simulatorTypes[0].second;

				std::shared_ptr<Simulators::ISimulator> sim = Simulators::SimulatorsFactory::CreateSimulator(simType, method);
				if (sim)
				{
					if (method == Simulators::SimulationType::kMatrixProductState)
					{
						if (!maxBondDim.empty()) sim->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
						if (!singularValueThreshold.empty()) sim->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
						if (!mpsSample.empty()) sim->Configure("mps_sample_measure_algorithm", mpsSample.c_str());
					}
					sim->SetMultithreading(true);
					if (!dontRunCircuitStart) Estimators::SimulatorsEstimatorInterface<Time>::ExecuteUpToMeasurements(dcirc, nrQubits, nrCbits, nrResultCbits, sim, executed, multithreading);

					return sim;
				}
			}

			return simulatorsEstimator->ChooseBestSimulator(simulatorTypes, dcirc, counts, nrQubits, nrCbits, nrResultCbits, simType, method, executed,
				maxBondDim, singularValueThreshold, mpsSample,
				GetMaxSimulators(), pauliStrings,
				multithreading, dontRunCircuitStart);
		}


	protected:
		/**
		 * @brief Converts back the state from the optimized network distribution mapping
		 *
		 * Converts back the state from the optimized network distribution mapping.
		 * It's used only if there is an optimiser set, that is, there was a rempapping/optimizing of the circuit to be distributed.
		 */
		void ConvertBackState()
		{
			auto optimiser = controller->GetOptimiser();
			if (optimiser)
			{
				// convert the classical state results back to the expected order
				const auto& qubitsMap = optimiser->GetReverseQubitsMap();

				ConvertBackState(qubitsMap);
			}
		}

		/**
		 * @brief Converts back the state using the passed qubits map
		 *
		 * Converts back the state using the passed qubits map.
		 * It's used only if there is an optimiser set, that is, there was a rempapping/optimizing of the circuit to be distributed.
		 * Or if there is an execution on a single host and the passed circuit had a different mapping for qubits than the host qubits.
		 *
		 * @param qubitsMap The map with the qubits mapping.
		 */
		void ConvertBackState(const std::unordered_map<Types::qubit_t, Types::qubit_t>& qubitsMap)
		{
			// might not be the one stored in the network, might exist in the DES
			Circuits::OperationState& theClassicalState = GetState();

			theClassicalState.Remap(qubitsMap);
		}

		/**
		 * @brief Converts back the results from the optimized network distribution mapping
		 *
		 * Converts back the results from the optimized network distribution mapping.
		 * It's used only if there is an optimiser set, that is, there was a rempapping/optimizing of the circuit to be distributed.
		 *
		 * @param res The results to convert back.
		 */
		void ConvertBackResults(ExecuteResults& res)
		{
			auto optimiser = controller->GetOptimiser();
			if (optimiser)
			{
				// convert the classical state results back to the expected order
				const auto& qubitsMap = optimiser->GetReverseQubitsMap();

				ConvertBackResults(res, qubitsMap);
			}
		}

		/**
		 * @brief Converts back the results using the passed qubits map
		 *
		 * Converts back the results using the passed qubits map
		 * It's used only if there is an optimiser set, that is, there was a rempapping/optimizing of the circuit to be distributed.
		 * Or if there is an execution on a single host and the passed circuit had a different mapping for qubits than the host qubits.
		 *
		 * @param res The results to convert back.
		 * @param bitsMap The map with the bits mapping.
		 */
		void ConvertBackResults(ExecuteResults& res, const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap) const
		{
			ExecuteResults translatedRes;

			size_t numClassicalBits = 0;
			for (const auto& [q, b] : bitsMap)
				if (b >= numClassicalBits) numClassicalBits = b + 1;

			numClassicalBits = std::max(numClassicalBits, GetNumClassicalBits());

			for (const auto& r : res)
			{
				Circuits::OperationState translatedState(r.first);

				translatedState.Remap(bitsMap, false, numClassicalBits);
				translatedRes[translatedState.GetAllBits()] = r.second;
			}

			res.swap(translatedRes);
		}


		/**
		 * @brief Map the circuit on the host.
		 *
		 * Map the circuit on the host.
		 * It's used only if there is an execution on a single host and the passed circuit had a different mapping for qubits than the host qubits.
		 *
		 * @param circuit The circuit to map.
		 * @param hostId The id of the host to map the circuit on.
		 * @return The reverse qubits/cbits map.
		 */
		std::unordered_map<Types::qubit_t, Types::qubit_t> MapCircuitOnHost(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId, size_t& nrQubits, size_t& nrCbits, bool useSeparateSimForHosts = false)
		{
			qubitsMapOnHost.clear();
			nrQubits = 0;
			nrCbits = 0;
			if (!circuit) return {};

			size_t mxq = 0;
			size_t mnq = std::numeric_limits<size_t>::max();
			size_t mxb = 0;
			size_t mnb = std::numeric_limits<size_t>::max();

			for (const auto& op : circuit->GetOperations())
			{
				const auto qbits = op->AffectedQubits();
				for (auto q : qbits)
				{
					if (q > mxq) mxq = q;
					if (q < mnq) mnq = q;
				}
				const auto cbits = op->AffectedBits();
				for (auto b : cbits)
				{
					if (b > mxb) mxb = b;
					if (b < mnb) mnb = b;
				}
			}

			if (mnq > mxq) mnq = 0;
			if (mnb > mxb) mnb = 0;

			const auto host = std::static_pointer_cast<SimpleHost<Time>>(GetHost(hostId));
			const size_t hostNrQubits = host->GetNumQubits();

			nrQubits = mxq - mnq + 1;
			nrCbits = mxb - mnb + 1;
			if (nrCbits < nrQubits) nrCbits = nrQubits;

			const size_t startQubit = host->GetStartQubitId();

			std::unordered_map<Types::qubit_t, Types::qubit_t> reverseQubitsMap;

			if (!useSeparateSimForHosts && (mnq < startQubit || mxq >= startQubit + hostNrQubits))
			{
				if (nrQubits > hostNrQubits + 1) // the host has an additional 'special' qubit for the entanglement or other operations (like those for cutting)
					throw std::runtime_error("Circuit does not fit on the host!");

				for (size_t i = 0; i < nrCbits; ++i)
				{
					const size_t mapFrom = mnq + i;
					const size_t mapTo = startQubit + i;

					qubitsMapOnHost[mapFrom] = mapTo;
					reverseQubitsMap[mapTo] = mapFrom;
				}

				distCirc = std::static_pointer_cast<Circuits::Circuit<Time>>(circuit->Remap(qubitsMapOnHost, qubitsMapOnHost));
			}
			else if (useSeparateSimForHosts /*&& mnq > 0*/)
			{
				/*
				for (size_t i = 0; i < nrCbits; ++i)
				{
					const size_t mapFrom = mnq + i;
					const size_t mapTo = i;

					qubitsMapOnHost[mapFrom] = mapTo;
					reverseQubitsMap[mapTo] = mapFrom;
				}
				*/

				distCirc = circuit->RemapToContinuous(qubitsMapOnHost, reverseQubitsMap, nrQubits, nrCbits);

				nrQubits = qubitsMapOnHost.size();
				if (nrQubits == 0) nrQubits = 1;

				if (nrQubits > hostNrQubits + 1) // the host has an additional 'special' qubit for the entanglement or other operations (like those for cutting)
					throw std::runtime_error("Circuit does not fit on the host!");
			}

			return reverseQubitsMap;
		}

		bool optimizeSimulator = true;                     /**< The flag to optimize the simulator. */
		typename BaseClass::SimulatorsSet simulatorsForOptimizations;

		Simulators::SimulatorType lastSimulatorType = Simulators::SimulatorType::kQCSim;	/**< The last simulator type used. */
		Simulators::SimulationType lastMethod = Simulators::SimulationType::kStatevector;	/**< The last simulation method used. */

		std::string maxBondDim;
		std::string singularValueThreshold;
		std::string mpsSample;

		size_t maxSimulators = QC::QubitRegisterCalculator<>::GetNumberOfThreads(); /**< The maximum number of simulators that can be used in the network. */

		Circuits::OperationState classicalState;           /**< The classical state of the network. */
		std::shared_ptr<Simulators::ISimulator> simulator; /**< The quantum computing simulator for the network. */

		std::shared_ptr<Circuits::Circuit<Time>> distCirc; /**< The distributed circuit. */

		std::shared_ptr<IController<Time>> controller;     /**< The controller for the network. */
		// TODO: depending on the network topology, we will have adiacency lists, etc.
		// or simply a vector of hosts for a totally connected network (or where the communication details do not matter so much)
		std::vector<std::shared_ptr<IHost<Time>>> hosts;   /**< The hosts in the network. */

		std::unique_ptr<Estimators::SimulatorsEstimatorInterface<Time>> simulatorsEstimator;  /**< The simulators estimator. */

	private:
		Utils::ThreadsPool<ExecuteJob<Time>> threadsPool;  /**< The threads pool for the execution of the circuits. */
		bool recreateIfNeeded = true;               /**< The flag to recreate the simulator if needed. */
		std::unordered_map<Types::qubit_t, Types::qubit_t> qubitsMapOnHost; /**< The map with the qubits mapping when executing on a host. Relevant only when computing expectation values. */
		const std::vector<std::string>* pauliStrings = nullptr;  /**< Set to the vector of pauli strings if computing the expectation values. */
	};

}

#endif // !_SIMPLE_NETWORK_H_
