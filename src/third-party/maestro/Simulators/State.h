/**
 * @file State.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The State interface for a simulator.
 * 
 * This interface is used to define the state of a simulator.
 * Exposes functions to initialize, configure, allocate qubits, measure, apply reset on qubits, and obtain probabilities.
 * Also allows to register observers that will be notified when the state changes.
 * 
 * The interface could be used to pass around the state of a simulator, if operations on the state are not required.
 */

#pragma once

#ifndef _SIMULATOR_STATE_H_
#define _SIMULATOR_STATE_H_

#include <complex>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <Eigen/Eigen>

#ifndef NO_QISKIT_AER
#include "framework/linalg/vector.hpp"
#endif

#include "SimulatorObserver.h"

namespace Simulators {

	/**
	 * @class avoid_init_allocator
	 * @brief An allocator that avoids initializing the allocated memory.
	 *
	 * This allocator is used to avoid initializing the allocated memory.
	 * It is used to avoid initializing the memory when the state is initialized with a vector of amplitudes.
	 * std::vector will initialize the memory with 0, which is not needed when the values in the vector are set right after the allocation.
	 * 
	 * @sa IState::InitializeState
	 */
	/*
	template<class T> class avoid_init_allocator : public std::allocator<T>
	{
	public:
		using std::allocator<T>::allocator;

		template <class U, class... Args> void construct(U*, Args&&...) {}
	};
	*/

	/**
	 * @enum SimulatorType
	 * @brief The type of simulator.
	 */
	enum class SimulatorType : int {
#ifndef NO_QISKIT_AER
		kQiskitAer, /**< qiskit aer simulator type */
#endif
		kQCSim,         /**< qcsim simulator type */
#ifndef NO_QISKIT_AER
		kCompositeQiskitAer, /**< composite qiskit aer simulator type */
#endif
		kCompositeQCSim      /**< composite qcsim simulator type */
#ifdef __linux__
		, kGpuSim            /**< gpu simulator type */
#endif
	};


	/**
	 * @enum SimulationType
	 * @brief The type of simulation.
	 */
	enum class SimulationType : int {
		kStatevector, /**< statevector simulation type */
		kMatrixProductState, /**< matrix product state simulation type */
		kStabilizer, /**< Clifford gates simulation type */
		kTensorNetwork, /**< Tensor network simulation type */
		kOther /**< other simulation type, could occur for the aer simulator, which also has density matrix, stabilizer, extended stabilizer, unitary, superop */
	};

	/**
	 * @class IState
	 * @brief Interface class for a quantum computing simulator state.
	 *
	 * Use this interface if only the state of the simulator is required.
	 * @sa ISimulator
	 */
	class IState
	{
	public:
		/**
		 * @brief Virtual destructor.
		 *
		 * Since this is a base class, the destructor should be virtual.
		 */
		virtual ~IState() = default;

		/**
		 * @brief Initializes the state.
		 *
		 * This function is called when the simulator is initialized.
		 * Call it after the qubits allocation.
		 * @sa IState::AllocateQubits
		 */
		virtual void Initialize() = 0;


		/**
		 * @brief Initializes the state.
		 *
		 * This function is called when the simulator is initialized.
		 * Call it only on a non-initialized state.
		 * This is good only for a statevector simulator and should be used only by calling from a composite simulator.
		 * 
		 * @param num_qubits The number of qubits to initialize the state with.
		 * @param amplitudes A vector with the amplitudes to initialize the state with.
		 */
		virtual void InitializeState(size_t num_qubits, std::vector<std::complex<double>>& amplitudes) = 0;


		/**
		 * @brief Initializes the state.
		 *
		 * This function is called when the simulator is initialized.
		 * Call it only on a non-initialized state.
		 * This is good only for a statevector simulator and should be used only by calling from a composite simulator.
		 *
		 * @param num_qubits The number of qubits to initialize the state with.
		 * @param amplitudes A vector with the amplitudes to initialize the state with.
		 */
#ifndef NO_QISKIT_AER
		 virtual void InitializeState(size_t num_qubits, AER::Vector<std::complex<double>>& amplitudes) = 0;
#endif

		/**
		 * @brief Initializes the state.
		 *
		 * This function is called when the simulator is initialized.
		 * Call it only on a non-initialized state.
		 * This is good only for a statevector simulator and should be used only by calling from a composite simulator.
		 *
		 * @param num_qubits The number of qubits to initialize the state with.
		 * @param amplitudes A vector with the amplitudes to initialize the state with.
		 */
		virtual void InitializeState(size_t num_qubits, Eigen::VectorXcd& amplitudes) = 0;


		/**
		 * @brief Just resets the state to 0.
		 *
		 * Does not destroy the internal state, just resets it to zero (as a 'reset' op on each qubit would do).
		 */
		virtual void Reset() = 0;

		/**
		 * @brief Configures the state.
		 *
		 * This function is called to configure the simulator.
		 * Currently only aer supports configuration, qcsim will gracefully ignore this.
		 * @param key The key of the configuration option.
		 * @param value The value of the configuration.
		 */
		virtual void Configure(const char* key, const char* value) = 0;


