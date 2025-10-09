/**
 * @file GpuState.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The gpu state class.
 *
 * Should not be used directly, create an instance with the factory and use the generic simulator interface.
 */

#pragma once

#ifndef _GPUSTATE_H_
#define _GPUSTATE_H_

#ifdef __linux__

#ifdef INCLUDED_BY_FACTORY

namespace Simulators {
	// TODO: Maybe use the pimpl idiom https://en.cppreference.com/w/cpp/language/pimpl to hide the implementation for good
	// but during development this should be good enough
	namespace Private {

		/**
		 * @class GpuState
		 * @brief Class for the gpu simulator state.
		 *
		 * Implements the gpu state.
		 * Do not use this class directly, use the factory to create an instance.
		 * @sa ISimulator
		 * @sa IState
		 * @sa GpuSimulator
		 */
		class GpuState : public ISimulator
		{
		public:
			/**
			 * @brief Initializes the state.
			 *
			 * This function is called when the simulator is initialized.
			 * Call it after the qubits allocation.
			 * @sa GpuState::AllocateQubits
			 */
			void Initialize() override
			{
				if (nrQubits) {
					if (simulationType == SimulationType::kStatevector)
					{
						state = SimulatorsFactory::CreateGpuLibStateVectorSim();
						if (state) {
							const bool res = state->Create(nrQubits);
							if (!res) throw std::runtime_error("GpuState::Initialize: Failed to create and initialize the statevector state.");
						}
						else throw std::runtime_error("GpuState::Initialize: Failed to create the statevector state.");
					}
					else if (simulationType == SimulationType::kMatrixProductState)
					{
						mps = SimulatorsFactory::CreateGpuLibMPSSim();
						if (mps) {
							if (limitEntanglement && singularValueThreshold > 0.)
								mps->SetCutoff(singularValueThreshold);
							if (limitSize && chi > 0)
								mps->SetMaxExtent(chi);
							const bool res = mps->Create(nrQubits);
							if (!res) throw std::runtime_error("GpuState::Initialize: Failed to create and initialize the MPS state.");
						}
						else throw std::runtime_error("GpuState::Initialize: Failed to create the MPS state.");
					}
					else throw std::runtime_error("GpuState::Initialize: Invalid simulation type for initializing the state.");
				}
			}

			/**
			 * @brief Initializes the state.
			 *
			 * This function is called when the simulator is initialized.
			 * Call it only on a non-initialized state.
			 * This works only for 'statevector' method.
			 *
			 * @param num_qubits The number of qubits to initialize the state with.
			 * @param amplitudes A vector with the amplitudes to initialize the state with.
			 */
			void InitializeState(size_t num_qubits, std::vector<std::complex<double>>& amplitudes) override
			{
				if (num_qubits == 0) return;
				Clear();
				nrQubits = num_qubits;
				Initialize();

				if (simulationType != SimulationType::kStatevector) throw std::runtime_error("GpuState::InitializeState: Invalid simulation type for initializing the state.");
				
				if (nrQubits) {
					if (simulationType == SimulationType::kStatevector)
					{
						state = SimulatorsFactory::CreateGpuLibStateVectorSim();
						if (state) state->CreateWithState(nrQubits, reinterpret_cast<const double*>(amplitudes.data()));
					}
				}
			}

			 /**
			  * @brief Initializes the state.
			  *
			  * This function is called when the simulator is initialized.
			  * Call it only on a non-initialized state.
			  * This works only for 'statevector' method.
			  *
			  * @param num_qubits The number of qubits to initialize the state with.
			  * @param amplitudes A vector with the amplitudes to initialize the state with.
			  */
#ifndef NO_QISKIT_AER
			void InitializeState(size_t num_qubits, AER::Vector<std::complex<double>>& amplitudes) override
			{
				if (num_qubits == 0) return;
				Clear();
				nrQubits = num_qubits;
				Initialize();

				if (simulationType != SimulationType::kStatevector) throw std::runtime_error("GpuState::InitializeState: Invalid simulation type for initializing the state.");

				if (nrQubits) {
					if (simulationType == SimulationType::kStatevector)
					{
						state = SimulatorsFactory::CreateGpuLibStateVectorSim();
						if (state) state->CreateWithState(nrQubits, reinterpret_cast<const double*>(amplitudes.data()));
					}
				}
			}
#endif

