/**
 * @file Measurements.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Measurement operation class.
 * 
 * Implementation of an operation that does a qubit measurement on a simulator.
 * The results are stored in a classical 'state'.
 * Typically each qubit has a corresponding classical 'bit', but that does not need to be the case.
 */

#pragma once

#ifndef _MEASUREMENTS_H_
#define _MEASUREMENTS_H_

#include <vector>
#include "Operations.h"


namespace Circuits {

	/**
	 * @class MeasurementOperation
	 * @brief Measurement operation class.
	 *
	 * Implementation of an operation that does a qubits measurements on a simulator.
	 * The results are stored in a classical 'state'.
	 * Each qubit has a corresponding classical 'bit' where the measurement result is stored.
	 * @tparam Time The time type used for the simulation.
	 * @see IOperation
	 */
	template<typename Time = Types::time_type> class MeasurementOperation : public IOperation<Time> {
	public:
		/**
		 * @brief Constructor.
		 *
		 * Creates a measurement operation that measures the given qubits and stores the results in the given bits.
		 * @param qs The qubits to measure and the classical bits where the result is stored, specified as pairs.
		 * @param delay The duration of the operation.
		 * @sa IOperation
		 */
		MeasurementOperation(const std::vector<std::pair<Types::qubit_t, size_t>>& qs = {}, Time delay = 0)
			: IOperation<Time>(delay)
		{
			SetQubits(qs);
		}

		/**
		 * @brief Get the type of the operation.
		 *
		 * Returns the type of the operation, in this case, measurement.
		 * @return The type of the operation.
		 * @sa OperationType
		 */
		OperationType GetType() const override { return OperationType::kMeasurement; }

		/**
		 * @brief Get the number of qubits affected by the operation.
		 *
		 * Returns the number of qubits affected by the operation.
		 * @return The number of qubits affected by the operation.
		 */
		virtual size_t GetNumQubits() const
		{
			return qubits.size();
		}

		/**
		 * @brief Clears the qubits and classical bits involved in the measurement.
		 *
		 * Clears the qubits and classical bits involved in the measurement.
		 */
		void Clear()
		{
			qubits.clear();
			bits.clear();
		}

		/**
		 * @brief Sets the qubits and classical bits involved in the measurement.
		 *
		 * Sets the qubits and classical bits involved in the measurement, specified as pairs.
		 * @param qs The qubits to measure and the classical bits where the result is stored, specified as pairs.
		 */
		void SetQubits(const std::vector<std::pair<Types::qubit_t, size_t>>& qs)
		{
			qubits.resize(qs.size());
			bits.resize(qs.size());
			for (size_t i = 0; i < qs.size(); ++i)
			{
				qubits[i] = qs[i].first;
				bits[i] = qs[i].second;
			}
		}

		/**
		 * @brief Returns the qubits that are to be measured.
		 *
		 * Returns the qubits that are to be measured.
		 * @return The qubits to measure.
		 */
		const Types::qubits_vector& GetQubits() const
		{
			return qubits;
		}

		/**
		 * @brief Returns the classical bits where the measurement results are stored.
		 *
		 * Returns the classical bits where the measurement results are stored.
		 * @return The classical bits where the measurement results are stored.
		 */
		const std::vector<size_t>& GetBitsIndices() const
		{
			return bits;
		}

		/**
		 * @brief Executes the measurement on the given simulator.
		 *
		 * Executes the measurement on the given simulator.
		 * @param sim The simulator to execute the measurement on.
		 * @param state The classical state for the simulator.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			if (qubits.empty()) return;

			size_t res = sim->Measure(qubits);

			for (size_t i = 0; i < qubits.size(); ++i)
			{
				state.SetBit(bits[i], (res & 1) != 0);
				res >>= 1;
			}
		}

		/**
		 * @brief Samples the measurement on the given simulator.
		 *
		 * Samples the measurement on the given simulator.
		 * Should not apply the measurement.
		 * 
		 * @param sim The simulator to sample the measurement on.
		 * @param state The classical state for the simulator.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Sample(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const
		{
			if (qubits.empty()) return;

			size_t res = sim->SampleCounts(qubits, 1).begin()->first;

			for (auto bit : bits)
			{
				state.SetBit(bit, (res & 1) != 0);
				res >>= 1;
			}
		}

		void SetStateFromSample(size_t measurements, OperationState& state) const
		{
			if (qubits.empty()) return;

			for (size_t index = 0; index < qubits.size(); ++index)
			{
				const auto cbit = bits[index];
				const size_t qubitMask = 1ULL << index;
				state.SetBit(cbit, (measurements & qubitMask) != 0);
			}
		}


		void SetStateFromAllMeasurements(size_t allMeasurements, OperationState& state) const
		{
			if (qubits.empty()) return;

			for (size_t index = 0; index < qubits.size(); ++index)
			{
				const auto q = qubits[index];
				const auto cbit = bits[index];

				const size_t qubitMask = 1ULL << q;

				state.SetBit(cbit, (allMeasurements & qubitMask) != 0);
			}
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			std::vector<std::pair<Types::qubit_t, size_t>> qs(qubits.size());
			for (size_t i = 0; i < qubits.size(); ++i)
				qs[i] = std::make_pair(qubits[i], bits[i]);

			return std::make_shared<MeasurementOperation<Time>>(qs, IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Returns the qubits affected by the measurement.
		 *
		 * Returns the qubits affected by the measurement.
		 * @return The qubits affected by the measurement.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return qubits;
		}

		/**
		 * @brief Returns the classical bits affected by the measurement.
		 *
		 * Returns the classical bits affected by the measurement.
		 * @return The classical bits affected by the measurement.
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
			auto newOp = this->Clone();

			for (size_t i = 0; i < qubits.size(); ++i)
			{
				const auto qubitit = qubitsMap.find(qubits[i]);
				if (qubitit != qubitsMap.end())
					std::static_pointer_cast<MeasurementOperation<Time>>(newOp)->SetQubit(i, qubitit->second);
				//else throw std::runtime_error("MeasurementOperation::Remap: qubit not found in map.");
			}

			for (size_t i = 0; i < bits.size(); ++i)
			{
				const auto bitit = bitsMap.find(bits[i]);
				if (bitit != bitsMap.end())
					std::static_pointer_cast<MeasurementOperation<Time>>(newOp)->SetBit(i, bitit->second);
				//else throw std::runtime_error("MeasurementOperation::Remap: bit not found in map.");
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

		/**
		 * @brief Set the qubit to measure.
		 *
		 * This method sets the qubit to measure at the specified index.
		 * @param index The index of the qubit to measure.
		 * @param qubit The qubit to measure.
		 */
		void SetQubit(size_t index, Types::qubit_t qubit)
		{
			if (index < qubits.size())
				qubits[index] = qubit;
		}

		/**
		 * @brief Set the classical bit to index.
		 *
		 * This method sets the classical bit index at the specified index.
		 * @param index The index of the classical bit index.
		 * @param bit The classical bit index to set.
		 */
		void SetBit(size_t index, Types::qubit_t bit)
		{
			if (index < bits.size())
				bits[index] = bit;
		}

	private:
		Types::qubits_vector qubits; /**< The qubits to be measured */
		std::vector<size_t> bits;   /**< The classical bits where the measurement results are stored */
	};

}

#endif // !_MEASUREMENTS_H_