		/**
		 * @brief Returns configuration value.
		 *
		 * This function is called get a configuration value.
		 * @param key The key of the configuration value.
		 * @return The configuration value as a string.
		 */
		virtual std::string GetConfiguration(const char* key) const = 0;

		/**
		 * @brief Allocates qubits.
		 *
		 * This function is called to allocate qubits.
		 * @param num_qubits The number of qubits to allocate.
		 * @return The index of the first qubit allocated.
		 */
		virtual size_t AllocateQubits(size_t num_qubits) = 0;

		/**
		 * @brief Returns the number of qubits.
		 *
		 * This function is called to obtain the number of the allocated qubits.
		 * @return The number of qubits.
		 */
		virtual size_t GetNumberOfQubits() const = 0;

		/**
		 * @brief Clears the state.
		 *
		 * Sets the number of allocated qubits to 0 and clears the state.
		 * After this qubits allocation is required then calling IState::AllocateQubits in order to use the simulator.
		 */
		virtual void Clear() = 0;

		/**
		 * @brief Performs a measurement on the specified qubits.
		 *
		 * @param qubits A vector with the qubits to be measured.
		 * @return The outcome of the measurements, the first qubit result is the least significant bit.
		 */
		virtual size_t Measure(const Types::qubits_vector& qubits) = 0;

		/**
		 * @brief Performs a reset of the specified qubits.
		 * 
		 * Measures the qubits and for those that are 1, applies X on them
		 * @param qubits A vector with the qubits to be reset.
		 */
		virtual void ApplyReset(const Types::qubits_vector& qubits) = 0;

		/**
		 * @brief Returns the probability of the specified outcome.
		 *
		 * Use it to obtain the probability to obtain the specified outcome, if all qubits are measured.
		 * @sa IState::Amplitude
		 * @sa IState::Probabilities
		 * 
		 * @param outcome The outcome to obtain the probability for.
		 * @return The probability of the specified outcome.
		 */
		virtual double Probability(Types::qubit_t outcome) = 0;

		/**
		 * @brief Returns the amplitude of the specified state.
		 *
		 * Use it to obtain the amplitude of the specified state.
		 * @sa IState::Probability
		 * @sa IState::Probabilities
		 * 
		 * @param outcome The outcome to obtain the amplitude for.
		 * @return The amplitude of the specified outcome.
		 */
		virtual std::complex<double> Amplitude(Types::qubit_t outcome) = 0;

		/**
		 * @brief Returns the probabilities of all possible outcomes.
		 *
		 * Use it to obtain the probabilities of all possible outcomes.
		 * @sa IState::Probability
		 * @sa IState::Amplitude
		 * @sa IState::Probabilities
		 * 
		 * @return A vector with the probabilities of all possible outcomes.
		 */
		virtual std::vector<double> AllProbabilities() = 0;

		/**
		 * @brief Returns the probabilities of the specified outcomes.
		 *
		 * Use it to obtain the probabilities of the specified outcomes.
		 * @sa IState::Probability
		 * @sa IState::Amplitude
		 * 
		 * @param qubits A vector with the qubits configuration outcomes.
		 * @return A vector with the probabilities for the specified qubit configurations.
		 */
		virtual std::vector<double> Probabilities(const Types::qubits_vector& qubits) = 0;

		/**
		 * @brief Returns the counts of the outcomes of measurement of the specified qubits, for repeated measurements.
		 *
		 * Use it to obtain the counts of the outcomes of the specified qubits measurements.
		 * The state is not collapsed, so the measurement can be repeated 'shots' times.
		 * 
		 * @param qubits A vector with the qubits to be measured.
		 * @param shots The number of shots to perform.
		 * @return A map with the counts for the otcomes of measurements of the specified qubits.
		 */
		virtual std::unordered_map<Types::qubit_t, Types::qubit_t> SampleCounts(const Types::qubits_vector& qubits, size_t shots = 1000) = 0;

		/**
		 * @brief Returns the expected value of a Pauli string.
		 *
		 * Use it to obtain the expected value of a Pauli string.
		 * The Pauli string is a string of characters representing the Pauli operators, e.g. "XIZY".
		 * The length of the string should be less or equal to the number of qubits (if it's less, it's completed with I).
		 * 
		 * @param pauliString The Pauli string to obtain the expected value for.
		 * @return The expected value of the specified Pauli string.
		 */
		virtual double ExpectationValue(const std::string& pauliString) = 0;

		/**
		 * @brief Registers an observer.
		 *
		 * Registers an observer that will be notified when the state changes.
		 * @sa ISimulatorObserver
		 * 
		 * @param observer A smart pointer to an observer.
		 */
		void RegisterObserver(const std::shared_ptr<ISimulatorObserver>& observer)
		{
			observers.insert(observer);
		}

		/**
		* @brief Unregisters an observer.
		*
		* Unegisters an observer.
		* @sa ISimulatorObserver
		*
		* @param observer A smart pointer to an observer.
		*/
		void UnregisterObserver(const std::shared_ptr<ISimulatorObserver>& observer)
		{
			observers.erase(observer);
		}