			/**
			 * @brief Initializes the state.
			 *
			 * This function is called when the simulator is initialized.
			 * Call it only on a non-initialized state.
			 * This works only for 'statevector' method.
			 *
			 * @param num_qubits The number of qubits to initialize the state with.
			 * @param amplitudes A vector with the amplitudes to initialize the state with.
			 */
			void InitializeState(size_t num_qubits, Eigen::VectorXcd& amplitudes) override
			{
				if (num_qubits == 0) return;
				Clear();
				nrQubits = num_qubits;
				Initialize();

				if (simulationType != SimulationType::kStatevector) throw std::runtime_error("GpuState::InitializeState: Invalid simulation type for initializing the state.");

				if (nrQubits) {
					if (simulationType == SimulationType::kStatevector)
					{
						state = SimulatorsFactory::CreateGpuLibStateVectorSim();
						if (state) state->CreateWithState(nrQubits, reinterpret_cast<const double*>(amplitudes.data()));
					}
				}
			}

			/**
			 * @brief Just resets the state to 0.
			 *
			 * Does not destroy the internal state, just resets it to zero (as a 'reset' op on each qubit would do).
			 */
			void Reset() override
			{
				if (state) state->Reset();
				else if (mps) mps->Reset();
			}

			/**
             * @brief Configures the state.
             *
             * This function is called to configure the simulator.
             *
             * @param key The key of the configuration option.
             * @param value The value of the configuration.
             */
			void Configure(const char* key, const char* value) override
			{
				if (std::string("method") == key)
				{
					if (std::string("statevector") == value)
						simulationType = SimulationType::kStatevector;
					else if (std::string("matrix_product_state") == value)
						simulationType = SimulationType::kMatrixProductState;
				}
				else if (std::string("matrix_product_state_truncation_threshold") == key)
				{
					singularValueThreshold = std::stod(value);
					if (singularValueThreshold > 0.)
					{
						limitEntanglement = true;
						if (mps) mps->SetCutoff(singularValueThreshold);
					}
					else limitEntanglement = false;
				}
				else if (std::string("matrix_product_state_max_bond_dimension") == key)
				{
					chi = std::stoi(value);
					if (chi > 0)
					{
						limitSize = true;
						if (mps) mps->SetMaxExtent(chi);
					}
					else limitSize = false;
				}
			}


			/**
			 * @brief Returns configuration value.
			 *
			 * This function is called get a configuration value.
			 * @param key The key of the configuration value.
			 * @return The configuration value as a string.
			 */
			std::string GetConfiguration(const char* key) const override
			{
				if (std::string("method") == key)
				{
					switch (simulationType) {
					case SimulationType::kStatevector:
						return "statevector";
					case SimulationType::kMatrixProductState:
						return "matrix_product_state";
					default:
						return "other";
					}
				}
				else if (std::string("matrix_product_state_truncation_threshold") == key)
				{
					if (limitEntanglement && singularValueThreshold > 0.)
						return std::to_string(singularValueThreshold);
				}
				else if (std::string("matrix_product_state_max_bond_dimension") == key)
				{
					if (limitSize && limitSize > 0)
						return std::to_string(chi);
				}

				return "";
			}

			/**
			 * @brief Allocates qubits.
			 *
			 * This function is called to allocate qubits.
			 * @param num_qubits The number of qubits to allocate.
			 * @return The index of the first qubit allocated.
			 */
			size_t AllocateQubits(size_t num_qubits) override
			{
				if ((simulationType == SimulationType::kStatevector && state) || (simulationType == SimulationType::kMatrixProductState && mps))
					return 0;

				const size_t oldNrQubits = nrQubits;
				nrQubits += num_qubits;
				return oldNrQubits;
			}

			/**
			 * @brief Returns the number of qubits.
			 *
			 * This function is called to obtain the number of the allocated qubits.
			 * @return The number of qubits.
			 */
			size_t GetNumberOfQubits() const override
			{
				return nrQubits;
			}

