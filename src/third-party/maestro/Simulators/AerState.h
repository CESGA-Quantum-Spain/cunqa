/**
 * @file AerState.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The aer state class.
 * 
 * Should not be used directly, create an instance with the factory and use the generic simulator interface.
 */

#pragma once

#ifndef _AER_STATE_H_
#define _AER_STATE_H_

#ifndef NO_QISKIT_AER

#ifdef INCLUDED_BY_FACTORY

#include <algorithm>

#include "Simulator.h"
#include "QubitRegister.h"

#include "QiskitAerState.h"

namespace Simulators {
	// TODO: Maybe use the pimpl idiom https://en.cppreference.com/w/cpp/language/pimpl to hide the implementation for good
	// but during development this should be good enough
	namespace Private {

		class IndividualSimulator;

		/**
		 * @class AerState
		 * @brief Class for the qiskit aer simulator state.
		 *
		 * Implements the qiskit aer state.
		 * Do not use this class directly, use the factory to create an instance.
		 * @sa ISimulator
		 * @sa IState
		 * @sa AerSimulator
		 */
		class AerState : public ISimulator
		{
			friend class IndividualSimulator; /**< Allows the IndividualSimulator to access the private members of AerState */
		public:
			/**
			 * @brief The constructor.
			 *
			 * The constructor for the qiskit aer simulator state.
			 * Seeds the random number generator.
			 */
			AerState()
			{
				std::random_device rd;
				
				rng.seed(rd());
				Configure("method", "statevector");
			}

