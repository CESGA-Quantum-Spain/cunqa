/**
 * @file Individual.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The individual simulator class.
 * This is not to be used alone, it's a part of the CompositeSimulator class implementation.
 */

#ifndef _INDIVIDUAL_H
#define _INDIVIDUAL_H

#ifdef INCLUDED_BY_FACTORY

#include <unordered_map>
#include "Factory.h"
#include "../Utils/Alias.h"

namespace Simulators {

	// TODO: Maybe use the pimpl idiom https://en.cppreference.com/w/cpp/language/pimpl to hide the implementation for good
	// but during development this should be good enough
	namespace Private {

		class CompositeSimulator;

		/**
		 * @class IndividualSimulator
		 * @brief The individual simulator class.
		 *
		 * The individual simulator class.
		 * This is not to be used alone, it's a part of the CompositeSimulator class implementation.
		 */
		class IndividualSimulator : public ISimulator
		{
			friend class CompositeSimulator;
		public:
			/**
			 * @brief Constructs a simulator.
			 *
			 * Constructs a simulator.
			 * @param type The type of the simulator to create.
			 */
			IndividualSimulator(SimulatorType type = 
#ifdef NO_QISKIT_AER
				Simulators::SimulatorType::kQCSim
#else
				Simulators::SimulatorType::kQiskitAer
#endif			
			) noexcept
				: simulator(SimulatorsFactory::CreateSimulatorUnique(type, Simulators::SimulationType::kStatevector))
			{
			}

			/**
			 * @brief Just resets the state to 0.
			 *
			 * Does not destroy the internal state, just resets it to zero (as a 'reset' op on each qubit would do).
			 * Don't call it, it's here just to satisfy the interface.
			 */
			void Reset() override
			{
				simulator->Reset();
			}

			/**
			 * @brief Join the simulator with another one.
			 *
			 * Joins the simulator with another one.
			 * The simulator is enlarged to contain the qubits from both simulators.
			 * The qubits maps are updated accordingly.
			 * 
			 * @param simId The id of the simulator.
			 * @param other The other simulator to join with.
			 * @param qubitsMapToSim The map between qubits and simulators.
			 */
			inline void Join(size_t simId, const std::unique_ptr<IndividualSimulator>& other, std::vector<size_t>& qubitsMapToSim, bool enableMultithreading)
			{
				// 1. grab the state of both simulators (in the first phase, using Amplitude, but maybe something faster can be done)
				// 2. join the states by a tensor product
				const size_t nrQubits1 = GetNumberOfQubits();
				const size_t nrBasisStates1 = 1ULL << nrQubits1;
				const size_t nrQubits2 = other->GetNumberOfQubits();
				const size_t nrBasisStates2 = 1ULL << nrQubits2;

				const size_t newNrQubits = nrQubits1 + nrQubits2;
				const size_t nrBasisStates = 1ULL << newNrQubits;

				SaveStateToInternalDestructive();
				other->SaveStateToInternalDestructive();

				if (GetType() == SimulatorType::kQCSim)
				{
					if (enableMultithreading && nrBasisStates > OmpLimitJoin)
						JoinOmpQcsim(nrQubits1, nrBasisStates1, nrBasisStates2, newNrQubits, nrBasisStates, other, enableMultithreading);
					else
					{
						Eigen::VectorXcd newAmplitudes;
						newAmplitudes.resize(nrBasisStates);

						for (size_t state2 = 0; state2 < nrBasisStates2; ++state2)
						{
							const auto ampl2 = other->AmplitudeRaw(state2);
							const size_t state2Mask = state2 << nrQubits1;
							for (size_t state1 = 0; state1 < nrBasisStates1; ++state1)
								newAmplitudes[state2Mask | state1] = AmplitudeRaw(state1) * ampl2;
						}

						// 3. set the state of the current simulator to the joined state
						// the original qubits of this simulator get mapped as they are
						// the other ones get shifted to the left by the number of qubits of this simulator
						// so transfer mapping keeping this in mind

						//simulator->SetMultithreading(enableMultithreading);
						simulator->InitializeState(newNrQubits, newAmplitudes); // this will end up by swapping the data from newAmplitudes to the simulator, no allocation and copying is done
					}
				}
#ifndef NO_QISKIT_AER
				else
				{
					if (enableMultithreading && nrBasisStates > OmpLimitJoin)
						JoinOmpAer(nrQubits1, nrBasisStates1, nrBasisStates2, newNrQubits, nrBasisStates, other, enableMultithreading);
					else
					{
						AER::Vector<std::complex<double>> newAmplitudes(nrBasisStates, false); // the false here avoids data initialization, it will be set anyway

						for (size_t state2 = 0; state2 < nrBasisStates2; ++state2)
						{
							const auto ampl2 = other->AmplitudeRaw(state2);
							const size_t state2Mask = state2 << nrQubits1;
							for (size_t state1 = 0; state1 < nrBasisStates1; ++state1)
								newAmplitudes[state2Mask | state1] = AmplitudeRaw(state1) * ampl2;
						}

						// 3. set the state of the current simulator to the joined state
						// the original qubits of this simulator get mapped as they are
						// the other ones get shifted to the left by the number of qubits of this simulator
						// so transfer mapping keeping this in mind
						//simulator->SetMultithreading(enableMultithreading);
						simulator->InitializeState(newNrQubits, newAmplitudes); // this will move the data from newAmplitudes to the simulator, no allocation and copying is done
					}
				}
#endif

				for (auto [origq, mapq] : other->GetQubitsMap())
				{
					qubitsMap[origq] = mapq + nrQubits1;
					qubitsMapToSim[origq] = simId;
				}
			}


