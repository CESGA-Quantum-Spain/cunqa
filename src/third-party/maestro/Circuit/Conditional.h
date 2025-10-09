/**
 * @file Conditional.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Conditional operations.
 *
 * Contains conditional operations that can be executed on a simulator.
 * Those are classically controlled operations, based on conditions from a 'state' (typically the result of a measurement).
 * The most important one are the conditional quantum gates and the conditional measurements, but others are also possible if needed.
 */

#pragma once

#ifndef _CONDITIONAL_H_
#define _CONDITIONAL_H_

#include "QuantumGates.h"
#include "Measurements.h"
#include "RandomOp.h"

namespace Circuits {

	/**
	 * @class ICondition
	 * @brief Interface for a condition.
	 *
	 * Interface for a condition that can be used to control the execution of an operation.
	 * The condition is based on a classical state (typically the result of a measurement).
	 */
	class ICondition : public std::enable_shared_from_this<ICondition> {
	public:
		/**
		 * @brief Construct a new ICondition object.
		 *
		 * Construct a new ICondition object with the given indices.
		 * @param ind The indices of the bits that are used in the condition.
		 */
		ICondition(const std::vector<size_t>& ind) : indices(ind)
		{
		}

		/**
		 * @brief Virtual destructor.
		 *
		 * Destroy the ICondition object.
		 * It's a virtual destructor because this class is an abstract class that needs to be derived from.
		 */
		virtual ~ICondition() = default;

		/**
		 * @brief Check if the condition is met.
		 *
		 * Check if the condition is met on the given classical state.
		 * This function must be implemented by the derived classes.
		 * @param state The state to check the condition against.
		 * @return True if the condition is met, false otherwise.
		 */
		virtual bool IsConditionMet(OperationState& state) const = 0;

		/**
		 * @brief Get the indices of the bits used in the condition.
		 *
		 * Get the indices of the classical bits used in the condition.
		 * @return The indices of the bits used in the condition.
		 */
		const std::vector<size_t>& GetBitsIndices() const
		{
			return indices;
		}

		/**
		 * @brief Set the indices of the bits used in the condition.
		 *
		 * Set the indices of the classical bits used in the condition.
		 * @param ind The indices of the bits used in the condition.
		 */
		void SetBitsIndices(const std::vector<size_t>& ind)
		{
			indices = ind;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		virtual std::shared_ptr<ICondition> Clone() const = 0;

		/**
		 * @brief Get a shared pointer to a remapped condition.
		 *
		 * Returns a shared pointer to a copy of the condition with classical bits changed according to the provided map.
		 * 
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped object.
		 */
		virtual std::shared_ptr<ICondition> Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap = {}) const = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<ICondition> getptr()
		{
			return std::enable_shared_from_this<ICondition>::shared_from_this();
		}

	private:
		std::vector<size_t> indices; /**< The indices of the bits used in the condition. */
	};


	/**
	 * @class EqualCondition
	 * @brief Condition that checks if the bits are equal to a given value.
	 *
	 * Condition that checks if the bits are equal to a given value.
	 * @tparam Time The type of the time parameter that is used for operations timing.
	 * @sa ICondition
	 */
	class EqualCondition : public ICondition {
	public:
		/**
		 * @brief Construct a new EqualCondition object.
		 *
		 * Construct a new EqualCondition object with the given classical bits indices and classical bits values.
		 * @param ind The indices of the bits that are used in the condition.
		 * @param b The values to compare the bits to.
		 */
		EqualCondition(const std::vector<size_t>& ind, const std::vector<bool>& b)
			: ICondition(ind), bits(b)
		{
		}

		/**
		 * @brief Check if the condition is met.
		 *
		 * Check if the condition is met on the given classical state.
		 * @param state The state to check the condition against.
		 * @return True if the condition is met, false otherwise.
		 */
		bool IsConditionMet(OperationState& state) const override
		{
			return state.GetBits(GetBitsIndices()) == GetAllBits();
		}

		/**
		 * @brief Get the values to compare the bits to.
		 *
		 * Get the values to compare the classical bits to.
		 * @return The values to compare the bits to.
		 */
		const std::vector<bool>& GetAllBits() const
		{
			return bits;
		}

		/**
		 * @brief Set the values to compare the bits to.
		 *
		 * Set the values to compare the classical bits to.
		 * @param b The values to compare the bits to.
		 */
		void SetBits(const std::vector<bool>& b)
		{
			bits = b;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<ICondition> Clone() const override
		{
			return std::make_shared<EqualCondition>(GetBitsIndices(), GetAllBits());
		}

		/**
		 * @brief Get a shared pointer to a remapped condition.
		 *
		 * Returns a shared pointer to a copy of the condition with classical bits changed according to the provided map.
		 * 
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped object.
		 */
		std::shared_ptr<ICondition> Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap = {}) const override
		{
			auto newCond = this->Clone();

			auto newBits = newCond->GetBitsIndices();
			for (size_t i = 0; i < newBits.size(); ++i)
			{
				const auto bitit = bitsMap.find(newBits[i]);
				if (bitit != bitsMap.end())
					newBits[i] = bitit->second;
				//else throw std::invalid_argument("Conditional operation: bit not found in the map, couldn't remap.");
			}
			newCond->SetBitsIndices(newBits);

			return newCond;
		}

