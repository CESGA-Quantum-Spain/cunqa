/*******************************************************

Copyright (C) 2023 2639731 ONTARIO INC. <joe.diadamo.dryrock@gmail.com>

The files in this repository make up the Codebase.

All files in this Codebase are owned by 2639731 ONTARIO INC..

Any files within this Codebase can not be copied and/or distributed without the express permission of 2639731 ONTARIO INC.

*******************************************************/

/**
 * @file Controller.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The controller interface.
 *
 * Must be derived from by the particular controller implementations.
 * The controller is a special kind of host, it is the one that distributes the circuit to the other hosts.
 * Does not have a quantum processor, it can only do classical computations and can clasically communicate with the hosts that do quantum computations.
 */

#pragma once

#ifndef _CONTROLLER_INTERFACE_H_
#define _CONTROLLER_INTERFACE_H_

#include "Host.h"
#include "../Simulators/Simulator.h"
#include "../Circuit/Circuit.h"

namespace Distribution {
	template<typename Time> class IRemapper;
}

namespace Graphs {
	template<typename Time> class IOptimiser;

	enum class OptimiserType;
}

namespace Schedulers {
	template<typename Time> class IScheduler;

	/**
	 * @struct ExecuteCircuit
	 * @brief A way to pack together a circuit and the number of shots for its execution.
	 */
	template<typename Time = Types::time_type> struct ExecuteCircuit {
		ExecuteCircuit() = default;

		ExecuteCircuit(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t shots)
			: circuit(circuit), shots(shots)
		{
		}

		virtual ~ExecuteCircuit() = default;

		bool operator==(const ExecuteCircuit<Time>& other) const
		{
			return circuit == other.circuit && shots == other.shots;
		}

		std::shared_ptr<Circuits::Circuit<Time>> circuit;
		size_t shots = 1000;
	};
}


namespace Network {

	/**
	 * @enum SchedulerType
	 * @brief The type of the network scheduler for scheduling execution of multiple circuits.
	 */
	enum class SchedulerType : int
	{
		kNoEntanglementQubits = 0,
		kNoEntanglementQubitsParallel
	};

	template<typename Time> class INetwork;

	/**
	 * @class IController
	 * @brief The controller host interface.
	 * 
	 * The controller is a special kind of host, it is the one that distributes the circuit to the other hosts.
	 * 
	 * @tparam Time The time representation to use for execution times.
	 * @sa IHost
	 * @sa INetwork
	 */
	template<typename Time = Types::time_type> class IController : public IHost<Time> {
	public:
		/**
		 * @brief Distributes the circuit on the hosts.
		 * 
		 * The network will call this, the conversion and distribution that should happen before execution.
		 * 
		 * @param network The network to distribute the circuit for.
		 * @param circuit The circuit to distribute.
		 * @return The distributed circuit.
		 */
		virtual std::shared_ptr<Circuits::Circuit<Time>> DistributeCircuit(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) = 0;

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
		virtual std::shared_ptr<Circuits::Circuit<Time>> SplitCompositeOperations(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) = 0;

		/**
		 * @brief Changes the remapper that is used for remapping a circuit to a distributed one.
		 * 
		 * The remapper is used to remap the circuit to a distributed one.
		 * 
		 * @param remapper The new remapper.
		 * @sa Distribution::IRemapper
		 */
		virtual void SetRemapper(const std::shared_ptr<Distribution::IRemapper<Time>>& remapper) = 0;

		/**
		 * @brief Gets the remapper that is used for remapping a circuit to a distributed one.
		 * 
		 * The remapper is used to remap the circuit to a distributed one.
		 * 
		 * @return The remapper.
		 * @sa Distribution::IRemapper
		 */
		virtual std::shared_ptr<Distribution::IRemapper<Time>> GetRemapper() const = 0;

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
		virtual std::shared_ptr<Circuits::Circuit<Time>> DoNetworkSpecificConversionsForDistribution(const std::shared_ptr<INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) = 0;

		/**
		 * @brief Returns the optimiser used
		 *
		 * Returns the optimiser used. Could be nullptr, that is, no optimiser.
		 * 
		 * @return The optimiser used.
		 */
		virtual std::shared_ptr<Graphs::IOptimiser<Time>> GetOptimiser() const = 0;

		/**
		 * @brief Creates an optimiser.
		 *
		 * Creates an optimiser of the specified type.
		 * 
		 * @param type The type of optimiser to create.
		 */
		virtual void CreateOptimiser(Graphs::OptimiserType type) = 0;

		/**
		 * @brief Create the scheduler for the network.
		 *
		 * Creates the scheduler for the network.
		 * Call this only after the network topology has been set up.
		 * Should create the scheduler and set the network for it and any other necessary parameters.
		 *
		 * @param network The network to create the scheduler for.
		 * @param simType The type of the scheduler to create.
		 * @sa SchedulerType
		 */
		virtual void CreateScheduler(const std::shared_ptr<INetwork<Time>>& network, SchedulerType schType = SchedulerType::kNoEntanglementQubitsParallel) = 0;


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
		 * @brief Set circuit optimization.
		 *
		 * If the parameter is true, the circuit will be optimized, otherwise it will not be optimized.
		 * The default value is true.
		 *
		 * @param optimize True if the circuit should be optimized, false otherwise.
		 */
		virtual void SetOptimizeCircuit(bool optimize = true) = 0;

		/**
		 * @brief Get circuit optimization.
		 *
		 * Returns true if the circuit will be optimized, false otherwise.
		 *
		 * @return True if the circuit will be optimized, false otherwise.
		 */
		virtual bool GetOptimizeCircuit() const = 0;

		virtual bool GetOptimizeRotationGates() const = 0;

		virtual void SetOptimizeRotationGates(bool val = true) = 0;
	};

}

#endif // !_CONTROLLER_INTERFACE_H_