			/**
			 * @brief Split the simulator.
			 *
			 * Splits the simulator.
			 * The simulator is split into two, the current one and a new one.
			 * The passed qubit is removed from the current simulator and added to the new one.
		     * The qubit is mapped to the only local qubit in the new simulator, which is 0.
			 * 
			 * @param qubit The qubit to split the simulator at.
			 * @param qubitOutcome The outcome of the qubit.
			 * @return The new simulator split from this one.
			 */
			inline std::unique_ptr<IndividualSimulator> Split(size_t qubit, bool qubitOutcome, bool enableMultithreading)
			{
				const size_t oldNrQubits = GetNumberOfQubits();
				const size_t newNrQubits = oldNrQubits - 1;
				const size_t nrBasisStates = 1ULL << newNrQubits;
				const size_t localQubit = qubitsMap[qubit];

				// the new simulator split from this one
				// a one qubit one, containing the qubit that was measured or reset
				// TODO: this can be optimized a little bit by directly initializing with the proper amplitudes, but I'm not sure if it's worth it
				auto newSimulator = std::make_unique<IndividualSimulator>(GetType());
				newSimulator->AllocateQubits(1);
				newSimulator->GetQubitsMap()[qubit] = 0; // the qubit is mapped to the only local qubit in the new simulator, which is 0
				newSimulator->SetMultithreading(enableMultithreading);
				newSimulator->Initialize();
				if (qubitOutcome) 
				{
					newSimulator->ApplyX(qubit);
					//newSimulator->Flush();
				}

				qubitsMap.erase(qubit); // the qubit is removed from the current simulator
				
				SaveStateToInternalDestructive();

				if (GetType() == SimulatorType::kQCSim)
				{
					/*
					if (nrBasisStates > OmpLimitSplit) // parallelization for assignment and some bit manipulations, I must do some benchmarks to see if it's worth it and find where the limit is
						SplitOmpQcsim(localQubit, newNrQubits, nrBasisStates, qubitOutcome);
					else
					*/
					{
						// now the adjusted current simulator, without the removed qubit
						Eigen::VectorXcd newAmplitudes;
						newAmplitudes.resize(nrBasisStates);

						// compute the new amplitudes

						const size_t localQubitMask = 1ULL << localQubit;
						const size_t maskLow = localQubitMask - 1ULL;
						const size_t maskHigh = ~maskLow;
						const size_t qubitMask = qubitOutcome ? localQubitMask : 0ULL;

						for (size_t state = 0; state < nrBasisStates; ++state)
						{
							const size_t stateLow = state & maskLow;
							const size_t stateHigh = (state & maskHigh) << 1ULL;

							newAmplitudes[state] = AmplitudeRaw(stateLow | stateHigh | qubitMask);
						}

						//simulator->SetMultithreading(enableMultithreading);
						simulator->InitializeState(newNrQubits, newAmplitudes); // this will end up by swapping the data from newAmplitudes to the simulator, no allocation and copying is done
					}
				}
#ifndef NO_QISKIT_AER
				else
				{
					/*
					if (nrBasisStates > OmpLimitSplit) // parallelization for assignment and some bit manipulations, I must do some benchmarks to see if it's worth it and find where the limit is
						SplitOmpAer(localQubit, newNrQubits, nrBasisStates, qubitOutcome);
					else
					*/
					{
						// now the adjusted current simulator, without the removed qubit
						AER::Vector<std::complex<double>> newAmplitudes(nrBasisStates, false); // the false here avoids data initialization, it will be set anyway

						// compute the new amplitudes
						const size_t localQubitMask = 1ULL << localQubit;
						const size_t maskLow = localQubitMask - 1ULL;
						const size_t maskHigh = ~maskLow;
						const size_t qubitMask = qubitOutcome ? localQubitMask : 0ULL;

						for (size_t state = 0; state < nrBasisStates; ++state)
						{
							const size_t stateLow = state & maskLow;
							const size_t stateHigh = (state & maskHigh) << 1ULL;

							newAmplitudes[state] = AmplitudeRaw(stateLow | stateHigh | qubitMask);
						}

						//simulator->SetMultithreading(enableMultithreading);
						simulator->InitializeState(newNrQubits, newAmplitudes); // this will move the data from newAmplitudes to the simulator, no allocation and copying is done
					}
				}
#endif

				// now adjust the local qubits map
				for (auto& mapped : qubitsMap)
					if (mapped.second > localQubit)
						--mapped.second;

				return newSimulator;
			}

