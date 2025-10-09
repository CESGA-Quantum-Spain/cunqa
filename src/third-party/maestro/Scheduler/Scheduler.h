/**
 * @file Scheduler.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The scheduler interface.
 *
 * Must be derived from by the particular scheduler implementations.
 */

#pragma once

#ifndef _SCHEDULER_INTERFACE_H_
#define _SCHEDULER_INTERFACE_H_

#include "../Network/Network.h"
#include "../Utils/ThreadsPool.h"


namespace Schedulers {

	/**
	 * @struct ExecuteCircuit
	 * @brief A struct representing a circuit to be executed along with the number of shots.
	 * 
	 * A struct representing a circuit to be executed along with the number of shots.
	 * 
	 * @tparam Time The time type to use for the circuit.
	 * @sa Circuits::Circuit
	 */
	template<typename Time = Types::time_type> struct CircuitInfo : ExecuteCircuit<Time>
	{
		using BitMapping = typename Circuits::Circuit<Time>::BitMapping;

		CircuitInfo() = default;

		CircuitInfo(const std::shared_ptr<Circuits::Circuit<Time>>& circuit, size_t shots, size_t id, size_t gatesDepth, Time timeDepth, size_t qubits)
			: ExecuteCircuit<Time>(circuit, shots), id(id), gatesDepth(gatesDepth), timeDepth(timeDepth), qubits(qubits)
		{
		}

		bool operator==(const CircuitInfo<Time>& other) const
		{
			if (ExecuteCircuit<Time>::operator==(other))
			{
				return id == other.id && gatesDepth == other.gatesDepth && timeDepth == other.timeDepth && qubits == other.qubits && hostId == other.hostId;
			}

			return false;
		}

		size_t id = 0;
		size_t gatesDepth = 0;
		Time timeDepth = 0;
		size_t qubits = 0;
		BitMapping bitMapping;

		// those are redundant, but I set them anyway for convenience and easier usage in python
		size_t hostId = 0;
		size_t startQubits = 0;
	};


	/**
	 * @struct ExecuteJob
	 * @brief A struct representing a job to be executed on a host.
	 * 
	 * A struct representing a job to be executed on a host, containing both the circuit and the number of shots information, also a shared pointer to the network to execute on.
	 * Calling DoWork() will execute the job on the host network and store the results in the results member.
	 * 
	 * @tparam Time The time type to use for the network and circuit.
	 * @sa Network::INetwork
	 * @sa Circuits::Circuit
	 */
	template<typename Time = Types::time_type> struct ExecuteJob
	{
		using NetworkClass = typename Network::INetwork<Time>;
		using ExecuteResults = typename NetworkClass::ExecuteResults;

		std::shared_ptr<NetworkClass> hostNetwork;
		size_t h = 0;
		std::shared_ptr<Circuits::Circuit<Time>> circ;
		size_t nrShots = 0;
		ExecuteResults results;

		void DoWork()
		{
			if (!hostNetwork || !circ || nrShots == 0) return;

			results = hostNetwork->RepeatedExecuteOnHost(circ, h, nrShots);
		}

		size_t GetJobCount() const
		{
			return 1;
		}
	};
	
	/**
	 * @class IScheduler
	 * @brief The scheduler interface.
	 * 
	 * The scheduler interface. Must be derived from by the particular scheduler implementations.
	 * 
	 * @tparam Time The time type to use for the scheduler.
	 * @sa Network::INetwork
	 */
	template<typename Time = Types::time_type> class IScheduler : public std::enable_shared_from_this<IScheduler<Time>>
	{
	public:
		using NetworkClass = typename Network::INetwork<Time>;
		using ExecuteResults = typename NetworkClass::ExecuteResults;

		virtual ~IScheduler() = default;
	
		virtual void SetNetwork(const std::shared_ptr<NetworkClass>& n)
		{
			network = n;
		}

		std::shared_ptr<NetworkClass> GetNetwork() const
		{
			return network;
		}

		/**
		 * @brief Schedule and execute circuits on the network.
		 *
		 * Execute the circuits on the network, scheduling their execution and distributing the operations to the hosts.
		 * The way the circuits are distributed to the hosts depends on the specific interface implementations.
		 * The way they are scheduled depends on the network scheduler and parametrization.
		 *
		 * @param circuits The circuits to execute, along with the number of shots.
		 * @return A vector of maps with the results of each circuit execution, where the key is the qubit id and the value is the number of times the qubit was measured to be 1.
		 * @sa Circuits::Circuit
		 * @sa ExecuteCircuit
		 * @sa Network::INetwork
		 */
		virtual std::vector<ExecuteResults> ExecuteScheduled(const std::vector<ExecuteCircuit<Time>>& circuits) = 0;

		virtual std::shared_ptr<Circuits::Circuit<Time>> GetScheduledCircuit() const = 0;

		virtual std::vector<ExecuteResults> ExecuteScheduledSteps() = 0;


		virtual const std::vector<std::vector<CircuitInfo<Time>>>& GetScheduledSteps() const = 0;

		virtual void SetScheduledSteps(const std::vector<std::vector<CircuitInfo<Time>>>& steps) = 0;

		virtual bool SetCurrentStep(size_t step) = 0;

		virtual std::shared_ptr<Circuits::Circuit<Time>> GetScheduledCircuitForHost(size_t hostId) const = 0;

		virtual size_t GetCurrentShots() const = 0;

		virtual void ExecuteCurrentStep() = 0;

		virtual void SetDepthEstimationMethod(bool useSteps = true) = 0;

		virtual bool GetDepthEstimationMethod() const = 0;

		virtual void SetUseCost(bool useCost = true) = 0;

		virtual bool GetUseCost() const = 0;

		virtual void SetAddResetsAtEnd(bool addResets = true) = 0;

		virtual bool GetAddResetsAtEnd() const = 0;

		virtual void SetAddResetsAtStart(bool addResets = true) = 0;

		virtual bool GetAddResetsAtStart() const = 0;

		virtual std::vector<ExecuteResults> GetResults() const = 0;

		virtual void CollectResults(const ExecuteResults& res, size_t nrShots) = 0;

		virtual void CollectResultsForHost(const ExecuteResults& res, size_t hostId, size_t nrShots) = 0;

		virtual Network::SchedulerType GetType() const = 0;

	private:
		std::shared_ptr<NetworkClass> network; /**< The network to schedule and execute circuits on. */
	};
}

#endif
