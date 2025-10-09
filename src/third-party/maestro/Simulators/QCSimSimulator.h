/**
 * @file QCSimSimulator.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The qcsim simulator class.
 *
 * Should not be used directly, create an instance with the factory and use the generic interface instead.
 */

#pragma once

#ifndef _QCSIMSIMULATOR_H
#define _QCSIMSIMULATOR_H

#ifdef INCLUDED_BY_FACTORY

#include "QCSimState.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Simulators {
	// TODO: Maybe use the pimpl idiom https://en.cppreference.com/w/cpp/language/pimpl to hide the implementation for good
	// but during development this should be good enough
	namespace Private {

		class IndividualSimulator;

		/**
		 * @class QCSimSimulator
		 * @brief QCSim simulator class.
		 *
		 * This is the implementation for the qcsim simulator.
		 * Do not use this class directly, use the factory to create an instance.
		 * Only the interface should be exposed.
		 * @sa QCSimState
		 * @sa ISimulator
		 * @sa IState
		 */
		class QCSimSimulator : public QCSimState
		{
			friend class IndividualSimulator;
		public:
			QCSimSimulator() = default;
			// allow no copy or assignment
			QCSimSimulator(const QCSimSimulator&) = delete;
			QCSimSimulator& operator=(const QCSimSimulator&) = delete;

			// but allow moving
			QCSimSimulator(QCSimSimulator&& other) = default;
			QCSimSimulator& operator=(QCSimSimulator&& other) = default;

			/**
			 * @brief Applies a phase shift gate to the qubit
			 *
			 * Applies a specified phase shift gate to the qubit
			 * @param qubit The qubit to apply the gate to.
			 * @param lambda The phase shift angle.
			 */
			void ApplyP(Types::qubit_t qubit, double lambda) override
			{
				pgate.SetPhaseShift(lambda);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(pgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
				{
					if (std::abs(lambda - M_PI_2) > 1e-10)
						throw std::runtime_error("QCSimSimulator::ApplyP: Invalid phase shift angle for a Clifford gate.");
					cliffordSimulator->ApplyS(static_cast<unsigned int>(qubit));
				}
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(pgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(pgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(xgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyX(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(xgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(xgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(ygate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyY(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(ygate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(ygate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(zgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyZ(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(zgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(zgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(h, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyH(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(h, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(h, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(sgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyS(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(sgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(sgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(sdggate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplySdg(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(sdggate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(sdggate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(tgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyT: The stabilizer simulator does not support non-clifford gates.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(tgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(tgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(tdggate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyTDG: The stabilizer simulator does not support non-clifford gates.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(tdggate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(tdggate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(sxgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplySx(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(sxgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(sxgate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(sxdaggate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplySxDag(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(sxdaggate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(sxdaggate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(k, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyK(static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(k, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(k, static_cast<unsigned int>(qubit));
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
				rxgate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(rxgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyRx: The stabilizer simulator does not support the Rx gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(rxgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(rxgate, static_cast<unsigned int>(qubit));
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
				rygate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(rygate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyRy: The stabilizer simulator does not support the Ry gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(rygate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(rygate, static_cast<unsigned int>(qubit));
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
				rzgate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(rzgate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyRz: The stabilizer simulator does not support the Rz gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(rzgate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(rzgate, static_cast<unsigned int>(qubit));
				NotifyObservers({ qubit });
			}

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
			void ApplyU(Types::qubit_t qubit, double theta, double phi, double lambda, double gamma) override
			{
				ugate.SetParams(theta, phi, lambda, gamma);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(ugate, static_cast<unsigned int>(qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyU: The stabilizer simulator does not support the U gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(ugate, static_cast<unsigned int>(qubit));
				else
					state->ApplyGate(ugate, static_cast<unsigned int>(qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyCX(static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(cxgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(cygate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyCY(static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(cygate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(cygate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(czgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplyCZ(static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(czgate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(czgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				cpgate.SetPhaseShift(lambda);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(cpgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCP: The stabilizer simulator does not support the CP gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(cpgate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(cpgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				crxgate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(crxgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCRx: The stabilizer simulator does not support the CRx gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(crxgate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(crxgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				crygate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(crygate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCRy: The stabilizer simulator does not support the CRy gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(crygate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(crygate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				crzgate.SetTheta(theta);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(crzgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCRz: The stabilizer simulator does not support the CRz gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(crzgate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(crzgate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(ch, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCH: The stabilizer simulator does not support the CH gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(ch, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(ch, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCSx: The stabilizer simulator does not support the CSx gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(csx, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(csx, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(csxdag, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCSxDAG: The stabilizer simulator does not support the CSxDag gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(csxdag, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(csxdag, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(swapgate, static_cast<unsigned int>(qubit1), static_cast<unsigned int>(qubit0));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					cliffordSimulator->ApplySwap(static_cast<unsigned int>(qubit1), static_cast<unsigned int>(qubit0));
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(swapgate, static_cast<unsigned int>(qubit0), static_cast<unsigned int>(qubit1));
				else
					state->ApplyGate(swapgate, static_cast<unsigned int>(qubit1), static_cast<unsigned int>(qubit0));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
				{
					const size_t q1 = qubit0; // control 1
					const size_t q2 = qubit1; // control 2
					const size_t q3 = qubit2; // target

					// Sleator-Weinfurter decomposition
					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q1));
					mpsSimulator->ApplyGate(csxdag, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q1));
					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(q3), static_cast<unsigned int>(q1));
				}
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCCX: The stabilizer simulator does not support the CCX gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
				{
					const size_t q1 = qubit0; // control 1
					const size_t q2 = qubit1; // control 2
					const size_t q3 = qubit2; // target

					// Sleator-Weinfurter decomposition
					tensorNetwork->AddGate(csx, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));
					tensorNetwork->AddGate(csxdag, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));
					tensorNetwork->AddGate(csx, static_cast<unsigned int>(q1), static_cast<unsigned int>(q3));
				}
				else
					state->ApplyGate(ccxgate, static_cast<unsigned int>(qubit2), static_cast<unsigned int>(qubit1), static_cast<unsigned int>(qubit0));
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
				if (GetSimulationType() == SimulationType::kMatrixProductState)
				{
					const size_t q1 = ctrl_qubit; // control
					const size_t q2 = qubit0;
					const size_t q3 = qubit1; 

					// TODO: find a better decomposition
					// this one I've got with the qiskit transpiler
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q1));

					pgate.SetPhaseShift(M_PI);
					mpsSimulator->ApplyGate(pgate, static_cast<unsigned int>(q3));
					pgate.SetPhaseShift(-M_PI_2);
					mpsSimulator->ApplyGate(pgate, static_cast<unsigned int>(q2));

					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q1));

					pgate.SetPhaseShift(M_PI);
					mpsSimulator->ApplyGate(pgate, static_cast<unsigned int>(q3));

					mpsSimulator->ApplyGate(csx, static_cast<unsigned int>(q3), static_cast<unsigned int>(q1));
					mpsSimulator->ApplyGate(cxgate, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
				}
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCSwap: The stabilizer simulator does not support the CSwap gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
				{
					const size_t q1 = ctrl_qubit; // control
					const size_t q2 = qubit0;
					const size_t q3 = qubit1;

					// TODO: find a better decomposition
					// this one I've got with the qiskit transpiler
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
					tensorNetwork->AddGate(csx, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));

					pgate.SetPhaseShift(M_PI);
					tensorNetwork->AddGate(pgate, static_cast<unsigned int>(q3));
					pgate.SetPhaseShift(-M_PI_2);
					tensorNetwork->AddGate(pgate, static_cast<unsigned int>(q2));

					tensorNetwork->AddGate(csx, static_cast<unsigned int>(q2), static_cast<unsigned int>(q3));
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q1), static_cast<unsigned int>(q2));

					pgate.SetPhaseShift(M_PI);
					tensorNetwork->AddGate(pgate, static_cast<unsigned int>(q3));

					tensorNetwork->AddGate(csx, static_cast<unsigned int>(q1), static_cast<unsigned int>(q3));
					tensorNetwork->AddGate(cxgate, static_cast<unsigned int>(q3), static_cast<unsigned int>(q2));
				}
				else
					state->ApplyGate(cswapgate, static_cast<unsigned int>(qubit1), static_cast<unsigned int>(qubit0), static_cast<unsigned int>(ctrl_qubit));
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
				cugate.SetParams(theta, phi, lambda, gamma);
				if (GetSimulationType() == SimulationType::kMatrixProductState)
					mpsSimulator->ApplyGate(cugate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
				else if (GetSimulationType() == SimulationType::kStabilizer)
					throw std::runtime_error("QCSimSimulator::ApplyCU: The stabilizer simulator does not support the CU gate.");
				else if (GetSimulationType() == SimulationType::kTensorNetwork)
					tensorNetwork->AddGate(cugate, static_cast<unsigned int>(ctrl_qubit), static_cast<unsigned int>(tgt_qubit));
				else
					state->ApplyGate(cugate, static_cast<unsigned int>(tgt_qubit), static_cast<unsigned int>(ctrl_qubit));
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
				auto cloned = std::make_unique<QCSimSimulator>();

				cloned->simulationType = simulationType;
				cloned->nrQubits = nrQubits;
				cloned->limitSize = limitSize;
				cloned->limitEntanglement = limitEntanglement;
				cloned->chi = chi;
				cloned->singularValueThreshold = singularValueThreshold;
				cloned->enableMultithreading = enableMultithreading;
				cloned->useMPSMeasureNoCollapse = useMPSMeasureNoCollapse;

				if (state)
					cloned->state = state->Clone();

				if (mpsSimulator)
					cloned->mpsSimulator = mpsSimulator->Clone();

				if (cliffordSimulator)
					cloned->cliffordSimulator = cliffordSimulator->Clone();

				if (tensorNetwork)
					cloned->tensorNetwork = tensorNetwork->Clone();

				return cloned;
			}

		private:
			QC::Gates::PhaseShiftGate<> pgate;
			QC::Gates::PauliXGate<> xgate;
			QC::Gates::PauliYGate<> ygate;
			QC::Gates::PauliZGate<> zgate;
			QC::Gates::HadamardGate<> h;
			//QC::Gates::UGate<> ugate;
			QC::Gates::SGate<> sgate;
			QC::Gates::SDGGate<> sdggate;
			QC::Gates::TGate<> tgate;
			QC::Gates::TDGGate<> tdggate;
			QC::Gates::SquareRootNOTGate<> sxgate;
			QC::Gates::SquareRootNOTDagGate<> sxdaggate;
			QC::Gates::HyGate<> k;
			QC::Gates::RxGate<> rxgate;
			QC::Gates::RyGate<> rygate;
			QC::Gates::RzGate<> rzgate;
			QC::Gates::UGate<> ugate;
			QC::Gates::CNOTGate<> cxgate;
			QC::Gates::ControlledYGate<> cygate;
			QC::Gates::ControlledZGate<> czgate;
			QC::Gates::ControlledPhaseShiftGate<> cpgate;
			QC::Gates::ControlledRxGate<> crxgate;
			QC::Gates::ControlledRyGate<> crygate;
			QC::Gates::ControlledRzGate<> crzgate;
			QC::Gates::ControlledHadamardGate<> ch;
			QC::Gates::ControlledSquareRootNOTGate<> csx;
			QC::Gates::ControlledSquareRootNOTDagGate<> csxdag;
			QC::Gates::SwapGate<> swapgate;
			QC::Gates::ToffoliGate<> ccxgate;
			QC::Gates::FredkinGate<> cswapgate;
			QC::Gates::ControlledUGate<> cugate;
		};

	}
}

#endif

#endif // !_QCSIMSIMULATOR_H