			/**
			 * @brief Returns the amplitude of the specified state.
			 *
			 * Use it to obtain the amplitude of the specified state, with no conversion.
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param outcome The outcome to obtain the amplitude for.
			 * @return The amplitude of the specified outcome.
			 */
			std::complex<double> AmplitudeRaw(Types::qubit_t outcome) override
			{
				return simulator->AmplitudeRaw(outcome);
			}


			/**
			 * @brief Saves the state to internal storage.
			 *
			 * Saves the state to internal storage, if needed.
			 * Calling this should consider as the simulator is gone to uninitialized.
			 * Either do not use it except for getting amplitudes, or reinitialize the simulator after calling it.
			 * This is needed only for the composite simulator, for an optimization for qiskit aer.
			 * For qcsim it does nothing.
			 */
			void SaveStateToInternalDestructive() override
			{
				simulator->SaveStateToInternalDestructive();
			}


			/**
			 * @brief Restores the state from the internally saved state
			 *
			 * Restores the state from the internally saved state, if needed.
			 * This does something only for qiskit aer.
			 */
			void RestoreInternalDestructiveSavedState() override
			{
				simulator->RestoreInternalDestructiveSavedState();
			}


			/**
			 * @brief Convert qubits from the original id to the simulator's qubit id
			 *
			 * Converts qubits from the ids as seen from 'outside' to the ids as seen from the simulator.
			 * 
			 * @param qubits The qubits to convert.
			 * @return The converted qubits.
			 */
			inline Types::qubits_vector ConvertQubits(const Types::qubits_vector& qubits)
			{
				Types::qubits_vector converted;
				converted.reserve(qubits.size());

				for (auto qubit : qubits)
					if (HasQubit(qubit))
						converted.emplace_back(qubitsMap[qubit]);

				return converted;
			}

			/**
			 * @brief Checks if the qubit is present in the simulator.
			 *
			 * Checks if the qubit is present in the simulator.
			 * 
			 * @param qubit The qubit to check, the id being the 'outside' id.
			 * @return True if the qubit is present, false otherwise.
			 */
			inline bool HasQubit(Types::qubit_t qubit) const
			{
				return qubitsMap.find(qubit) != qubitsMap.end();
			}

			/**
			 * @brief Convert the outcome from the local qubit ids to the global qubit ids
			 *
			 * Converts the outcome from the local qubit ids to the global qubit ids.
			 * 
			 * @param outcome The outcome to convert.
			 * @return The converted outcome.
			 */
			inline Types::qubit_t ConvertOutcomeFromLocal(Types::qubit_t outcome) const
			{
				Types::qubit_t res = 0;

				for (auto [origQubit, localQubit] : qubitsMap)
					if (outcome & (1ULL << localQubit))
						res |= (1ULL << origQubit);

				return res;
			}

			/**
			 * @brief Convert the outcome from the global qubit ids to the local qubit ids
			 *
			 * Converts the outcome from the global qubit ids to the local qubit ids.
			 * 
			 * @param outcome The outcome to convert.
			 * @return The converted outcome.
			 */
			inline Types::qubit_t ConvertOutcomeFromGlobal(Types::qubit_t outcome) const
			{
				Types::qubit_t res = 0;

				for (auto [origQubit, localQubit] : qubitsMap)
					if (outcome & (1ULL << origQubit))
						res |= (1ULL << localQubit);

				return res;
			}

			/**
			 * @brief Save the state.
			 *
			 * Saves the state.
			 */
			void SaveState()
			{
				if (!simulator) return;
				const size_t nrBasisStates = 1ULL << simulator->GetNumberOfQubits();
				savedState.reserve(nrBasisStates);

				for (Types::qubit_t state = 0; state < nrBasisStates; ++state)
					savedState.emplace_back(simulator->Amplitude(state));
			}

			/**
			 * @brief Clear the saved state.
			 *
			 * Clears the saved state.
			 */
			void ClearSavedState()
			{
				savedState.clear();
			}

			/**
			 * @brief Restore the state.
			 *
			 * Restores the state.
			 */
			void RestoreState()
			{
				if (!simulator) return;
				const size_t nrQubits = simulator->GetNumberOfQubits();

				simulator->Clear();
				simulator->InitializeState(nrQubits, savedState);
				ClearSavedState();
			}

			/**
			 * @brief Returns the qubits map.
			 *
			 * Returns the qubits map, mapping the 'outside' qubits ids to simulator's qubits ids.
			 * 
			 * @return The qubits map.
			 */
			inline std::unordered_map<Types::qubit_t, Types::qubit_t>& GetQubitsMap()
			{
				return qubitsMap;
			}