	private:
		std::vector<bool> bits; /**< The values to compare the classical bits to. */
	};

	/**
	 * @class IConditionalOperation
	 * @brief An operation conditioned on classical values.
	 *
	 * An operation that is executed only if a condition is met.
	 * @tparam Time The type of the time parameter that is used for operations timing.
	 * @sa IConditionalOperation
	 * @sa IOperation
	 */
	template<typename Time = Types::time_type> class IConditionalOperation : public IOperation<Time> {
	public:
		/**
		 * @brief Construct a new IConditionalOperation object.
		 *
		 * Construct a new IConditionalOperation object with the given operation, condition and delay.
		 * @param operation The operation to execute if the condition is met.
		 * @param condition The condition to check.
		 * @param delay The delay of the operation.
		 * @sa IOperation
		 * @sa ICondition
		 */
		IConditionalOperation(const std::shared_ptr<IOperation<Time>>& operation, const std::shared_ptr<ICondition>& condition, Time delay = 0) :
			IOperation<Time>(delay), operation(operation), condition(condition)
		{
		}

		/**
		 * @brief Execute the operation.
		 *
		 * Execute the operation if the condition is met with the specified classical state, in the specified simulator.
		 * @param sim The simulator to execute the operation on.
		 * @param state The classical state to execute the operation on (check condition against it and if it has a classical result, put the results there).
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			if (!condition || !operation) return;

			if (condition->IsConditionMet(state))
				operation->Execute(sim, state);
		}

		/**
		 * @brief Set the operation for the conditional operation.
		 *
		 * Assigns the operation to execute if the condition is met.
		 * @param op The assigned operation.
		 */
		void SetOperation(const std::shared_ptr<IOperation<Time>>& op)
		{
			if (!op) return;
			// the reason why 'recursive' conditional gates are not allowed (as in conditional-conditional-...-gate) is because of the distribution for the network
			// execution would work fine as long as it's 'local'
			else if ((op->GetType() == OperationType::kGate && this->GetType() != OperationType::kConditionalGate) ||
				(op->GetType() == OperationType::kMeasurement && this->GetType() != OperationType::kConditionalMeasurement) ||
				(op->GetType() == OperationType::kRandomGen && this->GetType() != OperationType::kConditionalRandomGen))
				return;

			operation = op;
		}

		/**
		 * @brief Get the operation for the conditional operation.
		 *
		 * Get the operation to execute if the condition is met.
		 * @return The operation to execute if the condition is met.
		 * @sa IOperation
		 */
		std::shared_ptr<IOperation<Time>> GetOperation() const
		{
			return operation;
		}

		/**
		 * @brief Set the condition for the conditional operation.
		 *
		 * Assigns the condition to check.
		 * @param cond The assigned condition.
		 * @sa ICondition
		 */
		void SetCondition(const std::shared_ptr<ICondition>& cond)
		{
			condition = cond;
		}

		/**
		 * @brief Get the condition for the conditional operation.
		 *
		 * Get the condition to check.
		 * @return The condition to check.
		 * @sa ICondition
		 */
		std::shared_ptr<ICondition> GetCondition() const
		{
			return condition;
		}

		/**
		 * @brief Get bits that are involved in the condition.
		 *
		 * Get the classical bits that are involved in the condition.
		 * @return The bits involved.
		 */
		std::vector<size_t> AffectedBits() const override
		{
			if (!condition) return {};

			return condition->GetBitsIndices();
		}

		/**
		 * @brief Get the qubits affected by the operation.
		 *
		 * Get the qubits affected by the operation.
		 * @return The qubits affected by the operation.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			if (!operation) return {};

			return operation->AffectedQubits();
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
			const auto condOp = std::static_pointer_cast<IConditionalOperation<Time>>(this->Clone());

			condOp->SetCondition(condOp->GetCondition()->Remap(bitsMap));
			condOp->SetOperation(condOp->GetOperation()->Remap(qubitsMap, bitsMap));

			return condOp;
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
			if (!operation) return true;

			return operation->IsClifford();
		}

	private:
		std::shared_ptr<IOperation<Time>> operation; /**< The operation to execute if the condition is met. */
		std::shared_ptr<ICondition> condition;       /**< The condition to check. */
	};

