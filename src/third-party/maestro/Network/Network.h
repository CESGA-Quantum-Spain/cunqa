/**
 * @file Network.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The network interface.
 *
 * Must be derived from by the particular network implementations.
 */

#pragma once

#ifndef _NETWORK_INTERFACE_H_
#define _NETWORK_INTERFACE_H_

#include <vector>
#include <memory>
#include <boost/container_hash/hash.hpp>
#include <unordered_set>

#include "../Simulators/Factory.h"
#include "../Circuit/Circuit.h"
#include "Controller.h"



namespace Network {

	/**
	 * @enum NetworkType
	 * @brief The type of the network.
	 */
	enum class NetworkType {
		kSimpleDisconnectedNetwork,         /**< Simple network, no communication among hosts, sequential simulation */
		kSimpleNetwork,    /**< Simple fully connected network, sequential simulation */
		kSimpleDESNetwork,  /**< Simple fully connected network, discrete events simulation */
		kNetqasmNetwork,    /**< Fully connected network, hosts executing netqasm code with cooperative multitasking */
		kNetqasmDESNetwork  /**< Fully connected network, hosts executing netqasm code with discrete events simulation */
	};

	/**
	 * @class INetwork
	 * @brief The network interface.
	 *
	 * The network interface.
	 * Must be derived from by the particular network implementations.
	 * 
	 * @tparam Time The time type used for execution times.
	 * @sa IHost
	 * @sa IController
	 */
	template<typename Time = Types::time_type> class INetwork : public std::enable_shared_from_this<INetwork<Time>> {
	public:
		using ExecuteResults = typename Circuits::Circuit<Time>::ExecuteResults;
		using SimulatorPair = std::pair<Simulators::SimulatorType, Simulators::SimulationType>;
		using SimulatorsSet = std::unordered_set<SimulatorPair, boost::hash<SimulatorPair>>;

		/**
		 * @brief Destroy the INetwork object.
		 * 
		 * Destroy the INetwork object. Virtual because this is an interface.
		 */
		virtual ~INetwork() = default;


		/**
		 * @brief Get the number of gates that span more than one host.
		 *
		 * Get the number of gates that span more than one host for the given circuit.
		 *
		 * @param circuit The circuit to check.
		 * @return The number of gates that need distribution or cutting.
		 */
		virtual size_t GetNumberOfGatesDistributedOrCut(const std::shared_ptr<Circuits::Circuit<Time>>&circuit) const = 0;


		/**
		 * @brief Execute the circuit on the network.
		 *
		 * Execute the circuit on the network, using the controller for distributing the operations to the hosts.
		 * The base class functionality is used for circuit distribution, but then the distributed circuit is converted to netqasm.
		 * Ensure the quantum computing simulator and the netqasm virtual machines have been created before calling this.
		 *
		 * @param circuit The circuit to execute.
		 * @sa Circuits::Circuit
		 */
		virtual void Execute(const std::shared_ptr<Circuits::Circuit<Time>>& circuit) = 0;


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
		virtual void ExecuteOnHost(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId) = 0;


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
		virtual std::vector<double> ExecuteExpectations(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, const std::vector<std::string>& paulis) = 0;


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
		virtual std::vector<double> ExecuteOnHostExpectations(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId, const std::vector<std::string>& paulis) = 0;


		/**
		 * @brief Execute the circuit on the network, repeatedly.
		 *
		 * Execute the circuit on the network, distributing the operations to the hosts, repeating the execution 'shots' times.
		 * The way the circuit is distributed to the hosts depends on the specific interface implementations.
		 *
		 * @param circuit The circuit to execute.
		 * @param shots The number of times to repeat the execution.
		 * @return A vector of maps with the results of each circuit execution, where the key is the state as a vector of bools and the value is the number of times it was measured.
		 * @sa Circuits::Circuit
		 */
		virtual ExecuteResults RepeatedExecute(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t shots = 1000) = 0;

		/**
		 * @brief Execute the circuit on the network, repeatedly.
		 *
		 * Execute the circuit on the network, distributing the operations to the hosts, repeating the execution 'shots' times.
		 * The way the circuit is distributed to the hosts depends on the specific interface implementations.
		 *
		 * @param executeCircuit The circuit to execute, together with the number of shots.
		 * @return A vector of maps with the results of each circuit execution, where the key is the state as a vector of bools and the value is the number of times it was measured.
		 * @sa Circuits::Circuit
		 * @sa ExecuteCircuit
		 */
		ExecuteResults RepeatedExecuteCircuit(const Schedulers::ExecuteCircuit<Time>& executeCircuit)
		{
			return RepeatedExecute(executeCircuit.circuit, executeCircuit.shots);
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
		 * @return A vector of maps with the results of each circuit execution, where the key is the state as a vector of bools and the value is the number of times it was measured.
		 * @sa Circuits::Circuit
		 */
		virtual ExecuteResults RepeatedExecuteOnHost(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t hostId, size_t shots = 1000) = 0;

		/**
		 * @brief Execute the circuit on the specified host, repeatedly.
		 *
		 * Execute the circuit on the specified host, repeating the execution 'shots' times.
		 * The circuit must fit on the host, otherwise an exception is thrown.
		 * The circuit will be mapped on the specified host, if its qubits start with indexing from 0 (if already mapped, the qubits won't be altered).
		 *
		 * @param executeCircuit The circuit to execute, together with the number of shots.
		 * @param hostId The id of the host to execute the circuit on.
		 * @return A vector of maps with the results of each circuit execution, where the key is the state as a vector of bools and the value is the number of times it was measured.
		 * @sa Circuits::Circuit
		 * @sa ExecuteCircuit
		 */
		ExecuteResults RepeatedExecuteCircuitOnHost(const Schedulers::ExecuteCircuit<Time>& executeCircuit, size_t hostId)
		{
			return RepeatedExecuteOnHost(executeCircuit.circuit, hostId, executeCircuit.shots);
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
		virtual std::vector<ExecuteResults> ExecuteScheduled(const std::vector<Schedulers::ExecuteCircuit<Time>>& circuits) = 0;

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
		virtual void CreateSimulator(Simulators::SimulatorType simType = 
			Simulators::SimulatorType::kQCSim	
			,
			Simulators::SimulationType simExecType = Simulators::SimulationType::kMatrixProductState, size_t nrQubits = 0
		) = 0;

		/**
		 * @brief Get the simulator for the network.
		 * 
		 * Get the simulator for the network.
		 * 
		 * @return The simulator for the network.
		 * @sa Simulators::ISimulator
		 */
		virtual std::shared_ptr<Simulators::ISimulator> GetSimulator() const = 0;


		/**
		 * @brief Configures the network.
		 *
		 * This function is called to configure the network (for example the simulator(s) used by the network.
		 * 
		 * @param key The key of the configuration option.
		 * @param value The value of the configuration.
		 */
		virtual void Configure(const char* key, const char* value) = 0;


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
		virtual void CreateScheduler(SchedulerType schType = SchedulerType::kNoEntanglementQubitsParallel) = 0;


		/**
		 * @brief Get the scheduler for the network.
		 *
		 * Get the scheduler for the network.
		 *
		 * @return The scheduler for the network.
		 * @sa Schedulers::IScheduler
		 */
		virtual std::shared_ptr<Schedulers::IScheduler<Time>> GetScheduler() const = 0;

		/**
		 * @brief Get the host with the specified id.
		 * 
		 * Get a smart pointer to the host that has the specified id.
		 * 
		 * @param hostId The id of the host to get.
		 * @return A smart pointer to the host that has the specified id.
		 * @sa IHost
		 */
		virtual const std::shared_ptr<IHost<Time>> GetHost(size_t hostId) const = 0;

		/**
		 * @brief Get the controller for the network.
		 * 
		 * Gets a smart pointer to the controller for the network.
		 * 
		 * @return The controller for the network.
		 * @sa IController
		 */
		virtual const std::shared_ptr<IController<Time>> GetController() const = 0;

		/**
		 * @brief Get the classical state of the network.
		 * 
		 * Gets a reference to the classical state of the network.
		 * 
		 * @return The classical state of the network.
		 * @sa Circuits::ClassicalState
		 */
		virtual Circuits::OperationState& GetState() = 0;

		/**
		 * @brief Get the number of hosts in the network.
		 * 
		 * Get the number of hosts in the network, excluding the controller.
		 * 
		 * @return The number of hosts in the network.
		 */
		virtual size_t GetNumHosts() const = 0;

		/**
		 * @brief Get the number of qubits in the network.
		 * 
		 * Get the number of qubits in the network, excluding the qubits used for entanglement between the hosts.
		 * 
		 * @return The number of qubits in the network.
		 */
		virtual size_t GetNumQubits() const = 0;

		/**
		 * @brief Get the number of qubits in the network for the specified host.
		 * 
		 * Get the number of qubits in the network for the specified host, excluding the qubits used for entanglement between the hosts.
		 * 
		 * @param hostId The id of the host to get the number of qubits for.
		 * @return The number of qubits in the network for the specified host.
		 */
		virtual size_t GetNumQubitsForHost(size_t hostId) const = 0;

		/**
		 * @brief Get the number of qubits used for entanglement between hosts.
		 * 
		 * Get the number of qubits used for entanglement between hosts in the network.
		 * 
		 * @return The number of qubits used for entanglement between hosts.
		 */
		virtual size_t GetNumNetworkEntangledQubits() const = 0;

		/**
		 * @brief Get the number of qubits used for entanglement between hosts for the specified host.
		 * 
		 * Get the number of qubits used for entanglement between hosts in the network for the specified host.
		 * Usually it's a single qubit.
		 * 
		 * @param hostId The id of the host to get the number of qubits used for entanglement between hosts for.
		 * @return The number of qubits used for entanglement between hosts for the specified host.
		 */
		virtual size_t GetNumNetworkEntangledQubitsForHost(size_t hostId) const = 0;

		/**
		 * @brief Get the number of classical bits in the network.
		 * 
		 * Get the number of classical bits in the network, excluding the classical bits used for measurement of entanglement qubits between the hosts.
		 * 
		 * @return The number of classical bits in the network.
		 */
		virtual size_t GetNumClassicalBits() const = 0;

		/**
		 * @brief Get the number of classical bits in the network for the specified host.
		 * 
		 * Get the number of classical bits in the network for the specified host, excluding the classical bits used for measurement of entanglement qubits between the hosts.
		 * 
		 * @param hostId The id of the host to get the number of classical bits for.
		 * @return The number of classical bits in the network for the specified host.
		 */
		virtual size_t GetNumClassicalBitsForHost(size_t hostId) const = 0;

		/**
		 * @brief Check if the specified qubits are on the same host.
		 * 
		 * Check if the specified qubits are on the same host. This does not include the qubits used for entanglement between the hosts.
		 * 
		 * @return True if the specified qubits are on the same host, false otherwise.
		 */
		virtual bool AreQubitsOnSameHost(size_t qubitId1, size_t qubitId2) const = 0;

		/**
		 * @brief Check if the specified classical bits are on the same host.
		 * 
		 * Check if the specified classical are on the same host. This does not include the classical bits used for measuring the qubits used for entanglement between the hosts.
		 * 
		 * @return True if the specified qubits are on the same host, false otherwise.
		 */
		virtual bool AreClassicalBitsOnSameHost(size_t bitId1, size_t bitId2) const = 0;

		/**
		 * @brief Check if the specified qubit and classical bit are on the same host.
		 * 
		 * Check if the specified qubit and classical bit are on the same host. This does not include the qubits used for entanglement between the hosts and the classical bits used for their measurement.
		 * 
		 * @return True if the specified qubit and classical bit are on the same host, false otherwise.
		 */
		virtual bool AreQubitAndClassicalBitOnSameHost(size_t qubitId, size_t bitId) const = 0;

		/**
		 * @brief Get the host id for the specified qubit.
		 * 
		 * Get the host id for the specified qubit, excluding the qubits used for entanglement between hosts.
		 * 
		 * @param qubitId The id of the qubit to get the host id for.
		 * @return The host id for the specified qubit.
		 */
		virtual size_t GetHostIdForQubit(size_t qubitId) const = 0;

		/**
		 * @brief Get the host id for the specified qubit used for entanglement between hosts.
		 * 
		 * Get the host id for the specified qubit used for entanglement between hosts.
		 * 
		 * @param qubitId The id of qubit the used for entanglement between hosts to get the host id for.
		 * @return The host id for the specified qubit used for entanglement between hosts.
		 */
		virtual size_t GetHostIdForEntangledQubit(size_t qubitId) const = 0;

		/**
		 * @brief Get the host id for the specified qubit.
		 * 
		 * Get the host id for the squbit, including the qubits used for entanglement between hosts.
		 * 
		 * @param qubitId The id of the qubit to get the host id for.
		 * @return The host id for the specified qubit.
		 */
		virtual size_t GetHostIdForAnyQubit(size_t qubitId) const = 0;
		
		/**
		 * @brief Get the host id for the specified classical bit.
		 * 
		 * Get the host id for the specified classical bit, including the one(s) used for measurement of entanglement qubits between the hosts.
		 * 
		 * @param classicalBitId The id of the classical bit to get the host id for.
		 * @return The host id for the specified classical bit.
		 */
		virtual size_t GetHostIdForClassicalBit(size_t classicalBitId) const = 0;

		/**
		 * @brief Get the qubit ids for the specified host.
		 * 
		 * Get the qubit ids for the specified host.
		 * 
		 * @param hostId The id of the host to get the qubit ids for.
		 * @return A vector with the qubit ids.
		 */
		virtual std::vector<size_t> GetQubitsIds(size_t hostId) const = 0;

		/**
		 * @brief Get the qubit ids used for entanglement between hosts for the specified host.
		 *
		 * Get the qubit ids used for entanglement between hosts  for the specified host.
		 *
		 * @param hostId The id of the host to get the qubit ids for.
		 * @return A vector with the qubit ids.
		 */
		virtual std::vector<size_t> GetNetworkEntangledQubitsIds(size_t hostId) const = 0;

		/**
		 * @brief Get the classical bit ids for the specified host.
		 *
		 * Get the classical bit ids for the specified host, excluding the one(s) used for measurement of entanglemen qubits between the hosts.
		 *
		 * @param hostId The id of the host to get the classical bit ids for.
		 * @return A vector with the classical bit ids.
		 */
		virtual std::vector<size_t> GetClassicalBitsIds(size_t hostId) const = 0;

		/**
		 * @brief Get the classical bit ids used for measurement of entanglement qubits between the hosts for the specified host.
		 *
		 * Get the classical bit ids used for measurement of entanglement qubits between the hosts for the specified host.
		 *
		 * @param hostId The id of the host to get the classical bit ids for.
		 * @return A vector with the classical bit ids.
		 */
		virtual std::vector<size_t> GetEntangledQubitMeasurementBitIds(size_t hostId) const = 0;

		/**
		 * @brief Check if the specified qubit id is for a qubit used for entanglement between hosts.
		 *
		 * Check if the specified qubit id is for a qubit used for entanglement between hosts.
		 *
		 * @param qubitId The id of the qubit to check.
		 * @return True if the specified qubit id is for a qubit used for entanglement between hosts, false otherwise.
		 */
		virtual bool IsNetworkEntangledQubit(size_t qubitId) const = 0;

		/**
		 * @brief Check if the circuit operation is local.
		 *
		 * Check if the specified circuit operation is local. A local operation is an operation that is executed on a single host.
		 * This does not include the operations that also operate on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is local, false otherwise.
		 */
		virtual bool IsLocalOperation(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Check if the circuit operation is distributed.
		 *
		 * Check if the specified circuit operation is distributed. A distributed operation is an operation that is executed on multiple hosts.
		 * This does not include the operations that also operate on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is distributed, false otherwise.
		 */
		virtual bool IsDistributedOperation(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Check if the circuit operation operates on the entanglement qubits between hosts.
		 *
		 * Check if the specified circuit operation operates on the entanglement qubits between hosts.
		 *
		 * @param op The operation to check.
		 * @return True if the specified circuit operation operates on the entanglement qubits between hosts, false otherwise.
		 */
		virtual bool OperatesWithNetworkEntangledQubit(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Sends a packet between two hosts.
		 *
		 * Sends a packet between the two specified hosts.
		 * 
		 * @param fromHostId The id of the host to send the packet from.
		 * @param toHostId The id of the host to send the packet to.
		 * @param packet The packet to send.
		 * @return True if the packet was sent successfully, false otherwise.
		 */
		virtual bool SendPacket(size_t fromHostId, size_t toHostId, const std::vector<uint8_t>& packet) = 0;

		/**
		 * @brief Checks if a gate is an entangling gate.
		 * 
		 * An entangling gate is a gate that operates on two qubits that are used for entanglement between hosts.
		 * @param op The operation to check.
		 * @return True if the specified circuit operation is an entangling gate, false otherwise.
		 */ 
		virtual bool IsEntanglingGate(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Checks if a gate expects a classical bit from another host.
		 *
		 * It must be a conditional gate, conditioned on a classical bit from another host.
		 * Use it on already distributed gates, not on the original circuit.
		 * 
		 * @param op The operation to check.
		 * @return True if the specified circuit operation needs a classical bit from another host, false otherwise.
		 */
		virtual bool ExpectsClassicalBitFromOtherHost(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Get the host id where the classical control bit resides for a conditioned gate.
		 *
		 * It must be a conditional gate, conditioned on a qubit from another host.
		 * Use it on already distributed gates, not on the original circuit.
		 *
		 * @param op The operation to find the host of the control bit.
		 * @return The host id.
		 */
		virtual size_t GetHostIdForClassicalControl(const std::shared_ptr<Circuits::IOperation<Time>>& op) const = 0;

		/**
		 * @brief Check if the specified qubit used for entanglement between hosts is busy.
		 *
		 * Check if the specified qubit used for entanglement between hosts is busy.
		 *
		 * @param qubitId The id of the qubit to check.
		 * @return True if the specified qubit used for entanglement between hosts is busy, false otherwise.
		 */
		virtual bool IsEntanglementQubitBusy(size_t qubitId) const = 0;

		/**
		 * @brief Check if any of the two specified qubits used for entanglement between hosts are busy.
		 *
		 * Check if any of the two specified qubits used for entanglement between hosts are busy.
		 * This is used to check if the qubits are free in order to use them for creating an entanglement between hosts.
		 * 
		 * @param qubitId1 The id of the first qubit to check.
		 * @param qubitId2 The id of the second qubit to check.
		 * @return True if any of the two specified qubits used for entanglement between hosts are busy, false otherwise.
		 */
		virtual bool AreEntanglementQubitsBusy(size_t qubitId1, size_t qubitId2) const = 0;

		/**
		 * @brief Mark the pair of the specified qubits used for entanglement between hosts as busy.
		 *
		 * Mark the pair of the specified qubits used for entanglement between hosts as busy.
		 * This is used to mark the qubits as busy when they are used for creating an entanglement between hosts.
		 * @param qubitId1 The id of the first qubit to mark as busy.
		 * @param qubitId2 The id of the second qubit to mark as busy.
		 */
		virtual void MarkEntangledQubitsBusy(size_t qubitId1, size_t qubitId2) = 0;
		
		/**
		 * @brief Mark the specified qubit used for entanglement between hosts as free.
		 *
		 * Mark the specified qubit used for entanglement between hosts as free.
		 * This is used to mark the qubits as free when they are not used anymore for an entanglement between hosts.
		 * @param qubitId The id of the qubit to mark as free.
		 */
		virtual void MarkEntangledQubitFree(size_t qubitId) = 0;

		/**
		 * @brief Clear all entanglements between hosts in the network.
		 * 
		 * Clear all entanglements between hosts in the network.
		 * This marks all qubits used for entanglement between hosts as free.
		 * If the entanglements are explicitely coordinated in the network, all pairs of entangled qubits are released.
		 */
		virtual void ClearEntanglements() = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<INetwork<Time>> getptr()
		{
			return std::enable_shared_from_this<INetwork<Time>>::shared_from_this();
		}

		/**
		 * @brief Get the type of the network.
		 *
		 * Get the type of the network.
		 * @return The type of the network.
		 * @sa NetworkType
		 */
		virtual NetworkType GetType() const = 0;

		/**
		 * @brief Clone the network.
		 *
		 * Clone the network in a pristine state.
		 * @return A shared pointer to the cloned network.
		 */
		virtual std::shared_ptr<INetwork<Time>> Clone() const = 0;

		/**
		 * @brief Get the distributed circuit.
		 *
		 * Get the distributed circuit.
		 * Execute() must be called first, otherwise the return would be nullptr.
		 * 
		 * @return The distributed circuit.
		 * @sa Circuits::Circuit
		 */
		virtual std::shared_ptr<Circuits::Circuit<Time>> GetDistributedCircuit() const = 0;

		/**
		 * @brief Allows using an optimized simulator.
		 * 
		 * If set, allows changing the simulator with an optimized one.
		 * States/amplitudes are not available in such a case, disable if you need them.
		 *
		 * @param optimize If true, the simulator will be optimized if possible.
		 */
		virtual void SetOptimizeSimulator(bool optimize = true) = 0;


		/**
		 * @brief Returns the 'optimize' flag.
		 * 
		 * Returns the flag set by SetOptimizeSimulator().
		 *
		 * @return The 'optimize' flag.
		 */
		virtual bool GetOptimizeSimulator() const = 0;


		/**
		 * @brief Get the last used simulator type.
		 *
		 * Get the last used simulator type.
		 * 
		 * @return The simulator type that was used last time.
		 */
		virtual Simulators::SimulatorType GetLastSimulatorType() const = 0;

		/**
		 * @brief Get the last used simulation type.
		 *
		 * Get the last used simulation type.
		 *
		 * @return The simulation type that was used last time.
		 */
		virtual Simulators::SimulationType GetLastSimulationType() const = 0;

		/**
		 * @brief Get the optimizations simulators set.
		 *
		 * Get the optimization simulators set.
		 * To be used internally, will not be exposed from the library.
		 *
		 * @return The simulators set.
		 */
		virtual const SimulatorsSet& GetSimulatorsSet() const = 0;

		/**
		 * @brief Adds a simulator to the simulators optimization set.
		 *
		 * Adds a simulator (if not already present) to the simulators optimization set.
		 * 
		 * @param type The type of the simulator to add.
		 * @param kind The kind of the simulation to add.
		 */
		virtual void AddOptimizationSimulator(Simulators::SimulatorType type, Simulators::SimulationType kind) = 0;

		/**
		 * @brief Removes a simulator from the simulators optimization set.
		 *
		 * Removes a simulator from the simulators optimization set, if it exists.
		 *
		 * @param type The type of the simulator to remove.
		 * @param kind The kind of the simulation to remove.
		 */
		virtual void RemoveOptimizationSimulator(Simulators::SimulatorType type, Simulators::SimulationType kind) = 0;

		/**
		 * @brief Removes all simulators from the simulators optimization set and adds the one specified.
		 *
		 * Removes all simulators from the simulators optimization set and adds the one specified.
		 *
		 * @param type The type of the simulator to add.
		 * @param kind The kind of the simulation to add.
		 */
		virtual void RemoveAllOptimizationSimulatorsAndAdd(Simulators::SimulatorType type, Simulators::SimulationType kind) = 0;

		/**
		 * @brief Checks if a simulator exists in the optimization set.
		 *
		 * Checks if a simulator exists in the optimization set.
		 *
		 * @param type The type of the simulator to check.
		 * @param kind The kind of the simulation to check.
		 * @return True if the simulator exists in the optimization set, false otherwise.
		 */
		virtual bool OptimizationSimulatorExists(Simulators::SimulatorType type, Simulators::SimulationType kind) const = 0;


		/**
		 * @brief Get the maximum number of simulators that can be used in the network.
		 *
		 * Get the maximum number of simulators that can be used in the network.
		 * This is used to limit the number of simulators (and corresponding threads) that can be used in the network.
		 *
		 * @return The maximum number of simulators that can be used in the network.
		 */
		virtual size_t GetMaxSimulators() const = 0;

		/**
		 * @brief Set the maximum number of simulators that can be used in the network.
		 *
		 * Set the maximum number of simulators that can be used in the network.
		 * This is used to limit the number of simulators (and corresponding threads) that can be used in the network.
		 *
		 * @param maxSimulators The maximum number of simulators that can be used in the network.
		 */
		virtual void SetMaxSimulators(size_t maxSimulators) = 0;

		/**
		 * @brief Get the text code that is executed on the hosts.
		 * 
		 * Get the text code that is executed on the hosts, if available.
		 * The circuit might be converted to code to be executed (example: netqasm).
		 * If that's the case, the text code is returned in an array, each element of the array being the code for a host.
		 * Each host might have multiple subroutines, so each element of the array is an array of strings, each string being a subroutine.
		 * 
		 * @return The text code that is executed on the hosts.
		 */
		virtual std::vector<std::vector<std::string>> GetSubroutinesTextCode() const
		{
			return {};
		}

		/**
		 * @brief Choose the best simulator for the given circuit.
		 * 
		 * Choose the best simulator for the given circuit, based on the number of qubits, number of classical bits, number of result classical bits and number of shots.
		 * The choice is made from the optimization simulators set.
		 * 
		 * @param dcirc The distributed circuit to choose the simulator for.
		 * @param counts The number of shots to be executed.
		 * @param nrQubits The number of qubits in the circuit.
		 * @param nrCbits The number of classical bits in the circuit.
		 * @param nrResultCbits The number of result classical bits in the circuit.
		 * @param simType The type of the chosen simulator.
		 * @param method The kind of simulation of the chosen simulator.
		 * @param executed A vector of bools marking the executed gates from the circuit (some of them might be executed when picking up the simulator - usually the gates from the beginning up to the measurements/resets).
		 * @param multithreading If true, allows simulators to support multithreading (default is false). Multithreading will be set to false if multithreading is implemented at a higher level (multiple simulators in parallel).
		 * @return A shared pointer to the chosen simulator.
		 */
		virtual std::shared_ptr<Simulators::ISimulator> ChooseBestSimulator(const std::shared_ptr<Circuits::Circuit<Time>>& dcirc, size_t& counts, size_t nrQubits, size_t nrCbits, size_t nrResultCbits, Simulators::SimulatorType& simType, Simulators::SimulationType& method, std::vector<bool>& executed, bool multithreading = false, bool dontRunCircuitStart = false) const = 0;
 
	};

}

#endif // !_NETWORK_INTERFACE_H_