			/**
			 * @brief Returns the qubits map.
			 *
			 * Returns the qubits map, mapping the 'outside' qubits ids to simulator's qubits ids.
			 * 
			 * @return The qubits map.
			 */
			inline const std::unordered_map<Types::qubit_t, Types::qubit_t>& GetQubitsMap() const
			{
				return qubitsMap;
			}

			/**
			 * @brief Initializes the state.
			 *
			 * This function is called when the simulator is initialized.
			 * Call it after the qubits allocation.
			 */
			void Initialize() override
			{
				simulator->Initialize();
			}

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
			void InitializeState(size_t num_qubits, std::vector<std::complex<double>>& amplitudes) override
			{
				simulator->InitializeState(num_qubits, amplitudes);
			}

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
			/*
			void InitializeState(size_t num_qubits, std::vector<std::complex<double>, avoid_init_allocator<std::complex<double>>>& amplitudes) override
			{
				simulator->InitializeState(num_qubits, amplitudes);
			}
			*/


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
			void InitializeState(size_t num_qubits, AER::Vector<std::complex<double>>& amplitudes) override
			{
				simulator->InitializeState(num_qubits, amplitudes);
			}
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
			void InitializeState(size_t num_qubits, Eigen::VectorXcd& amplitudes) override
			{
				simulator->InitializeState(num_qubits, amplitudes);
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
				simulator->Configure(key, value);
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
				if (!simulator) return "";

				return simulator->GetConfiguration(key);
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
				return simulator->AllocateQubits(num_qubits);
			}

			/**
			 * @brief Returns the number of qubits.
			 *
			 * This function is called to obtain the number of the allocated qubits.
			 * @return The number of qubits.
			 */
			size_t GetNumberOfQubits() const override
			{
				return simulator->GetNumberOfQubits();
			}

			/**
			 * @brief Clears the state.
			 *
			 * Sets the number of allocated qubits to 0 and clears the state.
			 * After this qubits allocation is required then calling IState::AllocateQubits in order to use the simulator.
			 */
			void Clear() override
			{
				simulator->Clear();
			}

			/**
			 * @brief Performs a measurement on the specified qubits.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param qubits A vector with the qubits to be measured.
			 * @return The outcome of the measurements, the first qubit result is the least significant bit.
			 */
			size_t Measure(const Types::qubits_vector& qubits) override
			{
				return ConvertOutcomeFromLocal(simulator->Measure(ConvertQubits(qubits)));
			}

			/**
			 * @brief Performs a reset of the specified qubits.
			 *
			 * Measures the qubits and for those that are 1, applies X on them
			 * @param qubits A vector with the qubits to be reset.
			 */
			void ApplyReset(const Types::qubits_vector& qubits) override
			{
				simulator->ApplyReset(ConvertQubits(qubits));
			}

			/**
			 * @brief Returns the probability of the specified outcome.
			 *
			 * Use it to obtain the probability to obtain the specified outcome, if all qubits are measured.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param outcome The outcome to obtain the probability for.
			 * @return The probability of the specified outcome.
			 */
			double Probability(Types::qubit_t outcome) override
			{
				return simulator->Probability(ConvertOutcomeFromGlobal(outcome));
			}

			/**
			 * @brief Returns the amplitude of the specified state.
			 *
			 * Use it to obtain the amplitude of the specified state.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param outcome The outcome to obtain the amplitude for.
			 * @return The amplitude of the specified outcome.
			 */
			std::complex<double> Amplitude(Types::qubit_t outcome) override
			{
				return simulator->Amplitude(ConvertOutcomeFromGlobal(outcome));
			}

			/**
			 * @brief Returns the probabilities of all possible outcomes.
			 *
			 * Use it to obtain the probabilities of all possible outcomes.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @return A vector with the probabilities of all possible outcomes.
			 */
			std::vector<double> AllProbabilities() override
			{
				return simulator->AllProbabilities();
			}

			/**
			 * @brief Returns the probabilities of the specified outcomes.
			 *
			 * Use it to obtain the probabilities of the specified outcomes.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param qubits A vector with the qubits configuration outcomes.
			 * @return A vector with the probabilities for the specified qubit configurations.
			 */
			std::vector<double> Probabilities(const Types::qubits_vector& qubits) override
			{
				return simulator->Probabilities(ConvertQubits(qubits));
			}

			/**
			 * @brief Returns the counts of the outcomes of measurement of the specified qubits, for repeated measurements.
			 *
			 * Use it to obtain the counts of the outcomes of the specified qubits measurements.
			 * The state is not collapsed, so the measurement can be repeated 'shots' times.
			 *
			 * WARNING: Use it with care, might not do what you expect.
			 *
			 * @param qubits A vector with the qubits to be measured.
			 * @param shots The number of shots to perform.
			 * @return A map with the counts for the otcomes of measurements of the specified qubits.
			 */
			std::unordered_map<Types::qubit_t, Types::qubit_t> SampleCounts(const Types::qubits_vector& qubits, size_t shots = 1000) override
			{
				std::unordered_map<Types::qubit_t, Types::qubit_t> res;

				const auto sc = simulator->SampleCounts(ConvertQubits(qubits), shots);

				for (auto [outcome, count] : sc)
					res[ConvertOutcomeFromLocal(outcome)] = count;

				return res;
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
				return simulator->ExpectationValue(pauliString);
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
				return simulator->GetType();
			}