		/**
		* @brief Clears all observers.
		* 
		* Clears all observers.
		*/
		void ClearObservers()
		{
			observers.clear();
		}

		/**
		* @brief Returns the type of simulator.
		* 
		* Returns the type of simulator.
		* 
		* @return The type of simulator.
		* @sa SimulatorType
		*/
		virtual SimulatorType GetType() const = 0;

		/**
		* @brief Returns the type of simulation.
		* 
		* Returns the type of simulation.
		* 
		* @return The type of simulation.
		* @sa SimulationType
		*/
		virtual SimulationType GetSimulationType() const = 0;

		/**
		 * @brief Flushes the applied operations
		 *
		 * This function is called to flush the applied operations.
		 * qcsim applies them right away, so this has no effect on it, but qiskit aer does not.
		 */
		virtual void Flush() = 0;

		/**
		 * @brief Saves the state to internal storage.
		 *
		 * Saves the state to internal storage, if needed.
		 * Calling this should consider as the simulator is gone to uninitialized.
		 * Either do not use it except for getting amplitudes, or reinitialize the simulator after calling it.
		 * This is needed only for the composite simulator, for an optimization for qiskit aer.
		 */
		virtual void SaveStateToInternalDestructive() = 0;

		/**
		 * @brief Restores the state from the internally saved state
		 *
		 * Restores the state from the internally saved state, if needed.
		 * This does something only for qiskit aer.
		 */
		virtual void RestoreInternalDestructiveSavedState() = 0;


		/**
		 * @brief Saves the state to internal storage.
		 *
		 * Saves the state to internal storage, if needed.
		 * Calling this will not destroy the internal state, unlike the 'Destructive' variant.
		 * To be used in order to recover the state after doing measurements, for multiple shots executions.
		 * In the first phase, only qcsim will implement this.
		 */
		virtual void SaveState() = 0;

		/**
		 * @brief Restores the state from the internally saved state
		 *
		 * Restores the state from the internally saved state, if needed.
		 * To be used in order to recover the state after doing measurements, for multiple shots executions.
		 * In the first phase, only qcsim will implement this.
		 */
		virtual void RestoreState() = 0;

		/**
		 * @brief Gets the amplitude.
		 *
		 * Gets the amplitude, from the internal storage if needed.
		 * This is needed only for the composite simulator, for an optimization for qiskit aer.
		 */
		virtual std::complex<double> AmplitudeRaw(Types::qubit_t outcome) = 0;

		/**
		 * @brief Enable/disable multithreading.
		 * 
		 * Enable/disable multithreading. Default is enabled.
		 * 
		 * @param multithreading A flag to indicate if multithreading should be enabled.
		 */
		virtual void SetMultithreading(bool multithreading = true) = 0;

		/**
		 * @brief Get the multithreading flag.
		 * 
		 * Returns the multithreading flag.
		 * 
		 * @return The multithreading flag.
		 */
		virtual bool GetMultithreading() const = 0;

		/**
		 * @brief Returns if the simulator is a qcsim simulator.
		 *
		 * Returns if the simulator is a qcsim simulator.
		 * This is just a helper function to ease things up: qcsim has different functionality exposed sometimes so it's good to know if we deal with qcsim or with qiskit aer.
		 *
		 * @return True if the simulator is a qcsim simulator, false otherwise.
		 */
		virtual bool IsQcsim() const = 0;

		/**
		 * @brief Measures all the qubits without collapsing the state.
		 * 
		 * Measures all the qubits without collapsing the state, allowing to perform multiple shots.
		 * This is to be used only internally, only for the statevector simulators (or those based on them, as the composite ones).
		 * For the qiskit aer case, SaveStateToInternalDestructive is needed to be called before this.
		 * If one wants to use the simulator after such measurement(s), RestoreInternalDestructiveSavedState should be called at the end.
		 * 
		 * @return The result of the measurements, the first qubit result is the least significant bit.
		 */
		virtual Types::qubit_t MeasureNoCollapse() = 0;

	protected:
		/**
		* @brief Stops notifying observers.
		* 
		* Use it to stop notifying observers until Notify is called.
		*/
		void DontNotify()
		{
			notifyObservers = false;
		}

		/**
		* @brief Starts notifying observers.
		* 
		* Use it to allow notifying observers.
		*/
		void Notify()
		{
			notifyObservers = true;
		}

		/**
		* @brief Notifies observers.
		* 
		* Called when the state changes, to notify observers about it.
		* @param affectedQubits A vector with the qubits that were affected by the change.
		*/
		void NotifyObservers(const Types::qubits_vector& affectedQubits)
		{
			if (!notifyObservers) return;

			for (auto& observer : observers)
			{
				observer->Update(affectedQubits);
			}
		}

	private:
		std::unordered_set<std::shared_ptr<ISimulatorObserver>> observers; /**< The registered observers. */
		bool notifyObservers = true;                                       /**< A flag to indicate if observers should be notified. */
	};

}

#endif // !_SIMULATOR_STATE_H_



