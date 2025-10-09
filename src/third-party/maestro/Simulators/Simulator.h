/**
 * @file Simulator.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The interface for a simulator.
 * 
 * This interface extends the state interface allowing more operations to be performed on the state,
 * more specifically, the application of quantum gates.
 */

#pragma once

#ifndef _SIMULATOR_INTERFACE_H_
#define _SIMULATOR_INTERFACE_H_

#include "State.h"

namespace Simulators {

	/**
	 * @class ISimulator
	 * @brief Interface class for a quantum computing simulator.
	 *
	 * This is the interface that exposes the functionality of the quantum computing simulators.
	 * Use them wrapped in shared pointers.
	 * @sa IState
	 * @sa AerSimulator
	 * @sa QCSimSimulator
	 */
	class ISimulator : public IState, std::enable_shared_from_this<ISimulator>
	{
	public:
		/**
		 * @brief Applies a phase shift gate to the qubit
		 *
		 * Applies a specified phase shift gate to the qubit
		 * @param qubit The qubit to apply the gate to.
		 * @param lambda The phase shift angle.
		 */
		virtual void ApplyP(Types::qubit_t qubit, double lambda) = 0;

		/**
		 * @brief Applies a not gate to the qubit
		 *
		 * Applies a not (X) gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyX(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Y gate to the qubit
		 *
		 * Applies a not (Y) gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyY(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Z gate to the qubit
		 *
		 * Applies a not (Z) gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyZ(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Hadamard gate to the qubit
		 *
		 * Applies a Hadamard gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyH(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a S gate to the qubit
		 *
		 * Applies a S gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyS(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a S dagger gate to the qubit
		 *
		 * Applies a S dagger gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplySDG(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a T gate to the qubit
		 *
		 * Applies a T gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyT(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a T dagger gate to the qubit
		 *
		 * Applies a T dagger gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyTDG(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Sx gate to the qubit
		 *
		 * Applies a Sx gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplySx(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Sx dagger gate to the qubit
		 *
		 * Applies a Sx dagger gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplySxDAG(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a K gate to the qubit
		 *
		 * Applies a K (Hy) gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 */
		virtual void ApplyK(Types::qubit_t qubit) = 0;

		/**
		 * @brief Applies a Rx gate to the qubit
		 *
		 * Applies an x rotation gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 * @param theta The rotation angle.
		 */
		virtual void ApplyRx(Types::qubit_t qubit, double theta) = 0;

		/**
		 * @brief Applies a Ry gate to the qubit
		 *
		 * Applies a y rotation gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 * @param theta The rotation angle.
		 */
		virtual void ApplyRy(Types::qubit_t qubit, double theta) = 0;

		/**
		 * @brief Applies a Rz gate to the qubit
		 *
		 * Applies a z rotation gate to the specified qubit
		 * @param qubit The qubit to apply the gate to.
		 * @param theta The rotation angle.
		 */
		virtual void ApplyRz(Types::qubit_t qubit, double theta) = 0;

		/**
		 * @brief Applies a U gate to the qubit
         *
         * Applies a U gate to the specified qubit
         * @param qubit The qubit to apply the gate to.
         * @param theta The first parameter.
		 * @param phi The second parameter.
		 * @param lambda The third parameter.
		 * @param gamma The fourth parameter.
         */
		virtual void ApplyU(Types::qubit_t qubit, double theta, double phi, double lambda, double gamma) = 0;

		/**
		 * @brief Applies a CX gate to the qubits
		 *
		 * Applies a controlled X gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCX(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a CY gate to the qubits
		 *
		 * Applies a controlled Y gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCY(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a CZ gate to the qubits
		 *
		 * Applies a controlled Z gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCZ(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a CP gate to the qubits
		 *
		 * Applies a controlled phase gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 * @param lambda The phase shift angle.
		 */
		virtual void ApplyCP(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double lambda) = 0;

		/**
		 * @brief Applies a CRx gate to the qubits
		 *
		 * Applies a controlled x rotation gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 * @param theta The rotation angle.
		 */
		virtual void ApplyCRx(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) = 0;

		/**
		 * @brief Applies a CRy gate to the qubits
		 *
		 * Applies a controlled y rotation gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 * @param theta The rotation angle.
		 */
		virtual void ApplyCRy(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) = 0;

		/**
		 * @brief Applies a CRz gate to the qubits
		 *
		 * Applies a controlled z rotation gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 * @param theta The rotation angle.
		 */
		virtual void ApplyCRz(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta) = 0;

		/**
		 * @brief Applies a CH gate to the qubits
		 *
		 * Applies a controlled Hadamard gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCH(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a CSx gate to the qubits
		 *
		 * Applies a controlled squared root not gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCSx(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a CSx dagger gate to the qubits
		 *
		 * Applies a controlled squared root not dagger gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param tgt_qubit The target qubit
		 */
		virtual void ApplyCSxDAG(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit) = 0;

		/**
		 * @brief Applies a swap gate to the qubits
		 *
		 * Applies a swap gate to the specified qubits
		 * @param qubit0 The first qubit
		 * @param qubit1 The second qubit
		 */
		virtual void ApplySwap(Types::qubit_t qubit0, Types::qubit_t qubit1) = 0;

		/**
		 * @brief Applies a controlled controlled not gate to the qubits
		 *
		 * Applies a controlled controlled not gate to the specified qubits
		 * @param qubit0 The first control qubit
		 * @param qubit1 The second control qubit
		 * @param qubit2 The target qubit
		 */
		virtual void ApplyCCX(Types::qubit_t qubit0, Types::qubit_t qubit1, Types::qubit_t qubit2) = 0;

		/**
		 * @brief Applies a controlled swap gate to the qubits
		 *
		 * Applies a controlled swap gate to the specified qubits
		 * @param ctrl_qubit The control qubit
		 * @param qubit0 The first qubit
		 * @param qubit1 The second qubit
		 */
		virtual void ApplyCSwap(Types::qubit_t ctrl_qubit, Types::qubit_t qubit0, Types::qubit_t qubit1) = 0;

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
		virtual void ApplyCU(Types::qubit_t ctrl_qubit, Types::qubit_t tgt_qubit, double theta, double phi, double lambda, double gamma) = 0;

		/**
         * @brief Applies a nop
         *
		 * Applies a nop (no operation).
		 * Typically does (almost) nothing. Equivalent to an identity.
		 * For qiskit aer it will send the 'nop' to the qiskit aer simulator.
         */
		virtual void ApplyNop() = 0;

		/**
		 * @brief Clones the simulator.
		 * 
		 * Clones the simulator, including the state, the configuration and the internally saved state, if any.
		 * Does not copy the observers.
		 * Should be used mainly internally, to optimise multiple shots execution, copying the state from the simulator used for timing.
		 * 
		 * @return A unique pointer to the cloned simulator.
		 */
		virtual std::unique_ptr<ISimulator> Clone() = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<ISimulator> getptr()
		{
			return shared_from_this();
		}
	};
}

#endif // !_SIMULATOR_INTERFACE_H_