			/**
			 * @brief Returns the simulation type.
			 *
			 * Returns the simulation type.
			 *
			 * @return The simulation type.
			 * @sa SimulationType
			 */
			SimulationType GetSimulationType() const override
			{
				return SimulationType::kStatevector;
			}

			/**
			 * @brief Flushes the applied operations
			 *
			 * This function is called to flush the applied operations.
			 * It is used to flush the operations that were applied to the state, qcsim applies them right away, but qiskit aer does not.
			 */
			void Flush() override
			{
				simulator->Flush();
			}

			// WARNING: for all the following functions, the call is supposed to be made after the proper joining (or splitting)!

			/**
			 * @brief Applies a phase shift gate to the qubit
			 *
			 * Applies a specified phase shift gate to the qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param lambda The phase shift angle.
			 */
			void ApplyP(Types::qubit_t qubit, double lambda) override
			{
				simulator->ApplyP(qubitsMap[qubit], lambda);
			}

			/**
			 * @brief Applies a not gate to the qubit
			 *
			 * Applies a not (X) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyX(Types::qubit_t qubit) override
			{
				simulator->ApplyX(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Y gate to the qubit
			 *
			 * Applies a not (Y) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyY(Types::qubit_t qubit) override
			{
				simulator->ApplyY(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Z gate to the qubit
			 *
			 * Applies a not (Z) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyZ(Types::qubit_t qubit) override
			{
				simulator->ApplyZ(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Hadamard gate to the qubit
			 *
			 * Applies a Hadamard gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyH(Types::qubit_t qubit) override
			{
				simulator->ApplyH(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a S gate to the qubit
			 *
			 * Applies a S gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyS(Types::qubit_t qubit) override
			{
				simulator->ApplyS(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a S dagger gate to the qubit
			 *
			 * Applies a S dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySDG(Types::qubit_t qubit) override
			{
				simulator->ApplySDG(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a T gate to the qubit
			 *
			 * Applies a T gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyT(Types::qubit_t qubit) override
			{
				simulator->ApplyT(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a T dagger gate to the qubit
			 *
			 * Applies a T dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyTDG(Types::qubit_t qubit) override
			{
				simulator->ApplyTDG(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Sx gate to the qubit
			 *
			 * Applies a Sx gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySx(Types::qubit_t qubit) override
			{
				simulator->ApplySx(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Sx dagger gate to the qubit
			 *
			 * Applies a Sx dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySxDAG(Types::qubit_t qubit) override
			{
				simulator->ApplySxDAG(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a K gate to the qubit
			 *
			 * Applies a K (Hy) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyK(Types::qubit_t qubit) override
			{
				simulator->ApplyK(qubitsMap[qubit]);
			}

			/**
			 * @brief Applies a Rx gate to the qubit
			 *
			 * Applies an x rotation gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param theta The rotation angle.
			 */
			void ApplyRx(Types::qubit_t qubit, double theta) override
			{
				simulator->ApplyRx(qubitsMap[qubit], theta);
			}

			/**
			 * @brief Applies a Ry gate to the qubit
			 *
			 * Applies a y rotation gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param theta The rotation angle.
			 */
			void ApplyRy(Types::qubit_t qubit, double theta) override
			{
				simulator->ApplyRy(qubitsMap[qubit], theta);
			}

			/**
			 * @brief Applies a Rz gate to the qubit
			 *
			 * Applies a z rotation gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param theta The rotation angle.
			 */
			void ApplyRz(Types::qubit_t qubit, double theta) override
			{
				simulator->ApplyRz(qubitsMap[qubit], theta);
			}

			/**
			 * @brief Applies a U gate to the qubit
			 *
			 * Applies a z rotation gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param theta The first parameter.
			 * @param phi The second parameter.
			 * @param lambda The third parameter.
			 * @param gamma The fourth parameter.
			 */
			void ApplyU(Types::qubit_t qubit, double theta, double phi, double lambda, double gamma) override
			{
				simulator->ApplyU(qubitsMap[qubit], theta, phi, lambda, gamma);
			}

			/**
			 * @brief Applies a CX gate to the qubits
			 *
			 * Applies a controlled X gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCX(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCX(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a CY gate to the qubits
			 *
			 * Applies a controlled Y gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCY(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCY(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a CZ gate to the qubits
			 *
			 * Applies a controlled Z gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCZ(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCZ(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a CP gate to the qubits
			 *
			 * Applies a controlled phase gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 * @param lambda The phase shift angle.
			 */
			void ApplyCP(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit,
				double lambda) override
			{
				simulator->ApplyCP(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit], lambda);
			}

			/**
			 * @brief Applies a CRx gate to the qubits
			 *
			 * Applies a controlled x rotation gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 * @param theta The rotation angle.
			 */
			void ApplyCRx(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) override
			{
				simulator->ApplyCRx(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit], theta);
			}

