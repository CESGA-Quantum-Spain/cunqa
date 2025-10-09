/**
 * @file SimpleController.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * A simple network controller class.
 *
 * Implements the controller interface trivially.
 * The most important operations are to convert the circuit for distribution and to distribute it.
 * This functionality is delegated to the circuit itself and to the remapper.	
 */

#pragma once

#ifndef _SIMPLE_CONTROLLER_H_
#define _SIMPLE_CONTROLLER_H_


#include "Controller.h"
#include "../Distribution/Remapper.h"
#include "../Scheduler/Scheduler.h"
#include "../graph/Optimiser.h"

namespace Network {

	/**
	 * @class SimpleController
	 * @brief The simple controller host implementation.
	 *
	 * The controller is a special kind of host, it is the one that distributes the circuit to the other hosts.
	 * The simple controller just converts a local circuit to a distributed one.
	 *
	 * @tparam Time The time representation to use for execution times.
	 * @sa IController
	 * @sa INetwork
	 * @sa SimpleNetwork
	 * @sa Circuits::Circuit
	 */
	template<typename Time = Types::time_type> class SimpleController : public IController<Time> {
	public:
		/**
		 * @brief Distributes the circuit on the hosts.
		 *
		 * The network will call this, the conversion and distribution should happen before execution.
		 *
		 * @param network The network to distribute the circuit for.
		 * @param circuit The circuit to distribute.
		 * @return The distributed circuit.
		 */
		std::shared_ptr<Circuits::Circuit<Time>> DistributeCircuit(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) override
		{
			if (!remapper)
				return nullptr; // maybe throw an exception?

			// don't destroy the passed one, make a copy
			std::shared_ptr<Circuits::Circuit<Time>> distCirc = std::static_pointer_cast<Circuits::Circuit<Time>>(circuit->Clone());

			// this is a different optimization than below, that one is for qubits layout in the network
			// this one simpifies the circuit by merging gates
			// could be done after the distribution, perhaps, I'll think about it... might change some things that would break up the netqasm conversion, so for now it's here

			// also if it merges three cnots into a swap, for example, it shouldn't be done after 'ConvertForDistribution' because the swap is converted to 3 cnots
			if (optimize) distCirc->Optimize(optimizeRotationGates);

			// this converts the 3 qubit gates and swap to 2-qubit gates (the swap to 3 cnots)
			distCirc->ConvertForDistribution();
			
			// virtual, by default does nothing, but in derived classes it can do something
			// for example, for netqasm it arranges the measurements in the proper order for conversion
			distCirc = this->DoNetworkSpecificConversionsForDistribution(network, distCirc); 

			if (optimiser)
			{
				// optimise the circuit
				optimiser->SetNetworkAndCircuit(network, distCirc);
				optimiser->Optimise();
				const auto& qubitsMap = optimiser->GetQubitsMap();
				distCirc = std::dynamic_pointer_cast<Circuits::Circuit<Time>>(distCirc->Remap(qubitsMap, qubitsMap));
			}

			// now distribute

			// the first thing to do is to adjust the qubits indexes in the circuit according to the network topology
			// walk over the network hosts and add one qubit per host for the entangled one (as it's the case for the 'simple network')
			// and one cbit per host for measuring the entangled qubit
			// for now they will be added at the end of the registers, to be easier to convert the circuit for distribution

			return remapper->Remap(network, distCirc);
		}

		/**
		 * @brief Splits the composite operations from the circuit.
		 * 
		 * Splits some composite operations.
		 * There are composite operations - other than multiple qubits quantum gates - that act on multiple qubits/classical bits that might act on qubits that are not local.
		 * This function splits those operations into ones that act on a single qubit/classical bit.
		 * For example measurements on several qubits are split into several measurements on a single qubit.
		 * 
		 * @param network The network to execute the circuit for.
		 * @param circuit The circuit to execute.
		 * @return A shared pointer to the modified circuit.
		 * @sa Circuits::Circuit
		 * @sa INetwork
		 */
		std::shared_ptr<Circuits::Circuit<Time>> SplitCompositeOperations(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) override
		{
			if (!remapper)
				return nullptr; // maybe throw an exception?

			return remapper->SplitCompositeOperations(network, circuit);
		}

		/**
		 * @brief Get the host id.
		 *
		 * Obtain a host id used to identify the host in the network.
		 * Since this is a special host, it returns the maximum value of size_t.
		 * 
		 * @return The host id.
		 */
		size_t GetId() const override
		{
			return std::numeric_limits<size_t>::max();
		}

		/**
		 * @brief Get the number of qubits.
		 *
		 * Obtain the number of qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * The controller is a special host, it's not a quantum computer, so it returns 0.
		 * @return The number of qubits.
		 */
		size_t GetNumQubits() const override
		{
			return 0;
		}

		/**
		 * @brief Get the number of network entangled qubits.
		 *
		 * Obtain the number of qubits in the host used for entanglement with other hosts.
		 * The controller is a special host, it's not a quantum computer, so it returns 0.
		 *
		 * @return The number of network entangled qubits.
		 */
		size_t GetNumNetworkEntangledQubits() const override
		{
			return 0;
		}

		/**
		 * @brief Get the number of classical bits.
		 *
		 * Obtain the number of classical bits in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns 0.
		 * @return The number of classical bits.
		 */
		size_t GetNumClassicalBits() const override
		{
			return 0;
		}

		/**
		 * @brief Check if a qubit is in the host.
		 *
		 * Checks if a qubit with the specified id is in the host, excluding the one(s) used only for entanglement with other hosts.
		 * The controller is a special host, it's not a quantum computer, so it returns false.
		 * @return True if the qubit is on this host, false otherwise.
		 */
		bool IsQubitOnHost(size_t qubitId) const override
		{
			return false;
		}

		/**
		 * @brief Check if a classical bit is in the host.
		 *
		 * Checks if a classical bit with the specified id is in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns false.
		 * @return True if the classical bit is on this host, false otherwise.
		 */
		bool IsClassicalBitOnHost(size_t qubitId) const override
		{
			return false;
		}

		/**
		 * @brief Check if a qubit used for entanglement between hosts is in the host.
		 *
		 * Checks if a qubit used for entanglement between hosts with the specified id is in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns false.
		 * @return True if the network entangled qubit is on this host, false otherwise.
		 */
		bool IsEntangledQubitOnHost(size_t qubitId) const override
		{
			return false;
		}

		/**
		 * @brief Check if two qubits are in the same host.
		 *
		 * Checks if two qubits with the specified ids are in the same host, excluding the one(s) used only for entanglement with other hosts.
		 * The controller is a special host, it's not a quantum computer, so it returns false.
		 * @return True if the qubits are on the same host, false otherwise.
		 */
		bool AreQubitsOnSameHost(size_t qubitId1, size_t qubitId2) const override
		{
			return false;
		}

		/**
		 * @brief Check if two classical bits are in the same host.
		 *
		 * Checks if two classical bits with the specified ids are in the same host.
		 * The controller is a special host, it's not a quantum computer, so it returns false.
		 * @return True if the classical bits are on the same host, false otherwise.
		 */
		bool AreCbitsOnSameHost(size_t qubitId1, size_t qubitId2) const override
		{
			return false;
		}

		/**
		 * @brief Get the ids of the qubits in the host.
		 *
		 * Obtain the ids of the qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * The controller is a special host, it's not a quantum computer, so it returns an empty vector.
		 * @return The ids of the qubits in the host.
		 */
		std::vector<size_t> GetQubitsIds() const override
		{
			return {};
		}

		/**
		 * @brief Get the ids of the qubits used for entanglement between hosts in the host.
		 *
		 * Obtain the ids of the qubits used for entanglement between hosts in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns an empty vector.
		 * @return The ids of the qubits used for entanglement between hosts in the host.
		 */
		std::vector<size_t> GetNetworkEntangledQubitsIds() const override
		{
			return {};
		}

		/**
		 * @brief Get the ids of the classical bits in the host.
		 *
		 * Obtain the ids of the classical bits in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns an empty vector.
		 * @return The ids of the classical bits in the host.
		 */
		std::vector<size_t> GetClassicalBitsIds() const override
		{
			return {};
		}

		/**
		 * @brief Get the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 *
		 * Obtain the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 * The controller is a special host, it's not a quantum computer, so it returns an empty vector.
		 * @return The ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 */
		std::vector<size_t> GetEntangledQubitMeasurementBitIds() const override
		{
			return {};
		}

		/**
		 * @brief Changes the remapper that is used for remapping a circuit to a distributed one.
		 *
		 * The remapper is used to remap a circuit to a distributed one.
		 * A default remapper is already set, so calling this function is optional.
		 * 
		 * @param r The new remapper.
		 * @sa Distribution::IRemapper
		 */
		void SetRemapper(const std::shared_ptr<Distribution::IRemapper<Time>>& r) override
		{
			remapper = r;
		}

		/**
		 * @brief Gets the remapper that is used for remapping a circuit to a distributed one.
		 *
		 * The remapper is used to remap the circuit to a distributed one.
		 *
		 * @return The remapper.
		 * @sa Distribution::IRemapper
		 */
		std::shared_ptr<Distribution::IRemapper<Time>> GetRemapper() const override
		{
			return remapper;
		}

		/**
		 * @brief Send a packet to a host.
		 *
		 * Send a packet to a host.
		 * Usually called by the host implementation or perhaps in some cases by the network implementation.
		 * For the simple network, sending classical packets is not simulated, so this function does nothing and returns false.
		 * Most of network implementations do not simulate sending packets from the controller or receiving packets by the controller, only sending/receiving packets between quantum computers.
		 * 
		 * @param hostId The id of the host to send the packet to.
		 * @param packet The packet to send.
		 * @return False.
		 */
		bool SendPacketToHost(size_t hostId, const std::vector<uint8_t>& packet) override
		{
			return false;
		}

		/**
		 * @brief Receive a packet from a host.
		 *
		 * Receive a packet from a host.
		 * Usually called by the network implementation.
		 * For the simple network, receiving classical packets is not simulated, so this function does nothing and returns false.
		 * Most of network implementations do not simulate sending packets from the controller or receiving packets by the controller, only sending/receiving packets between quantum computers.
		 * 
		 * @param hostId The id of the host to receive the packet from.
		 * @param packet The packet to receive.
		 * @return False.
		 */
		bool RecvPacketFromHost(size_t hostId, const std::vector<uint8_t>& packet) override
		{
			return false;
		}

		/**
		 * @brief Get the id of the first qubit assigned to the host.
		 *
		 * Obtain the id of the first qubit assigned to the host, the other ones are assigned contiquously.
		 *
		 * @return The id of the first qubit assigned to the host.
		 */
		size_t GetStartQubitId() const override
		{
			return 0;
		}

		/**
		 * @brief Get the id of the first classical bit assigned to the host.
		 *
		 * Obtain the id of the first classical bit assigned to the host, the other ones are assigned contiquously.
		 *
		 * @return The id of the first classical bit assigned to the host.
		 */
		size_t GetStartClassicalBitId() const override
		{
			return 0;
		}

		/**
		 * @brief Returns the optimiser used
		 *
		 * Returns the optimiser used. Could be nullptr, that is, no optimiser.
		 *
		 * @return The optimiser used.
		 */
		std::shared_ptr<Graphs::IOptimiser<Time>> GetOptimiser() const override
		{
			return optimiser;
		}

		/**
		 * @brief Creates an optimiser.
		 *
		 * Creates an optimiser of the specified type.
		 *
		 * @param type The type of optimiser to create.
		 */
		void CreateOptimiser(Graphs::OptimiserType type) override
		{
			throw std::runtime_error("SimpleController::CreateOptimiser: not implemented for this type of network controller");
		}

		/**
		 * @brief Convert the circuit for distribution for specific networks.
		 *
		 * Convert the circuit for distribution for specific networks.
		 * Particular networks might need additional circuit conversions before distribution.
		 * They should override this implementation.
		 * By default, it does nothing.
		 *
		 * @param network The network to convert the circuit for.
		 * @param circuit The circuit to convert.
		 * @return The converted circuit.
		 * @sa Circuits::Circuit
		 */
		std::shared_ptr<Circuits::Circuit<Time>> DoNetworkSpecificConversionsForDistribution(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) override
		{
			return circuit;
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
		void CreateScheduler(const std::shared_ptr<INetwork<Time>>& network, SchedulerType schType = SchedulerType::kNoEntanglementQubitsParallel) override
		{
			throw std::runtime_error("SimpleController::CreateScheduler: not implemented for this type of network controller");
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
			return scheduler;
		}

		bool GetOptimizeRotationGates() const override
		{
			return optimizeRotationGates;
		}

		void SetOptimizeRotationGates(bool val = true) override
		{
			optimizeRotationGates = val;
		}

		/**
		 * @brief Set circuit optimization.
		 *
		 * If the parameter is true, the circuit will be optimized, otherwise it will not be optimized.
		 *
		 * @param optimize True if the circuit should be optimized, false otherwise.
		 */
		void SetOptimizeCircuit(bool o = true) override
		{
			optimize = o;
		}

		/**
		 * @brief Get circuit optimization.
		 *
		 * Returns true if the circuit will be optimized, false otherwise.
		 *
		 * @return True if the circuit will be optimized, false otherwise.
		 */
		bool GetOptimizeCircuit() const override
		{
			return optimize;
		}

	protected:
		std::shared_ptr<Distribution::IRemapper<Time>> remapper; /**< The remapper used to remap a circuit to a distributed one. */

		// the problem with setting a default optimiser here is that everywhere where amplitudes are checked in tests for some network distributions, the tests will fail
        // because the qubits are scrambled by the optimiser, they are rearranged so the number of distributions is reduced
        // so better set it explicitly only when tests for results are checked!
		std::shared_ptr<Graphs::IOptimiser<Time>> optimiser; // = Graphs::Factory<Time>::Create(Graphs::OptimiserType::kMonteCarlo); /**< The optimiser used to optimise the circuit. */

		std::shared_ptr<Schedulers::IScheduler<Time>> scheduler; /**< The scheduler used to schedule the circuits. */

	private:
		bool optimize = true;
		bool optimizeRotationGates = true; /**< If true, the rotation gates are optimized - default, except for netqasm networks */
	};
}

#endif // !_SIMPLE_CONTROLLER_H_
