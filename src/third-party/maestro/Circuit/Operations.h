/**
 * @file Operations.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The state class that stores the classical state (such as measurement results) of a quantum circuit execution.
 * The operation interface along with a 'no op' implementation of it and a generic gate interface (all gate operations should be derived from it).
 */

#pragma once

#ifndef _CIRCUIT_OPERATIONS_H_
#define _CIRCUIT_OPERATIONS_H_

#include "../Simulators/Simulator.h"
#include "../Types.h"

namespace Circuits {
	/**
	 * @enum OperationType
	 * @brief The type of operations.
	 */
	enum class OperationType {
		kGate,					  /**< the usual quantum gate, result stays in simulator's state */
		kMeasurement,             /**< measurement, result in 'OperationState' */
		kRandomGen,               /**< random classical bit generator, result in 'OperationState' */
		kConditionalGate,         /**< conditional gate, similar with gate, but conditioned on something from 'OperationState' */
		kConditionalMeasurement,  /**< conditional measurement, similar with measurement, but conditioned on something from 'OperationState' */
		kConditionalRandomGen,    /**< conditional random generator, similar with random gen, but conditioned on something from 'OperationState' */
		kReset,                   /**< reset, no result in 'state', just apply measurement, then apply not on all qubits that were measured as 1, the measurements are then discarded */
		kNoOp,                    /**< no operation, just a placeholder, could be used to erase some operation from a circuit */
		kComposite                /**< a composite operation, contains other operations - should not be used in the beginning, only the 'Circuit' is a composite for now */
	};


	// simulators keep track of the quantum state, but there are things that are not part of it
	// that need keeping around (for example, the result of a measurement, or if it's a 'random generation' operation, the generated number), so here it is
	// the values in there are needed either for the end result or for the next operation (conditional gate, conditional measurement)

	/**
	 * @class OperationState
	 * @brief The state class that stores the classical state of a quantum circuit execution.
	 * 
	 * Contains the classical state (such as measurement results and classical values for conditions) of a quantum circuit execution.
	 */
	class OperationState {
	public:
		/**
		 * @brief Construct a new Operation State object
		 * 
		 * Pass the number of bits to allocate. If not specified, the default is 0, they can be allocated later.
		 * @param numBits The number of bits to allocate.
		 */
		OperationState(size_t numBits = 0)
		{
			AllocateBits(numBits);
		}

		/**
		 * @brief Construct a new Operation State object
		 *
		 * Copy constructor.
		 * 
		 * @param other The OperationState object to copy.
		 */
		OperationState(const OperationState& other)
			: bits(other.bits)
		{
		}

		/**
		 * @brief Construct a new Operation State object
		 *
		 * Move constructor.
		 *
		 * @param other The OperationState object to move.
		 */
		OperationState(OperationState&& other) noexcept
			: bits(std::move(other.bits))
		{
		}

		/**
		 * @brief Construct a new Operation State object
		 *
		 * Construct a new Operation State object with the specified bits.
		 * @param bits The bits to set.
		 */
		OperationState(const std::vector<bool>& b)
			: bits(b)
		{
		}

		/**
		 * @brief Construct a new Operation State object
		 *
		 * Construct a new Operation State object with the specified bits.
		 * @param bits The bits to set.
		 */
		OperationState(std::vector<bool>&& b)
			: bits(std::move(b))
		{
		}

		/**
		 * @brief Assign the bits.
		 *
		 * Assign the bits.
		 * @param other The OperationState object to copy.
		 */
		OperationState& operator=(const OperationState& other)
		{
			bits = other.bits;
			return *this;
		}

		/**
		 * @brief Assign the bits.
		 *
		 * Assign the bits.
		 * @param other The OperationState object to move.
		 */
		OperationState& operator=(OperationState&& other) noexcept
		{
			bits.swap(other.bits);
			return *this;
		}

		/**
		 * @brief Assign the bits.
		 *
		 * Assign the bits.
		 * @param bits The bits to set.
		 */
		OperationState& operator=(const std::vector<bool>& b)
		{
			bits = b;
			return *this;
		}

		/**
		 * @brief Assign the bits.
		 *
		 * Assign the bits.
		 * @param bits The bits to set.
		 */
		OperationState& operator=(std::vector<bool>&& b)
		{
			bits.swap(b);
			return *this;
		}

		/**
		 * @brief Allocate more bits.
		 * 
		 * Allocate more bits, the number of classical bits is increased by the number of bits specified.
		 * @param numBits The number of bits to suplementary allocate.
		 */
		void AllocateBits(size_t numBits)
		{
			bits.resize(bits.size() + numBits, false);
		}