			/**
			 * @brief Applies a CRy gate to the qubits
			 *
			 * Applies a controlled y rotation gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 * @param theta The rotation angle.
			 */
			void ApplyCRy(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) override
			{
				simulator->ApplyCRy(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit], theta);
			}

			/**
			 * @brief Applies a CRz gate to the qubits
			 *
			 * Applies a controlled z rotation gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 * @param theta The rotation angle.
			 */
			void ApplyCRz(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) override
			{
				simulator->ApplyCRz(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit], theta);
			}

			/**
			 * @brief Applies a CH gate to the qubits
			 *
			 * Applies a controlled Hadamard gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCH(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCH(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a CSx gate to the qubits
			 *
			 * Applies a controlled squared root not gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCSx(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCSx(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a CSx dagger gate to the qubits
			 *
			 * Applies a controlled squared root not dagger gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 */
			void ApplyCSxDAG(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) override
			{
				simulator->ApplyCSxDAG(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit]);
			}

			/**
			 * @brief Applies a swap gate to the qubits
			 *
			 * Applies a swap gate to the specified qubits
			 * @param qubit0 The first qubit
			 * @param qubit1 The second qubit
			 */
			void ApplySwap(Types::qubit_t qubit0, Types::qubit_t qubit1) override
			{
				simulator->ApplySwap(qubitsMap[qubit0], qubitsMap[qubit1]);
			}

			/**
			 * @brief Applies a controlled controlled not gate to the qubits
			 *
			 * Applies a controlled controlled not gate to the specified qubits
			 * @param qubit0 The first control qubit
			 * @param qubit1 The second control qubit
			 * @param qubit2 The target qubit
			 */
			void ApplyCCX(Types::qubit_t qubit0, Types::qubit_t qubit1, Types::qubit_t qubit2) override
			{
				simulator->ApplyCCX(qubitsMap[qubit0], qubitsMap[qubit1], qubitsMap[qubit2]);
			}

			/**
			 * @brief Applies a controlled swap gate to the qubits
			 *
			 * Applies a controlled swap gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param qubit0 The first qubit
			 * @param qubit1 The second qubit
			 */
			void ApplyCSwap(Types::qubit_t ctrl_qubit, Types::qubit_t qubit0, Types::qubit_t qubit1) override
			{
				simulator->ApplyCSwap(qubitsMap[ctrl_qubit], qubitsMap[qubit0], qubitsMap[qubit1]);
			}

			/**
			 * @brief Applies a controlled U gate to the qubits
			 *
			 * Applies a controlled U gate to the specified qubits
			 * @param ctrl_qubit The control qubit
			 * @param tgt_qubit The target qubit
			 * @param theta Theta parameter for the U gate
			 * @param phi Phi parameter for the U gate
			 * @param lambda Lambda parameter for the U gate
			 * @param gamma Gamma parameter for the U gate
			 */
			void ApplyCU(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta, double phi, double lambda, double gamma) override
			{
				simulator->ApplyCU(qubitsMap[ctrl_qubit], qubitsMap[tgt_qubit], theta, phi, lambda, gamma);
			}

			void ApplyNop() override
			{
				simulator->ApplyNop();
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
				if (simulator) simulator->SetMultithreading(multithreading);

				processor_count = multithreading ? QC::QubitRegister<>::GetNumberOfThreads() : 1;
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
				if (simulator) return simulator->GetMultithreading();

				return false;
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
				return GetType() == Simulators::SimulatorType::kQiskitAer;
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
				return ConvertOutcomeFromLocal(simulator->MeasureNoCollapse());
			}

			/**
			 * @brief Clones the simulator.
			 *
			 * Clones the simulator, including the state, the configuration and the internally saved state, if any.
			 * Does not copy the observers.
			 * Should be used mainly internally, to optimise multiple shots execution, copying the state from the simulator used for timing.
			 *
			 * @return A shared pointer to the cloned simulator.
			 */
			std::unique_ptr<ISimulator> Clone() override
			{
				auto cloned = std::make_unique<IndividualSimulator>();

				cloned->qubitsMap = qubitsMap; /**< A map between qubits (as identified from outside) and the qubits from the actual simulator */
				cloned->savedState = savedState; /**< The saved state, to be used saving/restoring, for example when calling SampleCounts */
				cloned->simulator = simulator->Clone();        /**< The actual simulator to use */

				return cloned;
			}

			Types::qubit_t SampleFromAlias()
			{
				if (!alias || !simulator) return 0;
					
				double prob = 0.0;
				if (GetType() == SimulatorType::kQCSim)
				{
					// qcsim - convert 'simulator' to qcsim simulator and access 'state' (from there the statevector is accessible)
					QCSimSimulator* qcsim = dynamic_cast<QCSimSimulator*>(simulator.get());
					prob = 1. - qcsim->uniformZeroOne(qcsim->rng);
				}
				else
				{
					// qiskit aer - convert 'simulator' to qiskit aer simulator and access 'savedAmplitudes' (assumes destructive saving of the state)
					AerSimulator* aer = dynamic_cast<AerSimulator*>(simulator.get());
					prob = 1 - aer->uniformZeroOne(aer->rng);
				}

				const size_t measRaw = alias->Sample(prob);

				return ConvertOutcomeFromLocal(measRaw);
			}

		private:
			void InitializeAlias()
			{
				// TODO: implement it!
				if (GetType() == SimulatorType::kQCSim)
				{
					// qcsim - convert 'simulator' to qcsim simulator and access 'state' (from there the statevector is accessible)
					QCSimSimulator* qcsim = dynamic_cast<QCSimSimulator*>(simulator.get());

					alias = std::unique_ptr<Utils::Alias>(new Utils::Alias(qcsim->state->getRegisterStorage()));
				}
				else
				{
					// qiskit aer - convert 'simulator' to qiskit aer simulator and access 'savedAmplitudes' (assumes destructive saving of the state)
					AerSimulator* aer = dynamic_cast<AerSimulator*>(simulator.get());

					alias = std::unique_ptr<Utils::Alias>(new Utils::Alias(aer->savedAmplitudes));
				}
			}

			void ClearAlias()
			{
				alias = nullptr;
			}

			/**
			 * @brief Join helper function for joining with open mp
			 * 
			 * Join helper function for joining with open mp, for the aer simulators
			 *
			 * @param nrQubits1 The number of qubits of the first simulator.
			 * @param nrBasisStates1 The number of basis states of the first simulator.
			 * @param nrBasisStates2 The number of basis states of the second simulator.
			 * @param newNrQubits The number of qubits of the joined simulator.
			 * @param nrBasisStates The number of basis states of the joined simulator.
			 * @param other The other simulator to join with.
			 */
