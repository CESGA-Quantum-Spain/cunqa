/**
 * @file GpuSimulator.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The gpu simulator class.
 *
 * Should not be used directly, create an instance with the factory and use the generic interface instead.
 */

#pragma once

#ifndef _GPUSIMULATOR_H
#define _GPUSIMULATOR_H

#ifdef INCLUDED_BY_FACTORY

#ifdef __linux__

#include "GpuState.h"

namespace Simulators {
	// TODO: Maybe use the pimpl idiom https://en.cppreference.com/w/cpp/language/pimpl to hide the implementation for good
	// but during development this should be good enough
	namespace Private {

		/**
		 * @class GpuSimulator
		 * @brief Gpu simulator class.
		 *
		 * This is the implementation for the gpu simulator.
		 * Do not use this class directly, use the factory to create an instance.
		 * Only the interface should be exposed.
		 * @sa GpuState
		 * @sa ISimulator
		 * @sa IState
		 */
		class GpuSimulator : public GpuState
		{
		public:
			GpuSimulator() = default;
			// allow no copy or assignment
			GpuSimulator(const GpuSimulator&) = delete;
			GpuSimulator& operator=(const GpuSimulator&) = delete;

			// but allow moving
			GpuSimulator(GpuSimulator&& other) = default;
			GpuSimulator& operator=(GpuSimulator&& other) = default;

			/**
			 * @brief Applies a phase shift gate to the qubit
			 *
			 * Applies a specified phase shift gate to the qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param lambda The phase shift angle.
			 */
			void ApplyP(Types::qubit_t qubit, double lambda) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyP(qubit, lambda);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyP(qubit, lambda);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a not gate to the qubit
			 *
			 * Applies a not (X) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyX(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyX(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyX(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a Y gate to the qubit
			 *
			 * Applies a not (Y) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyY(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyY(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyY(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a Z gate to the qubit
			 *
			 * Applies a not (Z) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyZ(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyZ(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyZ(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a Hadamard gate to the qubit
			 *
			 * Applies a Hadamard gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyH(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyH(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyH(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a S gate to the qubit
			 *
			 * Applies a S gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyS(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyS(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyS(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a S dagger gate to the qubit
			 *
			 * Applies a S dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySDG(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplySDG(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplySDG(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a T gate to the qubit
			 *
			 * Applies a T gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyT(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyT(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyT(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a T dagger gate to the qubit
			 *
			 * Applies a T dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyTDG(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyTDG(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyTDG(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a Sx gate to the qubit
			 *
			 * Applies a Sx gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySx(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplySX(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplySX(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a Sx dagger gate to the qubit
			 *
			 * Applies a Sx dagger gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplySxDAG(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplySXDG(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplySXDG(qubit);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a K gate to the qubit
			 *
			 * Applies a K (Hy) gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 */
			void ApplyK(Types::qubit_t qubit) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyK(qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyK(qubit);

				NotifyObservers({ qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyRx(qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyRx(qubit, theta);

				NotifyObservers({ qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyRy(qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyRy(qubit, theta);

				NotifyObservers({ qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyRz(qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyRz(qubit, theta);

				NotifyObservers({ qubit });
			}

			/**
			 * @brief Applies a U gate to the qubit
			 *
			 * Applies a U gate to the specified qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param theta The rotation angle.
			 */
			void ApplyU(Types::qubit_t qubit, double theta, double phi, double lambda, double gamma) override
			{
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyU(qubit, theta, phi, lambda, gamma);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyU(qubit, theta, phi, lambda, gamma);

				NotifyObservers({ qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCX(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCX(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCY(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCY(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCZ(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCZ(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCP(ctrl_qubit, tgt_qubit, lambda);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCP(ctrl_qubit, tgt_qubit, lambda);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCRx(ctrl_qubit, tgt_qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCRx(ctrl_qubit, tgt_qubit, theta);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCRy(ctrl_qubit, tgt_qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCRy(ctrl_qubit, tgt_qubit, theta);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCRz(ctrl_qubit, tgt_qubit, theta);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCRz(ctrl_qubit, tgt_qubit, theta);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCH(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCH(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCSX(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCSX(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCSXDG(ctrl_qubit, tgt_qubit);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCSXDG(ctrl_qubit, tgt_qubit);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplySwap(qubit0, qubit1);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplySwap(qubit0, qubit1);

				NotifyObservers({ qubit1, qubit0 });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCCX(qubit0, qubit1, qubit2);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
				{
					const size_t q1 = qubit0; // control 1
					const size_t q2 = qubit1; // control 2
					const size_t q3 = qubit2; // target

					// Sleator-Weinfurter decomposition
					mps->ApplyCSX(static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					mps->ApplyCX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));
					mps->ApplyCSXDG(static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					mps->ApplyCX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));
					mps->ApplyCSX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q3));
				}
				
				NotifyObservers({ qubit2, qubit1, qubit0 });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCSwap(ctrl_qubit, qubit0, qubit1);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
				{
					const size_t q1 = ctrl_qubit; // control
					const size_t q2 = qubit0;
					const size_t q3 = qubit1;

					// TODO: find a better decomposition
					// this one I've got with the qiskit transpiler
					mps->ApplyCX(static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					mps->ApplyCSX(static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					mps->ApplyCX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));

					mps->ApplyP(static_cast<unsigned int>(q3), M_PI);
					mps->ApplyP(static_cast<unsigned int>(q2), -M_PI_2);

					mps->ApplyCSX(static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					mps->ApplyCX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));

					mps->ApplyP(static_cast<unsigned int>(q3), M_PI);

					mps->ApplyCSX(static_cast<unsigned int>(q1), static_cast<unsigned int>(q3));
					mps->ApplyCX(static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
				}

				NotifyObservers({ qubit1, qubit0, ctrl_qubit });
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
				if (GetSimulationType() == SimulationType::kStatevector)
					state->ApplyCU(ctrl_qubit, tgt_qubit, theta, phi, lambda, gamma);
				else if (GetSimulationType() == SimulationType::kMatrixProductState)
					mps->ApplyCU(ctrl_qubit, tgt_qubit, theta, phi, lambda, gamma);

				NotifyObservers({ tgt_qubit, ctrl_qubit });
			}

			/**
			 * @brief Applies a nop
			 *
			 * Applies a nop (no operation).
			 * Typically does (almost) nothing. Equivalent to an identity.
			 * For qiskit aer it will send the 'nop' to the qiskit aer simulator.
			 */
			void ApplyNop() override
			{
				// do nothing
			}

			/**
			 * @brief Clones the simulator.
			 *
			 * Clones the simulator, including the state, the configuration and the internally saved state, if any.
			 * Does not copy the observers.
			 * Should be used mainly internally, to optimise multiple shots execution, copying the state from the simulator used for timing.
			 *
			 * @return A unique pointer to the cloned simulator.
			 */
			std::unique_ptr<ISimulator> Clone() override
			{
				auto cloned = std::make_unique<GpuSimulator>();

				cloned->simulationType = simulationType;
				cloned->nrQubits = nrQubits;

				cloned->limitSize = limitSize;
				cloned->limitEntanglement = limitEntanglement;
				cloned->chi = chi;
				cloned->singularValueThreshold = singularValueThreshold;

				if (state)
					cloned->state = state->Clone();
				else if (mps)
					cloned->mps = mps->Clone();

				return cloned;
			}
		};

	}
}

#endif
#endif
#endif