	/**
	 * @class ConditionalGate
	 * @brief A conditional quantum gate.
	 * 
	 * A quantum gate that is executed only if a condition is met.
	 * @tparam Time The type of the time parameter that is used for operations timing.
	 * @sa IConditionalOperation
	 */
	template<typename Time = Types::time_type> class ConditionalGate : public IConditionalOperation<Time>
	{
	public:
		/**
		 * @brief Construct a new ConditionalGate object.
		 * 
		 * Construct a new ConditionalGate object with the given gate, condition and delay.
		 * @param operation The gate to execute if the condition is met.
		 * @param condition The condition to check.
		 * @param delay The delay of the gate.
		 * @sa IConditionalOperation
		 * @sa IGateOperation
		 * @sa ICondition
		 */
		ConditionalGate(const std::shared_ptr<IGateOperation<Time>>& operation, const std::shared_ptr<ICondition>& condition, Time delay = 0)
			: IConditionalOperation<Time>(operation, condition, delay)
		{
		}

		/**
		 * @brief Get the type of the operation.
		 * 
		 * Returns the type of the operation.
		 * @return The type of the operation.
		 * @sa OperationType
		 */
		OperationType GetType() const override
		{
			return OperationType::kConditionalGate;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			const auto gate = std::static_pointer_cast<IGateOperation<Time>>(IConditionalOperation<Time>::GetOperation()->Clone());
			const auto cond = std::static_pointer_cast<ICondition>(IConditionalOperation<Time>::GetCondition()->Clone());

			return std::make_shared<ConditionalGate<Time>>(gate, cond, IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class ConditionalMeasurement
	 * @brief A conditional measurement.
	 *
	 * A measurement that is executed only if a condition is met.
	 * @tparam Time The type of the time parameter that is used for operations timing.
	 * @sa IConditionalOperation
	 */
	template<typename Time = Types::time_type> class ConditionalMeasurement : public IConditionalOperation<Time>
	{
	public:
		/**
		 * @brief Construct a new ConditionalMeasurement object.
		 *
		 * Construct a new ConditionalMeasurement object with the given measurement, condition and delay.
		 * @param operation The measurement to execute if the condition is met.
		 * @param condition The condition to check.
		 * @param delay The time required for the operation to complete.
		 * @sa IConditionalOperation
		 * @sa MeasurementOperation
		 * @sa ICondition
		 */
		ConditionalMeasurement(const std::shared_ptr<MeasurementOperation<Time>>& operation, const std::shared_ptr<ICondition>& condition, Time delay = 0)
			: IConditionalOperation<Time>(operation, condition, delay)
		{
		}

		/**
		 * @brief Get the type of the operation.
		 *
		 * Returns the type of the operation.
		 * @return The type of the operation.
		 * @sa OperationType
		 */
		OperationType GetType() const override
		{
			return OperationType::kConditionalMeasurement;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<ConditionalMeasurement<Time>>(std::static_pointer_cast<MeasurementOperation<Time>>(IConditionalOperation<Time>::GetOperation()->Clone()), std::static_pointer_cast<ICondition>(IConditionalOperation<Time>::GetCondition()->Clone()), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class ConditionalRandomGen
	 * @brief A conditional random generator.
	 *
	 * A random generator that is executed only if a condition is met.
	 * @tparam Time The type of the time parameter that is used for operations timing.
	 * @sa IConditionalOperation
	 */
	template<typename Time = Types::time_type> class ConditionalRandomGen : public IConditionalOperation<Time>
	{
	public:
		/**
		 * @brief Construct a new ConditionalRandomGen object.
		 *
		 * Construct a new ConditionalRandomGen object with the given random generator, condition and delay.
		 * @param operation The random generator to execute if the condition is met.
		 * @param condition The condition to check.
		 * @param delay The time required for the operation to complete.
		 * @sa IConditionalOperation
		 * @sa Random
		 * @sa ICondition
		 */
		ConditionalRandomGen(const std::shared_ptr<Random<Time>>& operation, const std::shared_ptr<ICondition>& condition, Time delay = 0)
			: IConditionalOperation<Time>(operation, condition, delay)
		{
		}

		/**
		 * @brief Get the type of the operation.
		 *
		 * Returns the type of the operation.
		 * @return The type of the operation.
		 * @sa OperationType
		 */
		OperationType GetType() const override
		{
			return OperationType::kConditionalRandomGen;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<ConditionalRandomGen<Time>>(std::static_pointer_cast<Random<Time>>(IConditionalOperation<Time>::GetOperation()->Clone()), std::static_pointer_cast<ICondition>(IConditionalOperation<Time>::GetCondition()->Clone()), IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Get the quantum bits affected by the operation.
		 *
		 * Get the quantum bits affected by the operation.
		 * Returns an empty vector as the operation is not affecting any qubit.
		 * Implementation is required by the interface.
		 * @return An empty vector as the operation is not affecting any qubit.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return {};
		}
	};

}

#endif // !_CONDITIONAL_H_