#ifndef NO_QISKIT_AER
			inline void JoinOmpAer(size_t nrQubits1, size_t nrBasisStates1, size_t nrBasisStates2, size_t newNrQubits, size_t nrBasisStates, const std::unique_ptr<IndividualSimulator>& other, bool enableMultithreading)
			{
				AER::Vector<std::complex<double>> newAmplitudes(nrBasisStates, false); // the false here avoids data initialization, it will be set anyway

				/*
				const size_t state1Mask = nrBasisStates1 - 1ULL;

#pragma omp parallel for num_threads(processor_count) schedule(static, OmpLimitJoin / divSchedule)
				for (long long int state = 0; state < static_cast<long long int>(nrBasisStates); ++state)
					newAmplitudes[state] = AmplitudeRaw(state & state1Mask) * other->AmplitudeRaw(state >> nrQubits1);
				*/

				// TODO: check if this is better
#pragma omp parallel for num_threads(processor_count)
				for (long long int state2 = 0; state2 < static_cast<long long int>(nrBasisStates2); ++state2)
				{
					const auto ampl2 = other->AmplitudeRaw(state2);
					const size_t state2Mask = state2 << nrQubits1;
					for (size_t state1 = 0; state1 < nrBasisStates1; ++state1)
						newAmplitudes[state2Mask | state1] = AmplitudeRaw(state1) * ampl2;
				}
				

				// 3. set the state of the current simulator to the joined state
				// the original qubits of this simulator get mapped as they are
				// the other ones get shifted to the left by the number of qubits of this simulator
				// so transfer mapping keeping this in mind
				//simulator->SetMultithreading(enableMultithreading);
				simulator->InitializeState(newNrQubits, newAmplitudes); // this will move the data from newAmplitudes to the simulator, no allocation and copying is done
			}