			/**
			 * @brief Clears the state.
			 *
			 * Sets the number of allocated qubits to 0 and clears the state.
			 * After this qubits allocation is required then calling IState::AllocateQubits in order to use the simulator.
			 */
			void Clear() override
			{
				state = nullptr;
				mps = nullptr;
				nrQubits = 0;
			}

			/**
			 * @brief Performs a measurement on the specified qubits.
			 *
			 * @param qubits A vector with the qubits to be measured.
			 * @return The outcome of the measurements, the first qubit result is the least significant bit.
			 */
			size_t Measure(const Types::qubits_vector& qubits) override
			{
				// TODO: this is inefficient, maybe implement it better in gpu sim
				// for now it has the possibility of measuring a qubits interval, but not a list of qubits

				size_t res = 0;
				size_t mask = 1ULL;

				DontNotify();
				if (simulationType == SimulationType::kStatevector)
				{
					// TODO: measure all qubits in one shot?
					for (size_t qubit : qubits)
					{
						if (state->MeasureQubitCollapse(static_cast<int>(qubit)))
							res |= mask;
						mask <<= 1;
					}
				}
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					// TODO: measure all qubits in one shot?
					for (size_t qubit : qubits)
					{
						if (mps->Measure(static_cast<unsigned int>(qubit)))
							res |= mask;
						mask <<= 1;
					}
				}

				Notify();
				NotifyObservers(qubits);

				return res;
			}

			/**
			 * @brief Performs a reset of the specified qubits.
			 *
			 * Measures the qubits and for those that are 1, applies X on them
			 * @param qubits A vector with the qubits to be reset.
			 */
			void ApplyReset(const Types::qubits_vector& qubits) override
			{
				DontNotify();
				if (simulationType == SimulationType::kStatevector)
				{
					for (size_t qubit : qubits)
						if (state->MeasureQubitCollapse(static_cast<int>(qubit)))
							state->ApplyX(static_cast<int>(qubit));
				}
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					for (size_t qubit : qubits)
						if (mps->Measure(static_cast<unsigned int>(qubit)))
							mps->ApplyX(static_cast<unsigned int>(qubit));
				}
				
				Notify();
				NotifyObservers(qubits);
			}

			/**
             * @brief Returns the probability of the specified outcome.
             *
             * Use it to obtain the probability to obtain the specified outcome, if all qubits are measured.
             * @sa GpuState::Amplitude
             * @sa GpuState::Probabilities
             *
             * @param outcome The outcome to obtain the probability for.
             * @return The probability of the specified outcome.
             */
			double Probability(Types::qubit_t outcome) override
			{
				if (simulationType == SimulationType::kStatevector)
					return state->BasisStateProbability(outcome);
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					const auto ampl = Amplitude(outcome);
					return std::norm(ampl);
				}

