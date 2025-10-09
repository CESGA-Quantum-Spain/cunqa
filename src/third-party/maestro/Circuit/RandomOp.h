/**
 * @file RandomOp.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Random operation implementation.
 * 
 * Implements an operation that can generate random classical bits that are stored in the classical state/register.
 */

#pragma once

#ifndef _RANDOM_OP_H_
#define _RANDOM_OP_H_

#include <random>
#include "Operations.h"


namespace Circuits {

	/**
	 * @class Random 
	 * @brief Random operation.
	 *
	 * Implements an operation that can generate random classical bits that are stored in the classical state/register.
	 * @tparam Time The data type used for operation time.
	 * @see IOperation
	 */
	template<typename Time = Types::time_type> class Random : public IOperation<Time> {
	public:
		/**
		 * @brief Construct a new Random object.
		 *
		 * Constructs a new Random object.
		 * @param ind The indices of the classical bits to be set by the generator.
		 * @param seed The seed used for the random number generator.
		 * @param delay The excution time.
		 */
		Random(const std::vector<size_t>& ind = {}, size_t seed = 0, Time delay = 0)
			: IOperation<Time>(delay), indices(ind), s(seed)
		{
			Seed(seed);
		}

		/**
		 * @brief Execute the operation.
		 *
		 * Executes the operation.
		 * @param sim The simulator to be used for the execution.
		 * @param state The state of the simulator.
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			for (size_t i : indices)
				state.SetBit(i, dist_bool(rng));
		}

		/**
		 * @brief Get the type of the operation.
		 *
		 * Returns the type of the operation, in this case, random.
		 * @return The type of the operation.
		 */
		OperationType GetType() const override
		{
			return OperationType::kRandomGen;
		}

		/**
		 * @brief Get the indices of the classical bits affected by this operation.
		 *
		 * Returns the indices of the classical bits affected by this operation.
		 * @return The indices of the classical bits affected by this operation.
		 */
		const std::vector<size_t>& GetBitsIndices() const
		{
			return indices;
		}

		/**
		 * @brief Set the indices of the classical bits affected by this operation.
		 *
		 * Sets the indices of the classical bits affected by this operation.
		 * @param ind The indices of the classical bits affected by this operation.
		 */
		void SetBitsIndices(const std::vector<size_t>& ind)
		{
			indices = ind;
		}

		/**
		 * @brief Seeds the random generator.
		 *
		 * Seeds the random generator using the specified seed.
		 * @param sd The seed to be used.
		 */
		void Seed(size_t sd)
		{
			s = sd;
			std::seed_seq seed{ uint32_t(s & 0xffffffff), uint32_t(s >> 32) };
			rng.seed(seed);
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<Random<Time>>(GetBitsIndices(), s, IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Get the affected bits.
		 *
		 * Returns the indices of the classical bits affected by this operation.
		 * @return The indices of the bits affected by this operation.
		 */
		std::vector<size_t> AffectedBits() const override
		{
			return GetBitsIndices();
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
			auto newOp = Clone();

			for (size_t i = 0; i < indices.size(); ++i)
			{
				const auto bitit = bitsMap.find(indices[i]);
				if (bitit != bitsMap.end())
					std::static_pointer_cast<Random<Time>>(newOp)->SetBit(i, bitit->second);
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
		 * @brief Set the classical bit to index.
		 *
		 * This method sets the classical bit index at the specified index.
		 * @param index The index of the classical bit index.
		 * @param bit The classical bit index to set.
		 */
		void SetBit(size_t index, Types::qubit_t bit)
		{
			if (index < indices.size())
				indices[index] = bit;
		}

	private:
		std::vector<size_t> indices;             /**< The indices of the classical bits to be set when the random values are generated */
		mutable std::mt19937_64 rng;                     /**< The random number generator */
		mutable std::bernoulli_distribution dist_bool;   /**< The distribution used to generate random bits - only 0 and 1 values */
		size_t s;								 /**< The seed used for the random number generator */
	};

}

#endif // !_RANDOM_OP_H_