			/**
			 * @brief Initializes the state.
			 *
			 * This function is called when the simulator is initialized.
			 * Call it after the qubits allocation.
			 * @sa AerState::AllocateQubits
			 */
			void Initialize() override
			{
				SetMultithreading(enableMultithreading);
				if (simulationType == SimulationType::kMatrixProductState)
					Configure("mps_sample_measure_algorithm", useMPSMeasureNoCollapse ? "mps_probabilities" : "mps_apply_measure");
				state->initialize();
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
				Clear();
				state->initialize_statevector(num_qubits, amplitudes.data(), true);
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
			/*
			void InitializeState(size_t num_qubits, std::vector<std::complex<double>, avoid_init_allocator<std::complex<double>>>& amplitudes) override
			{
				Clear();
				state->initialize_statevector(num_qubits, amplitudes.data(), true);
			}
			*/

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
			 void InitializeState(size_t num_qubits, AER::Vector<std::complex<double>>& amplitudes) override
			 {
				 Clear();
				 state->initialize_statevector(num_qubits, amplitudes.move_to_buffer(), false);
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
			void InitializeState(size_t num_qubits, Eigen::VectorXcd& amplitudes) override
			{
				Clear();
				state->initialize_statevector(num_qubits, amplitudes.data(), true);
			}

			/**
			 * @brief Just resets the state to 0.
			 *
			 * Does not destroy the internal state, just resets it to zero (as a 'reset' op on each qubit would do).
			 */
			void Reset() override
			{
				const auto numQubits = GetNumberOfQubits();
				Clear();
				AllocateQubits(numQubits);
				state->initialize();
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
					else if (std::string("stabilizer") == value)
						simulationType = SimulationType::kStabilizer;
					else if (std::string("tensor_network") == value)
						simulationType = SimulationType::kTensorNetwork;
					else
						simulationType = SimulationType::kOther;
				}
				else if (std::string("matrix_product_state_truncation_threshold") == key)
				{
					singularValueThreshold = std::stod(value);
					if (singularValueThreshold > 0.) limitEntanglement = true;
					else limitEntanglement = false;
				}
				else if (std::string("matrix_product_state_max_bond_dimension") == key)
				{
					chi = std::stoi(value);
					if (chi > 0) limitSize = true;
					else limitSize = false;
				}
				else if (std::string("mps_sample_measure_algorithm") == key)
					useMPSMeasureNoCollapse = std::string("mps_probabilities") == value;

				state->configure(key, value);
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
					case SimulationType::kStabilizer:
						return "stabilizer";
					case SimulationType::kTensorNetwork:
						return "tensor_network";
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
				else if (std::string("mps_sample_measure_algorithm") == key)
				{
					return useMPSMeasureNoCollapse ? "mps_probabilities" : "mps_apply_measure";
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
				const auto ids = state->allocate_qubits(num_qubits);
				return ids[0];
			}

			/**
			 * @brief Returns the number of qubits.
			 *
			 * This function is called to obtain the number of the allocated qubits.
			 * @return The number of qubits.
			 */
			size_t GetNumberOfQubits() const override
			{
				if (state->is_initialized())
					return state->num_of_qubits();
				
				return 0;
			}

			/**
			 * @brief Clears the state.
			 *
			 * Sets the number of allocated qubits to 0 and clears the state.
			 * After this qubits allocation is required then calling IState::AllocateQubits in order to use the simulator.
			 */
			void Clear() override
			{
				state->clear();
				SetMultithreading(enableMultithreading);
				
				if (simulationType == SimulationType::kMatrixProductState)
					Configure("mps_sample_measure_algorithm", useMPSMeasureNoCollapse ? "mps_probabilities" : "mps_apply_measure");
			}

			/**
			 * @brief Performs a measurement on the specified qubits.
			 *
			 * @param qubits A vector with the qubits to be measured.
			 * @return The outcome of the measurements, the first qubit result is the least significant bit.
			 */
			size_t Measure(const Types::qubits_vector& qubits) override
			{
				const size_t res = state->apply_measure(qubits);

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
				state->apply_reset(qubits);

				NotifyObservers(qubits);
			}

			/**
			 * @brief Returns the probability of the specified outcome.
			 *
			 * Use it to obtain the probability to obtain the specified outcome, if all qubits are measured.
			 * @sa AerState::Amplitude
			 * @sa AerState::Probabilities
			 *
			 * @param outcome The outcome to obtain the probability for.
			 * @return The probability of the specified outcome.
			 */
			double Probability(Types::qubit_t outcome) override
			{
				return state->probability(outcome);
			}

			/**
			 * @brief Returns the amplitude of the specified state.
			 *
			 * Use it to obtain the amplitude of the specified state.
			 * @sa AerState::Probability
			 * @sa AerState::Probabilities
			 *
			 * @param outcome The outcome to obtain the amplitude for.
			 * @return The amplitude of the specified outcome.
			 */
			std::complex<double> Amplitude(Types::qubit_t outcome) override
			{
				return state->amplitude(outcome);
			}

			/**
			 * @brief Returns the probabilities of all possible outcomes.
			 *
			 * Use it to obtain the probabilities of all possible outcomes.
			 * @sa AerState::Probability
			 * @sa AerState::Amplitude
			 * @sa AerState::AllProbabilities
			 *
			 * @return A vector with the probabilities of all possible outcomes.
			 */
			std::vector<double> AllProbabilities() override
			{
				return state->probabilities();
			}
			
			/**
			 * @brief Returns the probabilities of the specified outcomes.
			 *
			 * Use it to obtain the probabilities of the specified outcomes.
			 * @sa AerState::Probability
			 * @sa AerState::Amplitude
			 *
			 * @param qubits A vector with the qubits configuration outcomes.
			 * @return A vector with the probabilities for the specified qubit configurations.
			 */
			std::vector<double> Probabilities(const Types::qubits_vector& qubits) override
			{
				return state->probabilities(qubits);
			}

			/**
			 * @brief Returns the counts of the outcomes of measurement of the specified qubits, for repeated measurements.
			 *
			 * Use it to obtain the counts of the outcomes of the specified qubits measurements.
			 * The state is not collapsed, so the measurement can be repeated 'shots' times.
			 * 
			 * WARNING: Up to 64 qubits, despite that for example MPS simulators can handle more... it's limited in qiskit aer function.
			 *
			 * @param qubits A vector with the qubits to be measured.
			 * @param shots The number of shots to perform.
			 * @return A map with the counts for the outcomes of measurements of the specified qubits.
			 */
			std::unordered_map<Types::qubit_t, Types::qubit_t> SampleCounts(const Types::qubits_vector& qubits, size_t shots = 1000) override
			{
				if (qubits.empty() || shots == 0) return {};

				const std::unordered_map<Types::qubit_t, Types::qubit_t> res = state->sample_counts(qubits, shots);

				NotifyObservers(qubits);

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
			double ExpectationValue(const std::string& pauliStringOrig) override
			{
				if (pauliStringOrig.empty()) return 1.0;

				std::string pauliString = pauliStringOrig;
				if (pauliString.size() > GetNumberOfQubits())
				{
					for (size_t i = GetNumberOfQubits(); i < pauliString.size(); ++i)
					{
						const auto pauliOp = toupper(pauliString[i]);
						if (pauliOp != 'I' && pauliOp != 'Z')
							return 0.0;
					}

					pauliString.resize(GetNumberOfQubits());
				}

				AER::reg_t qubits;
				std::string pauli;

				pauli.reserve(pauliString.size());
				qubits.reserve(pauliString.size());

				for (size_t q = 0; q < pauliString.size(); ++q)
				{
					const char p = toupper(pauliString[q]);
					if (p == 'I') continue;
				
					pauli.push_back(p);
					qubits.push_back(q);
				}

				if (qubits.empty()) return 1.0;

				// qiskit aer expects the pauli string in reverse order
				std::reverse(pauli.begin(), pauli.end());

				return state->expval_pauli(qubits, pauli);
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
				return SimulatorType::kQiskitAer;
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
			 * qcsim applies them right away, so this has no effect on it, but qiskit aer does not.
			 */
			void Flush() override
			{
				state->flush_ops();
				//state->set_random_seed(); // avoid reusing the old seed
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
				savedAmplitudes = state->move_to_vector();
			}

			/**
			 * @brief Restores the state from the internally saved state
			 *
			 * Restores the state from the internally saved state, if needed.
			 * This does something only for qiskit aer.
			 */
			void RestoreInternalDestructiveSavedState() override
			{
				const size_t numQubits = static_cast<size_t>(log2(savedAmplitudes.size()));
				state->initialize_statevector(numQubits, savedAmplitudes.move_to_buffer(), false);
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
				if (!state) return;

				const auto numQubits = GetNumberOfQubits();

				if (simulationType == SimulationType::kStatevector)
				{
					SaveStateToInternalDestructive();
					state->initialize_statevector(numQubits, savedAmplitudes.data(), true);
					return;
				}

				bool saved = false;

				if (state->is_initialized())
				{
					AER::Operations::Op op;
					
					op.type = AER::Operations::OpType::save_state;
					op.name = "save_state";
					op.save_type = AER::Operations::DataSubType::single;
					op.string_params.push_back("s");

					for (size_t q = 0; q < numQubits; ++q)
						op.qubits.push_back(q);

					state->buffer_op(std::move(op));
					Flush();

					// get the state from the last result
					AER::ExperimentResult& last_result = state->last_result();
					// state should be in last_result.data
					if (last_result.status == AER::ExperimentResult::Status::completed)
					{
						savedState = std::move(last_result.data);
						saved = true;
					}
				}
				else
				{
					// try get the state from the last result
					AER::ExperimentResult& last_result_prev = state->last_result();
					// state should be in last_result.data
					if (last_result_prev.status == AER::ExperimentResult::Status::completed)
					{
						savedState = std::move(last_result_prev.data);
						saved = true;
					}
				}
				
				// this is a hack, for statevector and matrix product state if the last op is executed, it can destroy the state!
				// see also the workaround for statevector
				// for the stabilizer at least for now it doesn't seem to do that
				// TODO: check everything!!!!
				if (saved && simulationType == SimulationType::kMatrixProductState)
					RestoreState();
			}

			/**
			 * @brief Restores the state from the internally saved state
			 *
			 * Restores the state from the internally saved state, if needed.
			 * To be used in order to recover the state after doing measurements, for multiple shots executions.
			 * In the first phase, only qcsim will implement this.
			 */
			void RestoreState() override
			{
				auto numQubits = GetNumberOfQubits();

				AER::Operations::Op op;

				switch (simulationType) {
				case SimulationType::kStatevector:
				{
					//op.type = AER::Operations::OpType::set_statevec;
					//op.name = "set_statevec";

					//const auto& vec = static_cast<AER::DataMap<AER::SingleData, AER::Vector<complex_t>>>(savedState).value()["s"].value();

					// this is a hack until I figure it out
					Clear();
					numQubits = static_cast<size_t>(log2(savedAmplitudes.size()));
					state->initialize_statevector(numQubits, savedAmplitudes.data(), true);

					return;
				}
				break;
				case SimulationType::kMatrixProductState:
					op.type = AER::Operations::OpType::set_mps;
					op.name = "set_mps";
					op.mps = static_cast<AER::DataMap<AER::SingleData, AER::mps_container_t>>(savedState).value()["s"].value();
					
					/*
					{
						auto& value = static_cast<AER::DataMap<AER::SingleData, AER::mps_container_t>>(savedState).value();
						if (!value.empty())
						{
							if (value.find("s") != value.end())
								op.mps = value["s"].value();
							else if (value.find("matrix_product_state") != value.end())
								op.mps = value["matrix_product_state"].value();
						}
					}
					*/

					numQubits = op.mps.first.size();
					break;
				case SimulationType::kStabilizer:
					op.type = AER::Operations::OpType::set_stabilizer;
					op.name = "set_stabilizer";
					op.clifford = static_cast<AER::DataMap<AER::SingleData, json_t>>(savedState).value()["s"].value();
					
					/*
					{
						auto& value = static_cast<AER::DataMap<AER::SingleData, json_t>>(savedState).value();
						if (!value.empty())
						{
							if (value.find("s") != value.end())
								op.clifford = value["s"].value();
							else if (value.find("stabilizer") != value.end())
								op.clifford = value["stabilizer"].value();
						}
					}
					*/
		
					numQubits = op.clifford.num_qubits();
					break;
				case SimulationType::kTensorNetwork:
				default:
					throw std::runtime_error("AerState::RestoreState: not implemented yet for this type of simulator.");
				}

				op.save_type = AER::Operations::DataSubType::single;
				op.string_params.push_back("s");

				for (size_t q = 0; q < numQubits; ++q)
					op.qubits.push_back(q);

				// WHY?
				if (!state->is_initialized())
				{
					Clear();
					AllocateQubits(numQubits);
					state->initialize();
				}

				state->buffer_op(std::move(op));
				Flush();
			}

			/**
			 * @brief Gets the amplitude.
			 *
			 * Gets the amplitude, from the internal storage if needed.
			 * This is needed only for the composite simulator, for an optimization for qiskit aer.
			 * For qcsim it does the same thing as Amplitude.
			 */
			std::complex<double> AmplitudeRaw(Types::qubit_t outcome) override
			{
				return savedAmplitudes[outcome];
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
				enableMultithreading = multithreading;
				if (state && !state->is_initialized()) 
				{
					const std::string nrThreads = std::to_string(enableMultithreading ? 0 : 1);
					state->configure("max_parallel_threads", nrThreads);
					state->configure("parallel_state_update", nrThreads);
					const std::string threadsLimit = std::to_string(12);
					state->configure("statevector_parallel_threshold", threadsLimit);
				}
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
				return enableMultithreading;
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
				{
					const double prob = 1. - uniformZeroOne(rng); // this excludes 0 as probabiliy 
					double accum = 0;
					Types::qubit_t state = 0;
					for (Types::qubit_t i = 0; i < savedAmplitudes.size(); ++i)
					{
						accum += std::norm(savedAmplitudes[i]);
						if (prob <= accum)
						{
							state = i;
							break;
						}
					}

					return state;
				}

				throw std::runtime_error("AerState::MeasureNoCollapse: Invalid simulation type for measuring all the qubits without collapsing the state.");

				return 0;
			}

		protected:
			SimulationType simulationType = SimulationType::kStatevector; /**< The simulation type. */
			std::unique_ptr<QiskitAerState> state = std::make_unique<QiskitAerState>(); /**< The qiskit aer state. */
			AER::Vector<complex_t> savedAmplitudes; /**< The amplitudes, saved. */
			bool limitSize = false;
			bool limitEntanglement = false;
			Eigen::Index chi = 10; // if limitSize is true
			double singularValueThreshold = 0.; // if limitEntanglement is true
			bool enableMultithreading = true; /**< The multithreading flag. */
			AER::Data savedState; /**< The saved data - here there will be the saved state of the simulator */
			std::mt19937_64 rng;
			std::uniform_real_distribution<double> uniformZeroOne{ 0., 1. };
			bool useMPSMeasureNoCollapse = true; /**< The flag to use the mps measure no collapse algorithm. */
		};

	}
}

#endif

#endif

#endif // !_AER_STATE_H_