		/**
		 * @brief Clear the classical state.
		 * 
		 * Clear the classical state, the number of allocated bits is reset to 0.
		 */
		void Clear()
		{
			bits.clear();
		}

		/**
		 * @brief Get the number of classical bits.
		 * 
		 * Get the number of allocated classical bits.
		 * @return The number of classical bits.
		 */
		size_t GetNumBits() const
		{
			return bits.size();
		}

		/**
		 * @brief Get the classical bit at the specified index.
		 * 
		 * Get the value of the classical bit at the specified index.
		 * @param index The index of the bit to get.
		 * @return The classical bit value at the specified index.
		 */
		bool GetBit(size_t index) const
		{
			if (index > bits.size()) return false;

			return bits[index];
		}

		/**
		 * @brief Get the classical bits at the specified indices.
		 * 
		 * Get the values of the classical bits at the specified indices.
		 * @param indices The indices of the bits to get.
		 * @return The classical bit values at the specified indices.
		 */
		std::vector<bool> GetBits(const std::vector<size_t>& indices) const
		{
			std::vector<bool> results(indices.size(), false);

			for (size_t i = 0; i < indices.size(); ++i)
				if (indices[i] < bits.size())
					results[i] = bits[indices[i]];

			return results;
		}

		/**
		 * @brief Get the classical bits.
		 * 
		 * Get the values of the classical bits.
		 * @return The classical bit values reference.
		 */
		const std::vector<bool>& GetAllBits() const
		{
			return bits;
		}

		/**
		 * @brief Get the classical bits.
		 *
		 * Get the values of the classical bits.
		 * @return A copy of the classical bit values.
		 */
		std::vector<bool> GetAllBitsCopy() const
		{
			return bits;
		}

		/**
		 * @brief Set the classical bit at the specified index.
		 * 
		 * Set the value of the classical bit at the specified index.
		 * @param index The index of the bit to set.
		 * @param value The value to set the bit to.
		 */
		void SetBit(size_t index, bool value = true)
		{
			if (index > bits.size()) return;

			bits[index] = value;
		}

		/**
		 * @brief Set the classical bits with the specified value.
		 * 
		 * Set the values of the classical bits with the specified value.
		 * If not specified, the default value is false.
		 * @param value The value assigned to the classical bits.
		 */
		void Reset(bool value = false)
		{
			std::fill(bits.begin(), bits.end(), value);
		}
		
		/**
         * @brief Set the classical bits.
         *
         * Set the values of the classical bits to the values specified in results.
         * Each bit in results specifies a value.
         * @param results A vector of bools that contains the values for the bits.
         */
		void SetResultsInOrder(const std::vector<bool>& results)
		{
			const size_t theSize = std::max(bits.size(), results.size());

			bits = results;
			bits.resize(theSize, false);
		}

		/**
		 * @brief Set the classical bits at the specified indices.
		 * 
		 * Set the values of the classical bits at the specified indices to the values specified in results.
		 * Each bit in results specifies a value.
		 * @param indices The indices of the bits to set.
		 * @param results The value that contains the values for the bits.
		 */
		void SetResults(const std::vector<size_t>& indices, size_t results)
		{
			for (int i = 0; i < static_cast<int>(indices.size()); ++i)
			{
				if (indices[i] < bits.size())
					bits[indices[i]] = results & 1;
				results >>= 1;
			}
		}

		/**
		 * @brief Set results.
		 * 
		 * Set the values of the classical bits.
		 * Destroys the source, it's implemented for increasing performance.
		 * 
		 * @param results The new values.
		 */
		void Swap(OperationState& results)
		{
			bits.swap(results.bits);
		}

		/**
		 * @brief Convert the state using the provided mapping.
		 *
		 * Set the values of the classical bits.
		 * 
		 * @param mapping The mapping of the bits.
		 * @param ignoreNotMapped If true, the bits that are not in the mapping are ignored. The size of the state will be the size of the mapping.
		 * @param newSize The new size of the state. If zero, the size of the state is used. Ignored if ignoreNotMapped is true.
		 * @param offset The offset to add to the remapping.
		 */
		void Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& mapping, bool ignoreNotMapped = false, size_t newSize = 0)
		{
			std::vector<bool> newBits(newSize > 0 ? newSize : (ignoreNotMapped ? mapping.size() : bits.size()), false);

			for (size_t i = 0; i < bits.size(); ++i)
			{
				const auto it = mapping.find(i);
				if (it != mapping.end())
					newBits[it->second] = bits[i];
				else if (!ignoreNotMapped && i < newBits.size())
					newBits[i] = bits[i];
			}

			bits.swap(newBits);
		}

