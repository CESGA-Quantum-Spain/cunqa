/**
 * @file SimulatorObserver.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Simulator observer interface and proxy.
 * 
 * Use them for observers of a simulator, if they are needed.
 */

#pragma once

#ifndef  _SIMULATOR_OBSERVER_H_
#define  _SIMULATOR_OBSERVER_H_

#include <memory>
#include <vector>

#include "../Types.h"

namespace Simulators {

	/**
	 * @class ISimulatorObserver
	 * @brief Interface class for a quantum computing simulator observer.
	 *
	 * Derive from this class to implement a simulator observer.
	 * @sa SimulatorObserverProxy
	 * @sa IState
	 * @sa ISimulator
	 */
	// either derive from this or use the following proxy class (advantage for that: no need to derive the Impl class from the interface, just implement 'Update')
	class ISimulatorObserver : public std::enable_shared_from_this<ISimulatorObserver>
	{
	public:
		/**
		 * @brief Virtual destructor.
		 *
		 * Since this is a base class, the destructor should be virtual.
		 */
		virtual ~ISimulatorObserver() = default;

		/**
		 * @brief Update function that is called each time an update is done.
		 *
		 * This function is called each time the state is changed, with the qubits that have been changed.
		 * @param qubits The qubits that have been changed.
		 */
		virtual void Update(const Types::qubits_vector& qubits) = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<ISimulatorObserver> getptr()
		{
			return shared_from_this();
		}
	};

	/**
	 * @class SimulatorObserverProxy
	 * @brief A proxy class for a quantum computing simulator observer.
	 *
	 * If you don't want to derive from ISimulatorObserver, use this proxy class.
	 * The implementation of the observer needs to implement the Update function.
	 * @sa ISimulatorObserver
	 * @sa IState
	 * @sa ISimulator
	 * 
	 * @tparam Impl The implementation of the observer.
	 */
	template<class Impl> class SimulatorObserverProxy : public ISimulatorObserver
	{
	public:
		/**
		 * @brief Constructor.
		 *
		 * Constructs a proxy for the observer.
		 * @param impl The implementation of the observer, wrapped in a shared pointer.
		 */
		SimulatorObserverProxy(const std::shared_ptr<Impl>& impl) : impl(impl) {}

		/**
		 * @brief Update function that is called each time an update is done.
		 *
		 * This function is called each time the state is changed, with the qubits that have been changed.
		 * @param qubits The qubits that have been changed.
		 */
		void Update(const Types::qubits_vector& qubits) override
		{
			if (impl) impl->Update(qubits);
		}

	private:
		std::shared_ptr<Impl> impl; /**< A shared pointer to the implementation of the observer */
	};

}

#endif // _SIMULATOR_OBSERVER_H_