#endif

			/**
			 * @brief Split helper function for splitting with open mp
			 *
			 * Split helper function for splitting with open mp, for the aer simulators
	         *
			 * @param localQubit The qubit to split out.
			 * @param newNrQubits The number of qubits of the new simulator.
			 * @param nrBasisStates The number of basis states of the new simulator.
			 * @param qubitOutcome The outcome of the qubit to split out.
			 */
			/*
			inline void SplitOmpAer(size_t localQubit, size_t newNrQubits, size_t nrBasisStates, bool qubitOutcome = false)
			{
				// now the adjusted current simulator, without the removed qubit
				AER::Vector<std::complex<double>> newAmplitudes(nrBasisStates, false); // the false here avoids data initialization, it will be set anyway

				// compute the new amplitudes

				const size_t localQubitMask = 1ULL << localQubit;
				const size_t maskLow = localQubitMask - 1ULL;
				const size_t maskHigh = ~maskLow;
				const size_t qubitMask = qubitOutcome ? localQubitMask : 0ULL;

#pragma omp parallel for num_threads(processor_count) schedule(static, OmpLimitSplit / divSchedule)
				for (long long int state = 0; state < static_cast<long long int>(nrBasisStates); ++state)
				{
					const size_t stateLow = state & maskLow;
					const size_t stateHigh = (state & maskHigh) << 1ULL;

					newAmplitudes[state] = AmplitudeRaw(stateLow | stateHigh | qubitMask);
				}

				simulator->InitializeState(newNrQubits, newAmplitudes); // this will move the data from newAmplitudes to the simulator, no allocation and copying is done
			}
			*/

			/**
			 * @brief Join helper function for joining with open mp
			 *
			 * Join helper function for joining with open mp, for the qcsim simulators
			 *
			 * @param nrQubits1 The number of qubits of the first simulator.
			 * @param nrBasisStates1 The number of basis states of the first simulator.
			 * @param nrBasisStates2 The number of basis states of the second simulator.
			 * @param newNrQubits The number of qubits of the joined simulator.
			 * @param nrBasisStates The number of basis states of the joined simulator.
			 * @param other The other simulator to join with.
			 */
			inline void JoinOmpQcsim(size_t nrQubits1, size_t nrBasisStates1, size_t nrBasisStates2, size_t newNrQubits, size_t nrBasisStates, const std::unique_ptr<IndividualSimulator>& other, bool enableMultithreading)
			{
				Eigen::VectorXcd newAmplitudes;
				newAmplitudes.resize(nrBasisStates);
				
				/*
				const size_t state1Mask = nrBasisStates1 - 1ULL;

#pragma omp parallel for num_threads(processor_count) schedule(static, OmpLimitJoin / divSchedule)
				for (long long int state = 0; state < static_cast<long long int>(nrBasisStates); ++state)
					newAmplitudes[state] = AmplitudeRaw(state & state1Mask) * other->AmplitudeRaw(state >> nrQubits1);
				*/

				// TODO: check if this is better
#pragma omp parallel for num_threads(processor_count)
				for (long long int state2 = 0; state2 < static_cast<long long int>(nrBasisStates2); ++state2)
				{
					const auto ampl2 = other->AmplitudeRaw(state2);
					const size_t state2Mask = state2 << nrQubits1;
					for (size_t state1 = 0; state1 < nrBasisStates1; ++state1)
						newAmplitudes[state2Mask | state1] = AmplitudeRaw(state1) * ampl2;
				}

				// 3. set the state of the current simulator to the joined state
				// the original qubits of this simulator get mapped as they are
				// the other ones get shifted to the left by the number of qubits of this simulator
				// so transfer mapping keeping this in mind
				//simulator->SetMultithreading(enableMultithreading);
				simulator->InitializeState(newNrQubits, newAmplitudes); // this will end up by swapping the data from newAmplitudes to the simulator, no allocation and copying is done
			}

			/**
			 * @brief Split helper function for splitting with open mp
			 *
			 * Split helper function for splitting with open mp, for the qcsim simulators
			 *
			 * @param localQubit The qubit to split out.
			 * @param newNrQubits The number of qubits of the new simulator.
			 * @param nrBasisStates The number of basis states of the new simulator.
			 * @param qubitOutcome The outcome of the qubit to split out.
			 */
			/*
			inline void SplitOmpQcsim(size_t localQubit, size_t newNrQubits, size_t nrBasisStates, bool qubitOutcome = false)
			{
				// now the adjusted current simulator, without the removed qubit
				Eigen::VectorXcd newAmplitudes;
				newAmplitudes.resize(nrBasisStates);

				// compute the new amplitudes
				const size_t localQubitMask = 1ULL << localQubit;
				const size_t maskLow = localQubitMask - 1ULL;
				const size_t maskHigh = ~maskLow;
				const size_t qubitMask = qubitOutcome ? localQubitMask : 0ULL;

#pragma omp parallel for num_threads(processor_count) schedule(static, OmpLimitSplit / divSchedule)
				for (long long int state = 0; state < static_cast<long long int>(nrBasisStates); ++state)
				{
					const size_t stateLow = state & maskLow;
					const size_t stateHigh = (state & maskHigh) << 1ULL;

					newAmplitudes[state] = AmplitudeRaw(stateLow | stateHigh | qubitMask);
				}

				simulator->InitializeState(newNrQubits, newAmplitudes); // this will end up by swapping the data from newAmplitudes to the simulator, no allocation and copying is done
			}
			*/

			std::unordered_map<Types::qubit_t, Types::qubit_t> qubitsMap; /**< A map between qubits (as identified from outside) and the qubits from the actual simulator */
			std::unique_ptr<ISimulator> simulator;        /**< The actual simulator to use */
			std::vector<std::complex<double>> savedState; /**< The saved state, to be used saving/restoring, for example when calling SampleCounts */

			std::unique_ptr<Utils::Alias> alias; /**< The alias object to use for sampling */

			int processor_count = QC::QubitRegister<>::GetNumberOfThreads(); /**< The number of processors to use for parallelization */

			//constexpr static int divSchedule = 4;
			constexpr static size_t OmpLimitJoin = 4096 * 2;
			//constexpr static size_t OmpLimitSplit = OmpLimitJoin * 16;
		};
	
	}
}

#endif
#endif