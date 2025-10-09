/**
 * @file QuantumGates.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The quantum gates interfaces and implementations.
 * 
 * There are one qubit, two qubits and three qubits gates interfaces, along with implementations for specific gates.
 * The QuantumGateType enum is used to identify the type of gate.
 */

#pragma once

#ifndef _QUANTUM_GATES_H_
#define _QUANTUM_GATES_H_

#include "Operations.h"

namespace Circuits {

	/**
	 * @enum QuantumGateType
	 * @brief The type of quantum gates.
	 */
	enum class QuantumGateType : int {
		kPhaseGateType = 0,
		kXGateType,
		kYGateType,
		kZGateType,
		kHadamardGateType,
		kSGateType,
		kSdgGateType,
		kTGateType,
		kTdgGateType,
		kSxGateType,
		kSxDagGateType,
		kKGateType,
		kRxGateType,
		kRyGateType,
		kRzGateType,
		kUGateType,
		kSwapGateType,
		kCXGateType,
		kCYGateType,
		kCZGateType,
		kCPGateType,
		kCRxGateType,
		kCRyGateType,
		kCRzGateType,
		kCHGateType,
		kCSxGateType,
		kCSxDagGateType,
		kCUGateType,
		kCSwapGateType,
		kCCXGateType
	};

	/**
	 * @class IQuantumGate
	 * @brief The interface for quantum gates.
	 * 
	 * The interface for quantum gates. Quantum gates are quantum operations that can be applied to a quantum state.
	 * @tparam Time The type of the execution delay.
	 * @sa IGateOperation
	 */
	template<typename Time = Types::time_type> class IQuantumGate : public IGateOperation<Time> {
	public:
		/**
		 * @brief IQuantumGate constructor.
		 *
		 * Constructs the IQuantumGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @sa IGateOperation
		 */
		IQuantumGate(Time delay = 0) : IGateOperation<Time>(delay) {}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		virtual QuantumGateType GetGateType() const = 0;

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate, empty if there are no parameters.
		 */
		virtual std::vector<double> GetParams() const { return {}; }
	};

	/**
	 * @class SingleQubitGate
	 * @brief The interface for single qubit quantum gates.
	 *
	 * The interface for single qubit quantum gates. Single qubit quantum gates are quantum operations that can be applied to a single qubit.
	 * This class is abstract and will be derived from.
	 * @tparam Time The type of the execution delay.
	 * @sa IQuantumGate
	 */
	template<typename Time = Types::time_type> class SingleQubitGate : public IQuantumGate<Time> {
	public:
		/**
		 * @brief SingleQubitGate constructor.
		 *
		 * Constructs the SingleQubitGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa IQuantumGate
		 */
		SingleQubitGate(Types::qubit_t qubit = 0, Time delay = 0) : IQuantumGate<Time>(delay), qubit(qubit) {}

		/**
		 * @brief The SingleQubitGate destructor.
		 * 
		 * The SingleQubitGate destructor, virtual because this is an abstract class that will be derived from.
		 */
		virtual ~SingleQubitGate() {}

		/**
		 * @brief Get the number of qubits the quantum gate is applied to.
		 *
		 * Returns the number of qubits the quantum gate is applied to, in this case 1.
		 * @return The number of qubits the quantum gate is applied to: 1.
		 */
		unsigned int GetNumQubits() const override { return 1; }

		/**
		 * @brief Set the qubit the quantum gate is applied to.
		 *
		 * Sets the qubit the quantum gate is applied to.
		 * Either specify zero for the index or leave it default.
		 * @param q The qubit the quantum gate is applied to.
		 * @param index The index of the qubit to set (ignored, default 0).
		 */
		void SetQubit(Types::qubit_t q, unsigned long index = 0) override
		{
			qubit = q;
		}

		/**
		 * @brief Get the qubit the quantum gate is applied to.
		 *
		 * Returns the qubit the quantum gate is applied to.
		 * Either specify zero for the index or leave it default.
		 * The parameter value is ignored anyway.
		 * @param index The index of the qubit to get (ignored, default 0).
		 * @return The qubit the quantum gate is applied to.
		 */
		Types::qubit_t GetQubit(unsigned int index = 0) const override
		{
			return qubit;
		}

		/**
		 * @brief Get the qubits the quantum gate is applied to.
		 *
		 * Returns the qubits the quantum gate is applied to, in this case a single qubit.
		 * @return The qubits the quantum gate is applied to.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return {qubit};
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
			auto newGate = this->Clone();

			const auto qubitit = qubitsMap.find(qubit);
			if (qubitit != qubitsMap.end())
				std::static_pointer_cast<SingleQubitGate<Time>>(newGate)->SetQubit(qubitit->second);
			//else throw std::invalid_argument("Qubit not found in qubits map to remap the gate.");

			return newGate;
		}

	private:
		Types::qubit_t qubit; /**< The qubit the quantum gate is applied to. */
	};


