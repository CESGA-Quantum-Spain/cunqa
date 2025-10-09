/**
 * @file Converter.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The converter class.
 *
 * Some functions (like Clone, but not limited to) are returning a smart pointer to the IOperation interface/base class.
 * Sometimes conversion to a smart pointer to the derived class is needed. This is where the Converter class comes in.
 */

#pragma once

#ifndef _CIRCUITCONVERTER_H_
#define _CIRCUITCONVERTER_H_

#include "Circuit.h"

namespace Circuits {

	/**
	 * @class Converter 
	 * @brief The converter class.
	 * 
	 * Contains static functions to convert smart pointers to the IOperation interface/base class to smart pointers to the derived classes.
	 *
	 * @tparam Time The time type.
	 */
	template<typename Time = Types::time_type> class Converter {
	public:
		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the Circuit class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the Circuit class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<Circuit<Time>> ToCircuit(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kComposite)
				return nullptr;

			return Convert<Circuit<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the QuantumGate class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the QuantumGate class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<IQuantumGate<Time>> ToQuantumGate(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kGate)
				return nullptr;

			return Convert<IQuantumGate<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the SingleQubitGate class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the SingleQubitGate class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<SingleQubitGate<Time>> ToSingleQubitGate(const std::shared_ptr<IOperation<Time>>& operation)
		{
			auto op = ToQuantumGate(operation);
			if (!op || op->GetNumQubits() != 1) return nullptr;

			return Convert<SingleQubitGate<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the TwoQubitsGate class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the TwoQubitsGate class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<TwoQubitsGate<Time>> ToTwoQubitsGate(const std::shared_ptr<IOperation<Time>>& operation)
		{
			auto op = ToQuantumGate(operation);
			if (!op || op->GetNumQubits() != 2) return nullptr;

			return Convert<TwoQubitsGate<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the ThreeQubitsGate class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the ThreeQubitsGate class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<ThreeQubitsGate<Time>> ToThreeQubitsGate(const std::shared_ptr<IOperation<Time>>& operation)
		{
			auto op = ToQuantumGate(operation);
			if (!op || op->GetNumQubits() != 3) return nullptr;

			return Convert<ThreeQubitsGate<Time>>(operation);
		}

		// TODO: Add for specific quantum gates

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the Random class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the Random class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<Random<Time>> ToRandom(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kRandomGen)
				return nullptr;

			return Convert<Random<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the Reset class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the Reset class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<Reset<Time>> ToReset(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kReset)
				return nullptr;

			return Convert<Reset<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the MeasurementOperation class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the MeasurementOperation class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<MeasurementOperation<Time>> ToMeasurement(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kMeasurement)
				return nullptr;

			return Convert<MeasurementOperation<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the ConditionalGate class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the ConditionalGate class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<ConditionalGate<Time>> ToConditionalGate(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kConditionalGate)
				return nullptr;

			return Convert<ConditionalGate<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the ConditionalMeasurement class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the ConditionalMeasurement class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<ConditionalMeasurement<Time>> ToConditionalMeasurement(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kConditionalMeasurement)
				return nullptr;

			return Convert<ConditionalMeasurement<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the IOperation interface/base class to a smart pointer to the ConditionalRandomGen class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the ConditionalRandomGen class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<ConditionalRandomGen<Time>> ToConditionalRandom(const std::shared_ptr<IOperation<Time>>& operation)
		{
			if (!operation || operation->GetType() != OperationType::kConditionalRandomGen)
				return nullptr;

			return Convert<ConditionalRandomGen<Time>>(operation);
		}

		/**
		 * Converts a smart pointer to the ICondition interface/base class to a smart pointer to the EqualCondition class.
		 *
		 * @param operation The operation.
		 * @return The smart pointer to the EqualCondition class or a null pointer if the conversion is not possible.
		 */
		static std::shared_ptr<EqualCondition> ToEqualCondition(const std::shared_ptr<ICondition>& cond)
		{
			if (!cond)
				return nullptr;

			return std::static_pointer_cast<EqualCondition>(cond);
		}
		
	private:
		template<typename T> static std::shared_ptr<T> Convert(const std::shared_ptr<IOperation<Time>>& operation)
		{
			return std::static_pointer_cast<T>(operation);
		}
	};

}

#endif
