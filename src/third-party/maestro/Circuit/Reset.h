/**
 * @file Reset.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Reset operation.
 * 
 * This operation can be used to reset qubits. 
 * It's a measurement of the qubit(s) followed by an X gate conditioned by the result of the measurement.
 * The measurement result is then discarded.
 */

#pragma once

#ifndef _RESET_STATE_H_
#define _RESET_STATE_H_

#include "QuantumGates.h"

namespace Circuits {

	/**
	 * @class Reset
	 * @brief Reset operation class.
	 *
	 * This operation can be used to reset qubits.
	 * @tparam Time The time type used for operation timing.
	 * @sa IOperation
	 */
	template<typename Time = Types::time_type> class Reset : public IOperation<Time> {
	public:
		/**
		 * @brief Construct a new Reset object.
		 *
		 * This constructor creates a new Reset operation.
		 * @param qubits The qubits to reset.
		 * @param delay The execution time of the operation.
		 * @param resetTargets Specifies what the qubits should be reset to (false |0>, true |1>, not specified |0>).
		 */
		Reset(const Types::qubits_vector& qubits = {}, Time delay = 0, const std::vector<bool>& resetTargets = {})
			: IOperation<Time>(delay), qubits(qubits), resetTargets(resetTargets)
		{
		}

		/**
		 * @brief Execute the operation.
		 *
		 * This method executes the reset operation. It's done by a measurement that is then discarded, followed by an X gate applied if needed.
		 * @param sim The simulator to execute the operation on.
		 * @param state The current classical state.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			size_t res = sim->Measure(qubits);

			for (size_t qi = 0; res && qi < qubits.size(); ++qi)
			{
				if ((res & 1) == (resetTargets.size() <= qi ? 1 : (resetTargets[qi] ? 0 : 1)))
				{
					xgate.SetQubit(qubits[qi]);
					xgate.Execute(sim, state);
				}

				res >>= 1;
			}
		}

		/**
		 * @brief Get the type of operation.
		 *
		 * This method returns the type of operation.
		 * @return The type of operation, reset type.
		 * @sa OperationType
		 */
		OperationType GetType() const override
		{
			return OperationType::kReset;
		}

		/**
		 * @brief Set the qubits to reset and the values to reset them to.
		 *
		 * This method sets the qubits to reset and the values to reset them to.
		 * @param qs The qubits to reset.
		 * @param resetTgts Specifies what the qubits should be reset to (false |0>, true |1>, not specified |0>).
		 */
		void SetQubits(const Types::qubits_vector& qs, const std::vector<bool>& resetTgts = {})
		{
			qubits = qs;
			resetTargets = resetTgts;
		}

		/**
		 * @brief Get the qubits to reset.
		 *
		 * This method returns the qubits to reset.
		 * @return The qubits to reset.
		 */
		const Types::qubits_vector& GetQubits() const
		{
			return qubits;
		}

		/**
		 * @brief Get the values to reset the qubits to.
		 *
		 * This method returns the values to reset the qubits to.
		 * false means |0>, true means |1>, not specified means |0>.
		 * @return The values to reset the qubits to.
		 */
		const std::vector<bool>& GetResetTargets() const
		{
			return resetTargets;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<Reset<Time>>(GetQubits(), IOperation<Time>::GetDelay(), GetResetTargets());
		}

		/**
		 * @brief Get the qubits affected by this operation.
		 *
		 * This method returns the qubits affected by this operation, that is, the reset qubits.
		 * @return The qubits affected by this operation.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return qubits;
		}

		/**
		 * @brief Get a shared pointer to a remapped operation.
		 *
		 * Returns a shared pointer to a copy of the operation with qubits and classical bits changed according to the provided maps.
		 *
		 * @param qubitsMap The map of qubits to remap.
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped object.
		 */
		std::shared_ptr<IOperation<Time>> Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& qubitsMap, const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap = {}) const override
		{
			auto newOp = this->Clone();

			for (size_t i = 0; i < qubits.size(); ++i)
			{
				const auto qubitit = qubitsMap.find(qubits[i]);
				if (qubitit != qubitsMap.end())
					std::static_pointer_cast<Reset<Time>>(newOp)->SetQubit(i, qubitit->second);
			}

			return newOp;
		}

		/**
		 * @brief Checks if the operation is a Clifford one.
		 *
		 * Checks if the operation is a Clifford one, allowing to be simulated in a stabilizers simulator.
		 *
		 * @return True if it can be applied in a stabilizers simulator, false otherwise.
		 */
		bool IsClifford() const override
		{
			return true;
		}

	protected:
		/**
		 * @brief Set the qubit to reset.
		 *
		 * This method sets the qubit to reset at the specified index.
		 * @param index The index of the qubit to reset.
		 * @param qubit The qubit to reset.
		 */
		void SetQubit(size_t index, Types::qubit_t qubit)
		{
			if (index < qubits.size())
				qubits[index] = qubit;
		}

	private:
		Types::qubits_vector qubits;    /**< The qubits to be reset */
		std::vector<bool> resetTargets; /**< The values to reset the qubits to */
		mutable XGate<Time> xgate;			    /**< The X gate used to change the qubit state after its measurement, if needed */
	};

}

#endif // !_RESET_STATE_H_