	/**
	 * @class TwoQubitsGate
	 * @brief The interface for two qubits quantum gates.
	 *
	 * The interface for two qubits quantum gates. Two qubits quantum gates are quantum operations that can be applied to two qubits.
	 * @tparam Time The type of the execution delay.
	 * @sa IQuantumGate
	 */
	template<typename Time = Types::time_type> class TwoQubitsGate : public IQuantumGate<Time> {
	public:
		/**
		 * @brief TwoQubitsGate constructor.
		 *
		 * Constructs the TwoQubitsGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * This gate is abstract and will be derived from.
		 * @param qubit1 The first qubit the quantum gate is applied to.
		 * @param qubit2 The second qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa IQuantumGate
		 */
		TwoQubitsGate(Types::qubit_t qubit1 = 0, Types::qubit_t qubit2 = 0, Time delay = 0) : IQuantumGate<Time>(delay), qubit1(qubit1), qubit2(qubit2) {}

		/**
		 * @brief The TwoQubitsGate destructor.
		 * 
		 * The TwoQubitsGate destructor, virtual because this is an abstract class that will be derived from.
		 */
		virtual ~TwoQubitsGate() {}

		/**
		 * @brief Get the number of qubits the quantum gate is applied to.
		 *
		 * Returns the number of qubits the quantum gate is applied to, in this case 2.
		 * @return The number of qubits the quantum gate is applied to: 2.
		 */
		unsigned int GetNumQubits() const override { return 2; }

		/**
		 * @brief Set the qubit the quantum gate is applied to.
		 *
		 * Sets the qubit the quantum gate is applied to.
		 * Either specify zero or one for the index or leave it default (0).
		 * @param q The qubit the quantum gate is applied to.
		 * @param index The index of the qubit to set (0 or 1, default value 0).
		 */
		void SetQubit(Types::qubit_t q, unsigned long index = 0) override
		{
			if (index == 0) qubit1 = q;
			else if (index == 1) qubit2 = q;
		}

		/**
		 * @brief Get the qubit the quantum gate is applied to.
		 *
		 * Returns the qubit the quantum gate is applied to.
		 * Either specify zero or one for the index or leave it default (0).
		 * If anything else is used, it will return UINT_MAX.
		 * @param index The index of the qubit to get (0 or 1, default value 0).
		 * @return The qubit the quantum gate is applied to.
		 */
		Types::qubit_t GetQubit(unsigned int index = 0) const override
		{
			if (index == 0) return qubit1;
			else if (index == 1) return qubit2;

			return UINT_MAX;
		}

		/**
		 * @brief Get the qubits the quantum gate is applied to.
		 *
		 * Returns the qubits the quantum gate is applied to, in this case two qubits.
		 * @return The qubits the quantum gate is applied to.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return {qubit1, qubit2};
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
			auto newGate = this->Clone();

			const auto qubitit1 = qubitsMap.find(qubit1);
			if (qubitit1 != qubitsMap.end())
				std::static_pointer_cast<TwoQubitsGate<Time>>(newGate)->SetQubit(qubitit1->second, 0);

			const auto qubitit2 = qubitsMap.find(qubit2);
			if (qubitit2 != qubitsMap.end())
				std::static_pointer_cast<TwoQubitsGate<Time>>(newGate)->SetQubit(qubitit2->second, 1);

			return newGate;
		}

	private:
		Types::qubit_t qubit1; /**< The first qubit the quantum gate is applied to. */
		Types::qubit_t qubit2; /**< The second qubit the quantum gate is applied to. */
	};