		/**
		 * @brief Convert the state using the provided mapping.
		 *
		 * Set the values of the classical bits.
		 *
		 * @param mapping The mapping of the bits.
		 * @param ignoreNotMapped If true, the bits that are not in the mapping are ignored. The size of the state will be the size of the mapping.
		 * @param newSize The new size of the state. If zero, the size of the state is used. Ignored if ignoreNotMapped is true.
		 * @param offset The offset to add to the remapping.
		 */
		void RemapWithVector(const std::vector<Types::qubit_t>& mapping, bool ignoreNotMapped = false, size_t newSize = 0)
		{
			std::vector<bool> newBits(newSize > 0 ? newSize : (ignoreNotMapped ? mapping.size() : bits.size()), false);

			for (size_t i = 0; i < bits.size(); ++i)
			{
				if (i < mapping.size())
					newBits[mapping.at(i)] = bits[i];
				else if (!ignoreNotMapped && i < newBits.size())
					newBits[i] = bits[i];
			}

			bits.swap(newBits);
		}

	private:
		std::vector<bool> bits; /**< The classical bits. */
	};

	/**
	 * @class IOperation
	 * @brief The operation interface.
	 * 
	 * The operation interface along with implementation of some generic functions that apply to all operations.
	 * @tparam Time The type of the time parameter.
	 * @sa IOperation
	 */
	template<typename Time = Types::time_type> class IOperation : public std::enable_shared_from_this<IOperation<Time>> {
	public:
		/**
		 * @brief Construct a new IOperation object.
		 * 
		 * Construct a new IOperation object with the specified execution time.
		 * @param delay The execution time of the operation.
		 */
		IOperation(Time delay = 0) : delay(delay) {};
		
		/**
		 * @brief Destroy the IOperation object.
		 * 
		 * Destroy the IOperation object.
		 * It's a virtual destructor because this class is an abstract class that needs to be derived from.
		 */
		virtual ~IOperation() = default;
		
		/**
		 * @brief Execute the operation.
		 * 
		 * Execute the operation on the specified simulator.
		 * @param sim The simulator to execute the operation on.
		 * @param state The state of the operation.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		virtual void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const = 0;
		
		/**
		 * @brief Get the type of the operation.
		 * 
		 * Get the type of the operation.
		 * @return The type of the operation.
		 * @sa OperationType
		 */
		virtual OperationType GetType() const
		{
			return OperationType::kNoOp;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		virtual std::shared_ptr<IOperation<Time>> Clone() const = 0;

		/**
		 * @brief Get a shared pointer to a remapped operation.
		 *
		 * Returns a shared pointer to a copy of the operation with qubits and classical bits changed according to the provided maps.
		 *
		 * @param qubitsMap The map of qubits to remap.
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped object.
		 */
		virtual std::shared_ptr<IOperation<Time>> Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& qubitsMap, const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap = {}) const = 0;

		/**
		 * @brief Find if the operation can affect the quantum state.
		 *
		 * Returns true if the operation can affect the quantum state, false otherwise.
		 * @return True if the operation can affect the quantum state, false otherwise.
		 */
		virtual bool CanAffectQuantumState() const
		{
			return GetType() == OperationType::kGate || GetType() == OperationType::kMeasurement ||
				GetType() == OperationType::kConditionalGate || GetType() == OperationType::kConditionalMeasurement ||
				GetType() == OperationType::kReset || GetType() == OperationType::kComposite;
		}

		/**
		 * @brief Find if the operation needs entanglement for distribution.
		 *
		 * Returns true if the operation needs entanglement for distribution, false otherwise.
		 * @return True if the operation needs entanglement for distribution, false otherwise.
		 */
		virtual bool NeedsEntanglementForDistribution() const
		{
			return (GetType() == OperationType::kGate || GetType() == OperationType::kConditionalGate) && AffectedQubits().size() > 1;
		}

		/**
		 * @brief Find if the operation is a conditional operation.
		 *
		 * Returns true if the operation is conditional, false otherwise.
		 * @return True if the operation is conditional, false otherwise.
		 */
		virtual bool IsConditional() const
		{
			return GetType() == OperationType::kConditionalGate || GetType() == OperationType::kConditionalMeasurement || GetType() == OperationType::kConditionalRandomGen;
		}

		/**
		 * @brief Returns the affected qubits.
		 *
		 * Returns the affected qubits by the operation.
		 * @return A vector with the affected qubits.
		 */
		virtual Types::qubits_vector AffectedQubits() const
		{
			return {};
		}

		/**
		 * @brief Returns the affected bits.
		 *
		 * Returns the affected classical bits.
		 * @return The affected bits.
		 */
		virtual std::vector<size_t> AffectedBits() const
		{
			return {};
		}

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> getptr()
		{
			return std::enable_shared_from_this<IOperation<Time>>::shared_from_this();
		}

		/**
		 * @brief Get the delay of the operation.
		 *
		 * Returns the delay due of the execution time of the operation.
		 * @return The delay of the operation.
		 */
		Time GetDelay() const
		{
			return delay;
		}

		/**
		 * @brief Set the delay of the operation.
		 *
		 * Sets the delay due of the execution time of the operation.
		 * @param d The delay of the operation.
		 */
		void SetDelay(Time d)
		{
			delay = d;
		}

		/**
		 * @brief Checks if the operation is a Clifford one.
		 *
		 * Checks if the operation is a Clifford one, allowing to be simulated in a stabilizers simulator.
		 * 
		 * @return True if it can be applied in a stabilizers simulator, false otherwise.
		 */
		virtual bool IsClifford() const
		{
			return false;
		}

	private:
		Time delay; /**< The delay of the operation. */
	};

	/**
	 * @class NoOperation
	 * @brief The no operation.
	 *
	 * This a an operation that does nothing.
	 * @tparam Time The type of the delay time parameter.
	 * @sa IOperation
	 */
	template<typename Time = Types::time_type> class NoOperation : public IOperation<Time>
	{
	public:
		/**
		 * @brief Construct a new NoOperation object.
		 *
		 * Construct a new NoOperation object with the specified execution time.
		 * @param delay The execution time of the operation.
		 */
		NoOperation(Time delay = 0) : IOperation<Time>(delay) {};

		/**
		 * @brief Execute the operation.
		 *
		 * Execute the operation on the specified simulator.
		 * The execution does nothing, except adding the delay for the execution if a discrete event simulator is used.
		 * To not be confused with the simulator passed here as a parameter, which is a quantum simulator.
		 * @param sim The simulator to execute the operation on.
		 * @param state The state of the operation.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override {}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override { return std::make_shared<NoOperation<Time>>(NoOperation<Time>::GetDelay()); }

		/**
		 * @brief Get a shared pointer to a remapped operation.
		 *
		 * Returns a shared pointer to a copy of the operation with qubits and classical bits changed according to the provided maps.
		 *
		 * @param qubitsMap The map of qubits to remap.
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped object.
		 */
		std::shared_ptr<IOperation<Time>> Remap(const std::unordered_map<Types::qubit_t, Types::qubit_t>& qubitsMap, const std::unordered_map<Types::qubit_t, Types::qubit_t>& bitsMap) const override
		{
			return this->Clone();
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
	};

	/**
	 * @class IGateOperation
	 * @brief The gate operation interface.
	 * 
	 * The gate operation interface, this is the base class for all gate operations.
	 * @tparam Time The type of the delay time parameter.
	 * @sa IOperation
	 */
	template<typename Time = Types::time_type> class IGateOperation : public IOperation<Time> {
	public:
		/**
		 * @brief Construct a new IGateOperation object.
		 *
		 * Construct a new IGateOperation object with the specified execution time.
		 * @param delay The execution time of the operation.
		 */
		IGateOperation(Time delay = 0) : IOperation<Time>(delay) {};

		/**
		 * @brief Get the type of the operation.
		 *
		 * Get the type of the operation, in this case, gate.
		 * @return The type of the operation, gate in this case.
		 * @sa OperationType
		 */
		OperationType GetType() const override { return OperationType::kGate; }

		/**
		 * @brief Get the number of qubits.
		 *
		 * Get the number of qubits the gate operates on.
		 * @return The number of qubits the gate operates on.
		 */
		virtual unsigned int GetNumQubits() const = 0;

		/**
		 * @brief Set the qubits involved.
		 *
		 * Set the qubits involved.
		 * @param qubit The qubit to set.
		 * @param index The index of the qubit to set (0 for 1 qubit gates, 0 and 1 for two qubit gates, 0, 1 and 3 for three qubit gates).
		 */
		virtual void SetQubit(Types::qubit_t qubit, unsigned long index = 0) = 0;

		/**
		 * @brief Get the qubit involved.
		 *
		 * Get the qubit involved.
		 * @param index The index of the qubit to get (0, 1, or 2, the maximum depending on the number of qubits of the gate).
		 * @return The qubit involved.
		 */
		virtual Types::qubit_t GetQubit(unsigned int index = 0) const = 0;
	};

}

#endif // !_CIRCUIT_OPERATIONS_H_