				return 0.0;
			}

			/**
			 * @brief Returns the amplitude of the specified state.
			 *
			 * Use it to obtain the amplitude of the specified state.
			 * @sa GpuState::Probability
			 * @sa GpuState::Probabilities
			 *
			 * @param outcome The outcome to obtain the amplitude for.
			 * @return The amplitude of the specified outcome.
			 */
			std::complex<double> Amplitude(Types::qubit_t outcome) override
			{
				double real = 0.0;
				double imag = 0.0;

				if (simulationType == SimulationType::kStatevector)
					state->Amplitude(outcome, &real, &imag);
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					std::vector<long int> fixedValues(nrQubits);
					for (size_t i = 0; i < nrQubits; ++i)
						fixedValues[i] = (outcome & (1ULL << i)) ? 1 : 0;
					mps->Amplitude(nrQubits, fixedValues.data(), &real, &imag);
				}

				return std::complex<double>(real, imag);
			}

			/**
			 * @brief Returns the probabilities of all possible outcomes.
			 *
			 * Use it to obtain the probabilities of all possible outcomes.
			 * @sa Gputate::Probability
			 * @sa GpuState::Amplitude
			 * @sa GpuState::AllProbabilities
			 *
			 * @return A vector with the probabilities of all possible outcomes.
			 */
			std::vector<double> AllProbabilities() override
			{
				if (nrQubits == 0) return {};
				const size_t numStates = 1ULL << nrQubits;
				std::vector<double> result(numStates);

				if (simulationType == SimulationType::kStatevector)
					state->AllProbabilities(result.data());
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					// this is very slow, it should be used only for tests!
					for (Types::qubit_t i = 0; i < (Types::qubit_t)numStates; ++i)
					{
						const auto val = Amplitude(i);
						result[i] = std::norm(std::complex<double>(val.real(), val.imag()));
					}
				}

				return result;
			}

			/**
			 * @brief Returns the probabilities of the specified outcomes.
			 *
			 * Use it to obtain the probabilities of the specified outcomes.
			 * @sa GpuState::Probability
			 * @sa GpuState::Amplitude
			 *
			 * @param qubits A vector with the qubits configuration outcomes.
			 * @return A vector with the probabilities for the specified qubit configurations.
			 */
			std::vector<double> Probabilities(const Types::qubits_vector& qubits) override
			{
				std::vector<double> result(qubits.size());

				if (simulationType == SimulationType::kStatevector)
				{
					for (size_t i = 0; i < qubits.size(); ++i)
						result[i] = state->BasisStateProbability(qubits[i]);
				}
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					for (size_t i = 0; i < qubits.size(); ++i)
					{
						const auto ampl = Amplitude(qubits[i]);
						result[i] = std::norm(ampl);
					}
				}

				return result;
			}


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
			std::unordered_map<Types::qubit_t, Types::qubit_t> SampleCounts(const Types::qubits_vector& qubits, size_t shots = 1000) override
			{
				if (qubits.empty() || shots == 0) return {};

				std::unordered_map<Types::qubit_t, Types::qubit_t> result;

				DontNotify();

				if (simulationType == SimulationType::kStatevector)
				{
					std::vector<long int> samples(shots);
					state->SampleAll(shots, samples.data());

					for (auto outcome : samples)
						++result[outcome];
				}
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					std::unordered_map<std::vector<bool>, int64_t>* map = mps->GetMapForSample();

					std::vector<unsigned int> qubitsIndices(qubits.begin(), qubits.end());

					mps->Sample(shots, qubitsIndices.size(), qubitsIndices.data(), map);

					// put the results in the result map
					for (const auto& [meas, cnt] : *map)
					{
						Types::qubit_t outcome = 0;
						Types::qubit_t mask = 1ULL;
						for (Types::qubit_t q = 0; q < qubits.size(); ++q)
						{
							if (meas[q]) outcome |= mask;
							mask <<= 1;
						}
						
						result[outcome] += cnt;
					}

					mps->FreeMapForSample(map);
				}

				Notify();
				NotifyObservers(qubits);

				return result;
			}


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
			double ExpectationValue(const std::string& pauliString) override
			{
				double result = 0.0;

				if (simulationType == SimulationType::kStatevector)
					result = state->ExpectationValue(pauliString);
				else if (simulationType == SimulationType::kMatrixProductState)
					result = mps->ExpectationValue(pauliString);

				return result;
			}

			/**
			 * @brief Returns the type of simulator.
			 *
			 * Returns the type of simulator.
			 * @return The type of simulator.
			 * @sa SimulatorType
			 */
			SimulatorType GetType() const override
			{
				return SimulatorType::kGpuSim;
			}

			/**
			 * @brief Returns the type of simulation.
			 *
			 * Returns the type of simulation.
			 *
			 * @return The type of simulation.
			 * @sa SimulationType
			 */
			SimulationType GetSimulationType() const override
			{
				return simulationType;
			}

			/**
			 * @brief Flushes the applied operations
			 *
			 * This function is called to flush the applied operations.
			 * It is used to flush the operations that were applied to the state.
			 * the gpu simulator applies them right away, so this has no effect on it, but qiskit aer does not.
			 */
			void Flush() override
			{
			}

			/**
			 * @brief Saves the state to internal storage.
			 *
			 * Saves the state to internal storage, if needed.
			 * Calling this should consider as the simulator is gone to uninitialized.
			 * Either do not use it except for getting amplitudes, or reinitialize the simulator after calling it.
			 * This is needed only for the composite simulator, for an optimization for qiskit aer.
			 * For the others it does nothing.
			 */
			void SaveStateToInternalDestructive() override
			{
				if (simulationType == SimulationType::kStatevector)
					state->SaveStateDestructive();
				else throw std::runtime_error("GpuState::SaveStateToInternalDestructive: Invalid simulation type for saving the state destructively.");
			}

			/**
			 * @brief Restores the state from the internally saved state
			 *
			 * Restores the state from the internally saved state, if needed.
			 * This does something only for qiskit aer.
			 */
			void RestoreInternalDestructiveSavedState() override
			{
				if (simulationType == SimulationType::kStatevector)
					state->RestoreStateFreeSaved();
				else throw std::runtime_error("GpuState::RestoreInternalDestructiveSavedState: Invalid simulation type for restoring the state destructively.");
			}

			/**
			 * @brief Saves the state to internal storage.
			 *
			 * Saves the state to internal storage, if needed.
			 * Calling this will not destroy the internal state, unlike the 'Destructive' variant.
			 * To be used in order to recover the state after doing measurements, for multiple shots executions.
			 */
			void SaveState() override
			{
				if (simulationType == SimulationType::kStatevector)
					state->SaveState();
				else if (simulationType == SimulationType::kMatrixProductState)
					mps->SaveState();
			}

			/**
			 * @brief Restores the state from the internally saved state
			 *
			 * Restores the state from the internally saved state, if needed.
			 * To be used in order to recover the state after doing measurements, for multiple shots executions.
			 */
			void RestoreState() override
			{
				if (simulationType == SimulationType::kStatevector)
					state->RestoreStateNoFreeSaved();
				else if (simulationType == SimulationType::kMatrixProductState)
					mps->RestoreState();
			}

			/**
			 * @brief Gets the amplitude.
			 *
			 * Gets the amplitude, from the internal storage if needed.
			 * This is needed only for the composite simulator, for an optimization for qiskit aer.
			 * For qcsim and gpu sim it does the same thing as Amplitude.
			 */
			std::complex<double> AmplitudeRaw(Types::qubit_t outcome) override
			{
				return Amplitude(outcome);
			}

			/**
			 * @brief Enable/disable multithreading.
			 *
			 * Enable/disable multithreading. Default is enabled.
			 *
			 * @param multithreading A flag to indicate if multithreading should be enabled.
			 */
			void SetMultithreading(bool multithreading = true) override
			{
				// don't do anything here, the multithreading is always enabled
			}

			/**
			 * @brief Get the multithreading flag.
			 *
			 * Returns the multithreading flag.
			 *
			 * @return The multithreading flag.
			 */
			bool GetMultithreading() const override
			{
				return true;
			}

			/**
			 * @brief Returns if the simulator is a qcsim simulator.
			 *
			 * Returns if the simulator is a qcsim simulator.
			 * This is just a helper function to ease things up: qcsim has different functionality exposed sometimes so it's good to know if we deal with qcsim or with qiskit aer.
			 *
			 * @return True if the simulator is a qcsim simulator, false otherwise.
			 */
			bool IsQcsim() const override
			{
				return false;
			}

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
			Types::qubit_t MeasureNoCollapse() override
			{
				if (simulationType == SimulationType::kStatevector)
					return state->MeasureAllQubitsNoCollapse();
				else if (simulationType == SimulationType::kMatrixProductState)
				{
					Types::qubits_vector fixedValues(nrQubits);
					std::iota(fixedValues.begin(), fixedValues.end(), 0);
					const auto res = SampleCounts(fixedValues, 1);
					if (res.empty()) return 0;
					return res.begin()->first; // return the first outcome, as it is the only one
				}
				
				throw std::runtime_error("QCSimState::MeasureNoCollapse: Invalid simulation type for measuring all the qubits without collapsing the state.");

				return 0;
			}

		protected:
			SimulationType simulationType = SimulationType::kStatevector; /**< The simulation type. */

			std::unique_ptr<GpuLibStateVectorSim> state; /**< The gpu statevector simulator. */
			std::unique_ptr<GpuLibMPSSim> mps; /**< The gpu MPS simulator. */

			size_t nrQubits = 0;                        /**< The number of allocated qubits. */
			bool limitSize = false;
			bool limitEntanglement = false;
			Eigen::Index chi = 10; // if limitSize is true
			double singularValueThreshold = 0.; // if limitEntanglement is true
		};

	}
}

#endif
#endif
#endif