	/**
	 * @class ThreeQubitsGate
	 * @brief The interface for three qubits quantum gates.
	 *
	 * The interface for three qubits quantum gates. Three qubits quantum gates are quantum operations that can be applied to three qubits.
	 * @tparam Time The type of the execution delay.
	 * @sa IQuantumGate
	 */
	template<typename Time = Types::time_type> class ThreeQubitsGate : public IQuantumGate<Time> {
	public:
		/**
		 * @brief ThreeQubitsGate constructor.
		 *
		 * Constructs the ThreeQubitsGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * This gate is abstract and will be derived from.
		 * @param qubit1 The first qubit the quantum gate is applied to.
		 * @param qubit2 The second qubit the quantum gate is applied to.
		 * @param qubit3 The third qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa IQuantumGate
		 */
		ThreeQubitsGate(Types::qubit_t qubit1 = 0, Types::qubit_t qubit2 = 0, Types::qubit_t qubit3 = 0, Time delay = 0)
			: IQuantumGate<Time>(delay), qubit1(qubit1), qubit2(qubit2), qubit3(qubit3) {}

		/**
		* @brief The ThreeQubitsGate destructor.
		*
		*The ThreeQubitsGate destructor, virtual because this is an abstract class that will be derived from.
		*/
		virtual ~ThreeQubitsGate() {}

		/**
		 * @brief Get the number of qubits the quantum gate is applied to.
		 *
		 * Returns the number of qubits the quantum gate is applied to, in this case 3.
		 * @return The number of qubits the quantum gate is applied to: 3.
		 */
		unsigned int GetNumQubits() const override { return 3; }

		/**
		 * @brief Set the qubit the quantum gate is applied to.
		 *
		 * Sets the qubit the quantum gate is applied to.
		 * Either specify zero, one or two for the index or leave it default (0).
		 * @param q The qubit the quantum gate is applied to.
		 * @param index The index of the qubit to set (0, 1 or 2, default value 0).
		 */
		void SetQubit(Types::qubit_t q, unsigned long index = 0) override
		{
			if (index == 0) qubit1 = q;
			else if (index == 1) qubit2 = q;
			else if (index == 2) qubit3 = q;
		}

		/**
		 * @brief Get the qubit the quantum gate is applied to.
		 *
		 * Returns the qubit the quantum gate is applied to.
		 * Either specify zero, one or two for the index or leave it default (0).
		 * If anything else is used, it will return UINT_MAX.
		 * @param index The index of the qubit to get (0, 1 or 2, default value 0).
		 * @return The qubit the quantum gate is applied to.
		 */
		Types::qubit_t GetQubit(unsigned int index = 0) const override
		{
			if (index == 0) return qubit1;
			else if (index == 1) return qubit2;
			else if (index == 2) return qubit3;

			return UINT_MAX;
		}

		/**
		 * @brief Get the qubits the quantum gate is applied to.
		 *
		 * Returns the qubits the quantum gate is applied to, in this case three qubits.
		 * @return The qubits the quantum gate is applied to.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			return {qubit1, qubit2, qubit3};
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
			auto newGate = this->Clone();

			const auto qubitit1 = qubitsMap.find(qubit1);
			if (qubitit1 != qubitsMap.end())
				std::static_pointer_cast<ThreeQubitsGate<Time>>(newGate)->SetQubit(qubitit1->second, 0);

			const auto qubitit2 = qubitsMap.find(qubit2);
			if (qubitit2 != qubitsMap.end())
				std::static_pointer_cast<ThreeQubitsGate<Time>>(newGate)->SetQubit(qubitit2->second, 1);

			const auto qubitit3 = qubitsMap.find(qubit3);
			if (qubitit3 != qubitsMap.end())
				std::static_pointer_cast<ThreeQubitsGate<Time>>(newGate)->SetQubit(qubitit3->second, 2);

			return newGate;
		}

	private:
		Types::qubit_t qubit1; /**< The first qubit the quantum gate is applied to. */
		Types::qubit_t qubit2; /**< The second qubit the quantum gate is applied to. */
		Types::qubit_t qubit3; /**< The third qubit the quantum gate is applied to. */
	};

	//**********************************************************************************************
	// Single Qubit Gates
	//**********************************************************************************************

	/**
	 * @class PhaseGate
	 * @brief The phase gate.
	 * 
	 * The phase gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class PhaseGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief PhaseGate constructor.
		 *
		 * Constructs the PhaseGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param lambda The lambda parameter for the phase gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		PhaseGate(Types::qubit_t qubit = 0, double lambda = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay), lambda(lambda) {}

		/**
		 * @brief Execute the quantum gate.
		 * 
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyP(SingleQubitGate<Time>::GetQubit(), lambda);
		}

		/**
		 * @brief Set the lambda parameter for the phase gate.
		 *
		 * Sets the lambda parameter for the phase gate.
		 * @param l The lambda parameter for the phase gate.
		 */
		void SetLambda(double l) { lambda = l; }
		
		/**
		 * @brief Get the lambda parameter for the phase gate.
		 *
		 * Returns the lambda parameter for the phase gate.
		 * @return The lambda parameter for the phase gate.
		 */
		double GetLambda() const { return lambda; }

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kPhaseGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<PhaseGate<Time>>(SingleQubitGate<Time>::GetQubit(), lambda, IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { lambda }; }

		/**
		 * @brief Checks if the operation is a Clifford one.
		 *
		 * Checks if the operation is a Clifford one, allowing to be simulated in a stabilizers simulator.
		 *
		 * @return True if it can be applied in a stabilizers simulator, false otherwise.
		 */
		bool IsClifford() const override
		{
			if (std::abs(lambda - M_PI_2) > 1e-10)
				return false;

			return true;
		}

	private:
		double lambda;
	};


	/**
	 * @class XGate
	 * @brief The X gate.
	 *
	 * The X gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class XGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief XGate constructor.
		 *
		 * Constructs the XGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		XGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyX(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kXGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<XGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class YGate
	 * @brief The Y gate.
	 *
	 * The Y gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class YGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief YGate constructor.
		 *
		 * Constructs the YGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		YGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyY(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kYGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<YGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class ZGate
	 * @brief The Z gate.
	 *
	 * The Z gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class ZGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief ZGate constructor.
		 *
		 * Constructs the ZGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		ZGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyZ(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kZGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<ZGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class HadamardGate
	 * @brief The Hadamard gate.
	 *
	 * The Hadamard gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class HadamardGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief HadamardGate constructor.
		 *
		 * Constructs the HadamardGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		HadamardGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyH(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kHadamardGateType;
		}
		
		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<HadamardGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class SGate
	 * @brief The S gate.
	 *
	 * The S gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class SGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief SGate constructor.
		 *
		 * Constructs the SGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		SGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyS(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kSGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<SGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class SdgGate
	 * @brief The S dagger gate.
	 *
	 * The S dagger gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class SdgGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief SdgGate constructor.
		 *
		 * Constructs the SdgGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		SdgGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplySDG(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kSdgGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<SdgGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class TGate
	 * @brief The T gate.
	 *
	 * The T gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class TGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief TGate constructor.
		 *
		 * Constructs the TGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		TGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyT(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kTGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<TGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class TdgGate
	 * @brief The T dagger gate.
	 *
	 * The T dagger gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class TdgGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief TdgGate constructor.
		 *
		 * Constructs the TdgGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		TdgGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyTDG(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kTdgGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<TdgGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
		}
	};

	
	/**
	 * @class SxGate
	 * @brief The Sx gate.
	 *
	 * The Sx gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class SxGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief SxGate constructor.
		 *
		 * Constructs the SxGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		SxGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplySx(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kSxGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<SxGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class SxDagGate
	 * @brief The Sx dagger gate.
	 *
	 * The Sx dagger gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class SxDagGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief SxDagGate constructor.
		 *
		 * Constructs the SxDagGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		SxDagGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplySxDAG(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kSxDagGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<SxDagGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class KGate
	 * @brief The K gate.
	 *
	 * The K gate.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class KGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief KGate constructor.
		 *
		 * Constructs the KGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		KGate(Types::qubit_t qubit = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyK(SingleQubitGate<Time>::GetQubit());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kKGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<KGate<Time>>(SingleQubitGate<Time>::GetQubit(), IOperation<Time>::GetDelay());
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
	 * @class RotationGate
	 * @brief The rotation gate.
	 *
	 * The rotation gate. Must be overriden for specific rotations.
	 * @tparam Time The type of the execution delay.
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class RotationGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief RotationGate constructor.
		 *
		 * Constructs the RotationGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * This class should not be used directly, it's a base class for specific rotation gates classes.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param theta The theta angle for the rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		RotationGate(Types::qubit_t qubit = 0, double theta = 0, Time delay = 0) : SingleQubitGate<Time>(qubit, delay), theta(theta) {}

		/**
		 * @brief Set the theta angle for the rotation gate.
		 *
		 * Sets the theta angle for the rotation gate.
		 * @param t The theta angle for the rotation gate.
		 */
		void SetTheta(double t) { theta = t; }
		
		/**
		 * @brief Get the theta angle for the rotation gate.
		 *
		 * Returns the theta angle for the rotation gate.
		 * @return The theta angle for the rotation gate.
		 */
		double GetTheta() const { return theta; }

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { theta }; }

	private:
		double theta; /**< The theta angle for the rotation gate. */
	};

	/**
	 * @class RxGate
	 * @brief The Rx gate.
	 *
	 * The x rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa RotationGate	
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class RxGate : public RotationGate<Time> {
	public:
		/**
		 * @brief RxGate constructor.
		 *
		 * Constructs the RxGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param theta The theta angle for the rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		RxGate(Types::qubit_t qubit = 0, double theta = 0, Time delay = 0) : RotationGate<Time>(qubit, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyRx(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kRxGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<RxGate<Time>>(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class RyGate
	 * @brief The Ry gate.
	 *
	 * The y rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa RotationGate
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class RyGate : public RotationGate<Time> {
	public:
		/**
		 * @brief RyGate constructor.
		 *
		 * Constructs the RyGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param theta The theta angle for the rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		RyGate(Types::qubit_t qubit = 0, double theta = 0, Time delay = 0) : RotationGate<Time>(qubit, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyRy(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kRyGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<RyGate<Time>>(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class RzGate
	 * @brief The Rz gate.
	 *
	 * The z rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa RotationGate
	 * @sa SingleQubitGate
	 */
	template<typename Time = Types::time_type> class RzGate : public RotationGate<Time> {
	public:
		/**
		 * @brief RzGate constructor.
		 *
		 * Constructs the RzGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param theta The theta angle for the rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		RzGate(Types::qubit_t qubit = 0, double theta = 0, Time delay = 0) : RotationGate<Time>(qubit, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyRz(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kRzGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<RzGate<Time>>(SingleQubitGate<Time>::GetQubit(), RotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
     * @class UGate
     * @brief The U gate.
     *
     * The U gate.
     * @tparam Time The type of the execution delay.
     * @sa SingleQubitGate
     */
	template<typename Time = Types::time_type> class UGate : public SingleQubitGate<Time> {
	public:
		/**
		 * @brief UGate constructor.
		 *
		 * Constructs the UGate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit The qubit the quantum gate is applied to.
		 * @param theta The theta angle for the rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa SingleQubitGate
		 */
		UGate(Types::qubit_t qubit = 0, double theta = 0, double phi = 0, double lambda = 0, double gamma = 0, Time delay = 0) 
			: SingleQubitGate<Time>(qubit, delay), theta(theta), phi(phi), lambda(lambda), gamma(gamma)
		{
		}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyU(SingleQubitGate<Time>::GetQubit(), theta, phi, lambda, gamma);
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kUGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<UGate<Time>>(SingleQubitGate<Time>::GetQubit(), GetTheta(), GetPhi(), GetLambda(), GetGamma(), IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Set the theta parameter for the gate.
		 *
		 * Sets the theta parameter for the gate.
		 * @param t The theta parameter for the gate.
		 */
		void SetTheta(double t) { theta = t; }

		/**
		 * @brief Get the theta parameter for the gate.
		 *
		 * Returns the theta parameter for the gate.
		 * @return The theta parameter for the gate.
		 */
		double GetTheta() const { return theta; }

		/**
		 * @brief Set the phi parameter for the gate.
		 *
		 * Sets the phi parameter for the gate.
		 * @param p The phi parameter for the gate.
		 */
		void SetPhi(double p) { phi = p; }

		/**
		 * @brief Get the phi parameter for the gate.
		 *
		 * Returns the phi parameter for the gate.
		 * @return The phi parameter for the gate.
		 */
		double GetPhi() const { return phi; }

		/**
		 * @brief Set the lambda parameter for the gate.
		 *
		 * Sets the lambda parameter for the gate.
		 * @param l The lambda parameter for the gate.
		 */
		void SetLambda(double l) { lambda = l; }

		/**
		 * @brief Get the lambda parameter for the gate.
		 *
		 * Returns the lambda parameter for the gate.
		 * @return The lambda parameter for the gate.
		 */
		double GetLambda() const { return lambda; }

		/**
		 * @brief Set the gamma parameter for the gate.
		 *
		 * Sets the gamma parameter for the gate.
		 * @param g The gamma parameter for the gate.
		 */
		void SetGamma(double g) { gamma = g; }

		/**
		 * @brief Get the gamma parameter for the gate.
		 *
		 * Returns the gamma parameter for the gate.
		 * @return The gamma parameter for the gate.
		 */
		double GetGamma() const { return gamma; }

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { theta, phi, lambda, gamma }; }

		/**
		 * @brief Checks if the operation is a Clifford one.
		 *
		 * Checks if the operation is a Clifford one, allowing to be simulated in a stabilizers simulator.
		 *
		 * @return True if it can be applied in a stabilizers simulator, false otherwise.
		 */
		bool IsClifford() const override
		{
			return false;
		}

	private:
		double theta; /**< The theta parameter for the gate. */
		double phi;   /**< The phi parameter for the gate. */
		double lambda; /**< The lambda parameter for the gate. */
		double gamma; /**< The gamma parameter for the gate. */
	};

	//**********************************************************************************************
	// Two Qubit Gates
	//**********************************************************************************************

	// this one should be replaced - for example with three qnots - to be able to distribute it
	
	/**
	 * @class SwapGate
	 * @brief The swap gate.
	 *
	 * The swap gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class SwapGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief SwapGate constructor.
		 *
		 * Constructs the swap gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param qubit1 The first qubit the quantum gate is applied to.
		 * @param qubit2 The second qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		SwapGate(Types::qubit_t qubit1 = 0, Types::qubit_t qubit2 = 1, Time delay = 0) : TwoQubitsGate<Time>(qubit1, qubit2, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplySwap(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kSwapGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<SwapGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
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

	// the others are all controlled gates

	/**
	 * @class CXGate
	 * @brief The controlled x gate.
	 *
	 * The controlled not gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CXGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CXGate constructor.
		 *
		 * Constructs the controlled x object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CXGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCX(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCXGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CXGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
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
	 * @class CYGate
	 * @brief The controlled y gate.
	 *
	 * The controlled y gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CYGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CYGate constructor.
		 *
		 * Constructs the controlled y gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CYGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCY(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCYGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CYGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
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
	 * @class CZGate
	 * @brief The controlled z gate.
	 *
	 * The controlled z gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CZGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CZGate constructor.
		 *
		 * Constructs the controlled z gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CZGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCZ(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCZGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CZGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
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
	 * @class CPGate
	 * @brief The controlled P gate.
	 *
	 * The controlled phase gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CPGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CPGate constructor.
		 *
		 * Constructs the controlled phase gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param lambda The lambda parameter for the controlled phase gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CPGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double lambda = 0, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay), lambda(lambda) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCP(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), GetLambda());
		}

		/**
		 * @brief Set the lambda parameter for the controlled phase gate.
		 *
		 * Sets the lambda parameter for the controlled phase gate.
		 * @param l The lambda parameter for the controlled phase gate.
		 */
		void SetLambda(double l) { lambda = l; }

		/**
		 * @brief Get the lambda parameter for the controlled phase gate.
		 *
		 * Returns the lambda parameter for the controlled phase gate.
		 * @return The lambda parameter for the controlled phase gate.
		 */
		double GetLambda() const { return lambda; }

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCPGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CPGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), GetLambda(), IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { lambda }; }

	private:
		double lambda; /**< The lambda parameter for the controlled phase gate. */
	};


	/**
	 * @class CRotationGate
	 * @brief The controlled rotation gate.
	 *
	 * The controlled rotation gate. Must be overriden for specific rotations.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class ControlledRotationGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief ControlledRotationGate constructor.
		 *
		 * Constructs the controlled rotation gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * This class is not to be used directly, there are derived classes for specific rotations.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param theta The theta angle for the controlled rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		ControlledRotationGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double theta = 0, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay), theta(theta) {}

		/**
		 * @brief Set the theta angle for the controlled rotation gate.
		 *
		 * Sets the theta angle for the controlled rotation gate.
		 * @param t The theta angle for the controlled rotation gate.
		 */
		void SetTheta(double t) { theta = t; }

		/**
		 * @brief Get the theta angle for the controlled rotation gate.
		 *
		 * Returns the theta angle for the controlled rotation gate.
		 * @return The theta angle for the controlled rotation gate.
		 */
		double GetTheta() const { return theta; }

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { theta }; }

	private:
		double theta; /**< The theta angle for the controlled rotation gate. */
	};

	/**
	 * @class CRxGate
	 * @brief The controlled x rotation gate.
	 *
	 * The controlled x rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CRxGate : public ControlledRotationGate<Time> {
	public:
		/**
		 * @brief CRxGate constructor.
		 *
		 * Constructs the controlled x rotation gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param theta The theta angle for the controlled rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CRxGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double theta = 0, Time delay = 0) : ControlledRotationGate<Time>(ctrl, target, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCRx(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCRxGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CRxGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CRyGate
	 * @brief The controlled y rotation gate.
	 *
	 * The controlled y rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CRyGate : public ControlledRotationGate<Time> {
	public:
		/**
		 * @brief CRyGate constructor.
		 *
		 * Constructs the controlled y rotation gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param theta The theta angle for the controlled rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CRyGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double theta = 0, Time delay = 0) : ControlledRotationGate<Time>(ctrl, target, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCRy(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCRyGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CRyGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CRzGate
	 * @brief The controlled z rotation gate.
	 *
	 * The controlled z rotation gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CRzGate : public ControlledRotationGate<Time> {
	public:
		/**
		 * @brief CRzGate constructor.
		 *
		 * Constructs the controlled z rotation gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param theta The theta angle for the controlled rotation gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CRzGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double theta = 0, Time delay = 0) : ControlledRotationGate<Time>(ctrl, target, theta, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCRz(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCRzGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CRzGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), ControlledRotationGate<Time>::GetTheta(), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CHGate
	 * @brief The controlled H gate.
	 *
	 * The controlled Hadamard gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CHGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CHGate constructor.
		 *
		 * Constructs the controlled Haramard gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CHGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCH(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCHGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CHGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CSxGate
	 * @brief The controlled Sx gate.
	 *
	 * The controlled Sx gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CSxGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CSxGate constructor.
		 *
		 * Constructs the controlled Sx gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CSxGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCSx(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCSxGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CSxGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CSxDagGate
	 * @brief The controlled Sx dagger gate.
	 *
	 * The controlled Sx dagger gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CSxDagGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CSxDagGate constructor.
		 *
		 * Constructs the controlled Sx dagger gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CSxDagGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCSxDAG(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCSxDagGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CSxDagGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CUGate
	 * @brief The controlled U gate.
	 *
	 * The controlled U gate.
	 * @tparam Time The type of the execution delay.
	 * @sa TwoQubitsGate
	 */
	template<typename Time = Types::time_type> class CUGate : public TwoQubitsGate<Time> {
	public:
		/**
		 * @brief CUGate constructor.
		 *
		 * Constructs the controlled U gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param theta The theta parameter for the controlled U gate.
		 * @param phi The phi parameter for the controlled U gate.
		 * @param lambda The lambda parameter for the controlled U gate.
		 * @param gamma The gamma parameter for the controlled U gate.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa TwoQubitsGate
		 */
		CUGate(Types::qubit_t ctrl = 0, Types::qubit_t target = 1, double theta = 0, double phi = 0, double lambda = 0, double gamma = 0, Time delay = 0) : TwoQubitsGate<Time>(ctrl, target, delay), theta(theta), phi(phi), lambda(lambda), gamma(gamma) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCU(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), GetTheta(), GetPhi(), GetLambda(), GetGamma());
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCUGateType;
		}

		/**
		 * @brief Set the theta parameter for the controlled U gate.
		 * 
		 * Sets the theta parameter for the controlled U gate.
		 * @param t The theta parameter for the controlled U gate.
		 */
		void SetTheta(double t) { theta = t; }
		
		/**
		 * @brief Get the theta parameter for the controlled U gate.
		 *
		 * Returns the theta parameter for the controlled U gate.
		 * @return The theta parameter for the controlled U gate.
		 */
		double GetTheta() const { return theta; }

		/**
		 * @brief Set the phi parameter for the controlled U gate.
		 *
		 * Sets the phi parameter for the controlled U gate.
		 * @param p The phi parameter for the controlled U gate.
		 */
		void SetPhi(double p) { phi = p; }
		
		/**
		 * @brief Get the phi parameter for the controlled U gate.
		 *
		 * Returns the phi parameter for the controlled U gate.
		 * @return The phi parameter for the controlled U gate.
		 */
		double GetPhi() const { return phi; }

		/**
		 * @brief Set the lambda parameter for the controlled U gate.
		 *
		 * Sets the lambda parameter for the controlled U gate.
		 * @param l The lambda parameter for the controlled U gate.
		 */
		void SetLambda(double l) { lambda = l; }
		
		/**
		 * @brief Get the lambda parameter for the controlled U gate.
		 *
		 * Returns the lambda parameter for the controlled U gate.
		 * @return The lambda parameter for the controlled U gate.
		 */
		double GetLambda() const { return lambda; }

		/**
		 * @brief Set the gamma parameter for the controlled U gate.
		 *
		 * Sets the gamma parameter for the controlled U gate.
		 * @param g The gamma parameter for the controlled U gate.
		 */
		void SetGamma(double g) { gamma = g; }
		
		/**
		 * @brief Get the gamma parameter for the controlled U gate.
		 *
		 * Returns the gamma parameter for the controlled U gate.
		 * @return The gamma parameter for the controlled U gate.
		 */
		double GetGamma() const { return gamma; }

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CUGate<Time>>(TwoQubitsGate<Time>::GetQubit(0), TwoQubitsGate<Time>::GetQubit(1), GetTheta(), GetPhi(), GetLambda(), GetGamma(), IOperation<Time>::GetDelay());
		}

		/**
		 * @brief Get the gate parameters.
		 *
		 * Returns the parameters of the gate.
		 * @return A vector with the parameters of the gate.
		 */
		std::vector<double> GetParams() const override { return { theta, phi, lambda, gamma }; }

	private:
		double theta;  /**< The theta parameter for the controlled U gate. */
		double phi;    /**< The phi parameter for the controlled U gate. */
		double lambda; /**< The lambda parameter for the controlled U gate. */
		double gamma;  /**< The gamma parameter for the controlled U gate. */
	};

	//**********************************************************************************************
	// Three Qubit Gates
	// those should be replaced by sets of gates with less qubits, to be able to distribute them
	//**********************************************************************************************

	/**
	 * @class CCXGate
	 * @brief The controlled controlled x gate.
	 *
	 * The controlled controlled not gate.
	 * @tparam Time The type of the execution delay.
	 * @sa ThreeQubitsGate
	 */
	template<typename Time = Types::time_type> class CCXGate : public ThreeQubitsGate<Time> {
	public:
		/**
		 * @brief CCXGate constructor.
		 *
		 * Constructs the controlled controlled not gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl1 The first control qubit the quantum gate is applied to.
		 * @param ctrl2 The second control qubit the quantum gate is applied to.
		 * @param target The target qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa ThreeQubitsGate
		 */
		CCXGate(Types::qubit_t ctrl1 = 0, Types::qubit_t ctrl2 = 1, Types::qubit_t target = 2, Time delay = 0) : ThreeQubitsGate<Time>(ctrl1, ctrl2, target, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCCX(ThreeQubitsGate<Time>::GetQubit(0), ThreeQubitsGate<Time>::GetQubit(1), ThreeQubitsGate<Time>::GetQubit(2));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCCXGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CCXGate<Time>>(ThreeQubitsGate<Time>::GetQubit(0), ThreeQubitsGate<Time>::GetQubit(1), ThreeQubitsGate<Time>::GetQubit(2), IOperation<Time>::GetDelay());
		}
	};

	/**
	 * @class CSwapGate
	 * @brief The controlled swap gate.
	 *
	 * The controlled swap gate.
	 * @tparam Time The type of the execution delay.
	 * @sa ThreeQubitsGate
	 */
	template<typename Time = Types::time_type> class CSwapGate : public ThreeQubitsGate<Time> {
	public:
		/**
		 * @brief CSwapGate constructor.
		 *
		 * Constructs the controlled swap gate object. If specified, the delay is the time the quantum gate takes to execute.
		 * @param ctrl The control qubit the quantum gate is applied to.
		 * @param target1 The first swap qubit the quantum gate is applied to.
		 * @param target2 The second swap qubit the quantum gate is applied to.
		 * @param delay The time the quantum gate takes to execute.
		 * @sa ThreeQubitsGate
		 */
		CSwapGate(Types::qubit_t ctrl = 0, Types::qubit_t target1 = 1, Types::qubit_t target2 = 2, Time delay = 0) : ThreeQubitsGate<Time>(ctrl, target1, target2, delay) {}

		/**
		 * @brief Execute the quantum gate.
		 *
		 * Executes the quantum gate on the given simulator with the given classical state.
		 * @param sim The simulator to execute the quantum gate on.
		 * @param state The classical state to execute the quantum gate with.
		 * @sa ISimulator
		 * @sa ClassicalState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			sim->ApplyCSwap(ThreeQubitsGate<Time>::GetQubit(0), ThreeQubitsGate<Time>::GetQubit(1), ThreeQubitsGate<Time>::GetQubit(2));
		}

		/**
		 * @brief Get the type of the quantum gate.
		 *
		 * Returns the type of the quantum gate.
		 * @return The type of the quantum gate.
		 * @sa QuantumGateType
		 */
		QuantumGateType GetGateType() const override
		{
			return QuantumGateType::kCSwapGateType;
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IOperation<Time>> Clone() const override
		{
			return std::make_shared<CSwapGate<Time>>(ThreeQubitsGate<Time>::GetQubit(0), ThreeQubitsGate<Time>::GetQubit(1), ThreeQubitsGate<Time>::GetQubit(2), IOperation<Time>::GetDelay());
		}
	};
}

#endif // !_QUANTUM_GATES_H_
