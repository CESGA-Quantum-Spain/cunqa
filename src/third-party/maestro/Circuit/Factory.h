/**
 * @file Factory.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Factory for quantum gates and other operations.
 * 
 * Contains factory methods for quantum gates, resets, measurements, conditional gates, etc.
 * There are even for factory methods for some circuits that are important (for example for distribution).
 */


#pragma once

#ifndef _CIRCUIT_FACTORY_H_
#define _CIRCUIT_FACTORY_H_

#include "Circuit.h"
#include "Reset.h"
#include "RandomOp.h"
#include "Measurements.h"
#include "Conditional.h"

namespace Circuits {

	/**
	 * @class CircuitFactory
	 * @brief Factory for quantum gates and other operations.
	 * 
	 * Contains factory methods for quantum gates, resets, measurements, conditional gates, etc.
	 * There are even for factory methods for some circuits that are important (for example for distribution).
	 * @tparam Time The time type used for operations timing.
	 */
	template<typename Time = Types::time_type> class CircuitFactory
	{
	public:
		/**
		 * @brief Construct a circuit.
		 * 
		 * Constructs a circuit with the given operations.
		 * @param ops The operations to add to the circuit.
		 * @return The created circuit.
		 * @sa Circuit
		 */
		static std::shared_ptr<Circuit<Time>> CreateCircuit(const std::vector<std::shared_ptr<IOperation<Time>>>& ops = {})
		{
			return std::make_shared<Circuit<Time>>(ops);
		}

		/**
		 * @brief Construct a reset operation.
		 * 
		 * Resets the qubits to the given state. If not specified, the qubits are reset to |0>.
		 * 
		 * @param qubits The qubits to reset.
		 * @param resetTgts The reset values, true to reset to |1>, false to reset to |0>.
		 * @return The reset operation.
		 * @sa Reset
		 */
		static std::shared_ptr<IOperation<Time>> CreateReset(const Types::qubits_vector& qubits = {}, const std::vector<bool>& resetTgts = {})
		{
			return std::make_shared<Reset<Time>>(qubits, 0, resetTgts);
		}

		/**
		 * @brief Construct a random operation.
		 * 
		 * Generates a random 0 or 1 for each specified classical bit.
		 * 
		 * @param bits The bits to generate random numbers for.
		 * @param seed The seed to use for the random number generator.
		 * @return The random operation.
		 * @sa Random
		 */
		static std::shared_ptr<IOperation<Time>> CreateRandom(const std::vector<size_t>& bits = {}, size_t seed = 0)
		{
			return std::make_shared<Random<Time>>(bits, seed);
		}

		/**
		 * @brief Construct a measurement operation.
		 * 
		 * Measures the qubits and stores the result in the classical bits.
		 * 
		 * @param qs The qubits to measure and the corresponding classical bit where to put results, specified as pairs.
		 * @return The measurement operation.
		 * @sa MeasurementOperation
		 */
		static const std::shared_ptr<IOperation<Time>> CreateMeasurement(const std::vector<std::pair<Types::qubit_t, size_t>>& qs = {})
		{
			return std::make_shared<MeasurementOperation<Time>>(qs);
		}

		/**
		 * @brief Construct a quantum gate.
		 *
		 * Constructs a quantum gate operation.
		 * 
		 * @param type The type of gate to construct.
		 * @param q1 The first qubit.
		 * @param q2 The second qubit (if it's two or three qubit gate, otherwise ignored).
		 * @param q3 The third qubit (if it's a three qubit gate, otherwise ignored).
		 * @param param1 The first parameter (if it has parameters, otherwise ignored).
		 * @param param2 The second parameter (if it has more than one parameter, otherwise ignored).
		 * @param param3 The third parameter (if it has more than two parameters, otherwise ignored).
		 * @param param4 The fourth parameter (if it has more than three parameters, otherwise ignored).
		 * @return The quantum gate.
		 * @sa IQuantumGate
		 * @sa QuantumGateType
		 */
		static const std::shared_ptr<IQuantumGate<Time>> CreateGate(QuantumGateType type, size_t q1, size_t q2 = 0, size_t q3 = 0, double param1 = 0, double param2 = 0, double param3 = 0, double param4 = 0)
		{
			// parameters that do not make sense for a gate are ignored, eg if it's a single qubit gate, q2 and q3 are ignored
			// if it doesn't have a parameter, the parameter is ignored
			// if it has only one, the others are ignored
			// TODO: maybe make specialized variants, as in CreateOneQubitGate, CreateTwoQubitGate, etc
			std::shared_ptr<IQuantumGate<Time>> gate;

			switch (type)
			{
				// one qubit gates
			case QuantumGateType::kPhaseGateType:
				gate = std::make_shared<PhaseGate<Time>>(q1, param1);
				break;
			case QuantumGateType::kXGateType:
				gate = std::make_shared<XGate<Time>>(q1);
				break;
			case QuantumGateType::kYGateType:
				gate = std::make_shared<YGate<Time>>(q1);
				break;
			case QuantumGateType::kZGateType:
				gate = std::make_shared<ZGate<Time>>(q1);
				break;
			case QuantumGateType::kHadamardGateType:
				gate = std::make_shared<HadamardGate<Time>>(q1);
				break;
			case QuantumGateType::kSGateType:
				gate = std::make_shared<SGate<Time>>(q1);
				break;
			case QuantumGateType::kSdgGateType:
				gate = std::make_shared<SdgGate<Time>>(q1);
				break;
			case QuantumGateType::kTGateType:
				gate = std::make_shared<TGate<Time>>(q1);
				break;
			case QuantumGateType::kTdgGateType:
				gate = std::make_shared<TdgGate<Time>>(q1);
				break;
			case QuantumGateType::kSxGateType:
				gate = std::make_shared<SxGate<Time>>(q1);
				break;
			case QuantumGateType::kSxDagGateType:
				gate = std::make_shared<SxDagGate<Time>>(q1);
				break;
			case QuantumGateType::kKGateType:
				gate = std::make_shared<KGate<Time>>(q1);
				break;
			case QuantumGateType::kRxGateType:
				gate = std::make_shared<RxGate<Time>>(q1, param1);
				break;
			case QuantumGateType::kRyGateType:
				gate = std::make_shared<RyGate<Time>>(q1, param1);
				break;
			case QuantumGateType::kRzGateType:
				gate = std::make_shared<RzGate<Time>>(q1, param1);
				break;
			case QuantumGateType::kUGateType:
				gate = std::make_shared<UGate<Time>>(q1, param1, param2, param3, param4);
				break;
				// two qubit gates
			case QuantumGateType::kSwapGateType:
				gate = std::make_shared<SwapGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCXGateType:
				gate = std::make_shared<CXGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCYGateType:
				gate = std::make_shared<CYGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCZGateType:
				gate = std::make_shared<CZGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCPGateType:
				gate = std::make_shared<CPGate<Time>>(q1, q2, param1);
				break;
			case QuantumGateType::kCRxGateType:
				gate = std::make_shared<CRxGate<Time>>(q1, q2, param1);
				break;
			case QuantumGateType::kCRyGateType:
				gate = std::make_shared<CRyGate<Time>>(q1, q2, param1);
				break;
			case QuantumGateType::kCRzGateType:
				gate = std::make_shared<CRzGate<Time>>(q1, q2, param1);
				break;
			case QuantumGateType::kCHGateType:
				gate = std::make_shared<CHGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCSxGateType:
				gate = std::make_shared<CSxGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCSxDagGateType:
				gate = std::make_shared<CSxDagGate<Time>>(q1, q2);
				break;
			case QuantumGateType::kCUGateType:
				gate = std::make_shared<CUGate<Time>>(q1, q2, param1, param2, param3, param4);
				break;
				// three qubit gates
			case QuantumGateType::kCSwapGateType:
				gate = std::make_shared<CSwapGate<Time>>(q1, q2, q3);
				break;
			case QuantumGateType::kCCXGateType:
				gate = std::make_shared<CCXGate<Time>>(q1, q2, q3);
				break;
			default:
				break;
			}

			return gate;
		}

		/**
		 * @brief Construct a quantum gate.
		 *
		 * Constructs a quantum gate operation.
		 * 
		 * @param type The type of gate to construct.
		 * @param qubits The qubits to apply the gate to.
		 * @param params The parameters of the gate.
		 * @return A smart pointer to the created quantum gate.
		 * @sa IQuantumGate
	     * @sa QuantumGateType
		 */
		static const std::shared_ptr<IOperation<Time>> CreateGateWithVectors(QuantumGateType type, const Types::qubits_vector& qubits, const std::vector<double>& params = {})
		{
			return CreateGate(type, qubits.empty() ? 0 : qubits[0], (qubits.size() < 2) ? 0 : qubits[1], (qubits.size() < 3) ? 0 : qubits[2], params.empty() ? 0. : params[0], (params.size() < 2) ? 0. : params[1], (params.size() < 3) ? 0. : params[2], (params.size() < 4) ? 0. : params[3]);
		}


		// maybe this could be refined, have a CreateCondition with the condition type, but for now we only have equality
		// can be done later

		/**
		 * @brief Construct an equality condition.
		 * 
		 * Constructs an equality condition for the given classical bits and the given values.
		 * 
		 * @param ind The classical bits to check.
		 * @param b The values to check for equality.
		 * @return The equality condition.
		 * @sa ICondition
		 */
		static std::shared_ptr<ICondition> CreateEqualCondition(const std::vector<size_t>& ind, const std::vector<bool>& b)
		{
			return std::make_shared<EqualCondition>(ind, b);
		}

		// we have only three conditional things, I don't think we need a single factory method for them (with a 'type' parameter)

		/**
		 * @brief Construct a conditional gate.
		 * 
		 * Constructs a conditional gate with the given operation and condition.
		 * 
		 * @param operation The operation to perform if the condition is met.
		 * @param condition The condition to check.
		 * @return The conditional gate.
		 * @sa ConditionalGate
		 * @sa IGateOperation
		 * @sa ICondition
		 */
		static std::shared_ptr<IOperation<Time>> CreateConditionalGate(const std::shared_ptr<IGateOperation<Time>>& operation, const std::shared_ptr<ICondition>& condition)
		{
			return std::make_shared<ConditionalGate<Time>>(operation, condition);
		}

		/**
         * @brief Construct a simple conditional gate.
         *
         * Constructs a simple conditional gate with the given operation and a single cbit value.
         * The gate is applied only if the cbit is true.
		 * 
         * @param operation The operation to perform if the condition is met.
         * @param cbit The cbit to be checked.
         * @return The conditional gate.
         * @sa ConditionalGate
         * @sa IGateOperation
         * @sa ICondition
         */
		static std::shared_ptr<IOperation<Time>> CreateSimpleConditionalGate(const std::shared_ptr<IGateOperation<Time>>& operation, const size_t cbit)
		{
			const auto eqcond = CreateEqualCondition({ cbit }, { true });
			return std::make_shared<ConditionalGate<Time>>(operation, eqcond);
		}

		/**
		 * @brief Constructs a conditional measurement.
		 *
		 * Constructs a conditional measurement with the given measurement and condition.
		 * 
		 * @param measurement The measurement to perform if the condition is met.
		 * @param condition The condition to check.
		 * @return The conditional measurement.
		 * @sa ConditionalMeasurement
		 * @sa MeasurementOperation
		 * @sa ICondition
		 */
		static std::shared_ptr<IOperation<Time>> CreateConditionalMeasurement(const std::shared_ptr<MeasurementOperation<Time>>& measurement, const std::shared_ptr<ICondition>& condition)
		{
			return std::make_shared<ConditionalMeasurement<Time>>(measurement, condition);
		}

		/**
		 * @brief Constructs a conditional random number generator.
		 *
		 * Constructs a conditional random number generator with the given random number generator and condition.
		 * 
		 * @param randomGen The random number generator to perform if the condition is met.
		 * @param condition The condition to check.
		 * @return The conditional random number generator.
		 * @sa ConditionalRandomGen
		 * @sa Random
		 * @sa ICondition
		 */
		static std::shared_ptr<IOperation<Time>> CreateConditionalRandomGen(const std::shared_ptr<Random<Time>>& randomGen, const std::shared_ptr<ICondition>& condition)
		{
			return std::make_shared<ConditionalRandomGen<Time>>(randomGen, condition);
		}

		/**
		 * @brief Creates a no op.
		 * 
		 * Creates a no op, an operation that does nothing.
		 * 
		 * @return The no op.
		 * @sa NoOperation
		 */
		static std::shared_ptr<IOperation<Time>> CreateNoOp()
		{
			return std::make_shared<NoOperation<Time>>();
		}

		/**
		 * @brief Creates a circuit that prepares a qubit pair in a Bell state.
		 * 
		 * Creates a circuit that prepares a qubit pair in a Bell state.
		 * 
		 * @param qbit1 The first qubit.
		 * @param qbit2 The second qubit.
		 * @param qbit1X Whether to apply an X gate to the first qubit.
		 * @param qbit2X Whether to apply an X gate to the second qubit.
		 * @return The circuit that prepares the qubit pair in a Bell state.
		 * @sa IOperation
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateBellStateCircuit(size_t qbit1, size_t qbit2, bool qbit1X = false, bool qbit2X = false)
		{
			size_t s = 2;
			if (qbit1X) ++s;
			if (qbit2X) ++s;

			std::vector<std::shared_ptr<IOperation<Time>>> ops(s);

			s = 0;
			if (qbit1X)
				ops[s++] = CreateGate(QuantumGateType::kXGateType, qbit1);
			if (qbit2X)
				ops[s++] = CreateGate(QuantumGateType::kXGateType, qbit2);
			ops[s++] = CreateGate(QuantumGateType::kHadamardGateType, qbit1);
			ops[s] = CreateGate(QuantumGateType::kCXGateType, qbit1, qbit2);

			return ops;
		}

		/**
		 * @brief Creates a Bell state decoder circuit.
		 * 
		 * Creates a Bell state decoder circuit.
		 * 
		 * @param qbit1 The first qubit.
		 * @param qbit2 The second qubit.
		 * @return The decoder circuit.
		 * @sa IOperation
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateBellStateDecoderCircuit(size_t qbit1, size_t qbit2)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(2);

			ops[0] = CreateGate(QuantumGateType::kCXGateType, qbit1, qbit2);
			ops[1] = CreateGate(QuantumGateType::kHadamardGateType, qbit1);

			return ops;
		}

		/**
		 * @brief Creates a circuit that prepares three qubits in a GHZ state.
		 * 
		 * Creates a circuit that prepares a three qubits in a GHZ state.
		 * 
		 * @param qbit1 The first qubit.
		 * @param qbit2 The second qubit.
		 * @param qbit3 The third qubit.
		 * @return The circuit that prepares the qubits in a GHZ state.
		 * @sa IOperation
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateGZHStateCircuit(size_t qbit1, size_t qbit2, size_t qbit3)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);

			ops[0] = CreateGate(QuantumGateType::kHadamardGateType, qbit1);
			ops[1] = CreateGate(QuantumGateType::kCXGateType, qbit1, qbit2);
			ops[2] = CreateGate(QuantumGateType::kCXGateType, qbit2, qbit3);

			return ops;
		}

		/**
		 * @brief Creates a teleportation circuit.
		 * 
		 * Creates a teleportation.
		 * The source qubit is teleported to the second entanglement qubit.
		 * 
		 * @param entqbit1 The first entanglment qubit.
		 * @param entqbit2 The second entanglement qubit.
		 * @param srcqbit The qubit to be teleported.
		 * @param cbit1 The classical bit to store the measurement of the qubit to be teleported.
		 * @param cbit2 The classical bit to store the measurement of the first entanglement qubit.
		 * @return The teleportation circuit.
		 * @sa IOperation
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateTeleportationCircuit(size_t entqbit1, size_t entqbit2, size_t srcqbit, size_t cbit1, size_t cbit2)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(8);

			// EPR pair
			ops[0] = CreateGate(QuantumGateType::kHadamardGateType, entqbit1);
			ops[1] = CreateGate(QuantumGateType::kCXGateType, entqbit1, entqbit2);

			// teleportation follows, using the entangled qubits
			ops[2] = CreateGate(QuantumGateType::kCXGateType, srcqbit, entqbit1);
			ops[3] = CreateGate(QuantumGateType::kHadamardGateType, srcqbit);

			// measurements
			ops[4] = CreateMeasurement({ {srcqbit, cbit1} }); 
			ops[5] = CreateMeasurement({ {entqbit1, cbit2} });

			// apply the conditional gates based on the measurements
			const auto xgate = CreateGate(QuantumGateType::kXGateType, entqbit2);
			const auto zgate = CreateGate(QuantumGateType::kZGateType, entqbit2);

			ops[6] = CreateSimpleConditionalGate(xgate, cbit2);
			ops[7] = CreateSimpleConditionalGate(zgate, cbit1);

			return ops;
		}

		/**
		 * @brief Creates the circuit for distribution start
		 * 
		 * Creates the circuit operations for the start of the distribution.
		 * 
		 * @param ctrlQubit The control qubit for the operation to distribute.
		 * @param ctrlEntangledQubit The entangled qubit on the control host.
		 * @param tgtEntangledQubit The entangled qubit on the target host.
		 * @param ctrlEntangledMeasureBit The classical bit to store the measurement of the entangled qubit on the control host.
		 * @return The distribution start circuit operations.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateStartDistributionCircuit(size_t ctrlQubit, size_t ctrlEntangledQubit, size_t tgtEntangledQubit, size_t ctrlEntangledMeasureBit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(5);
			// 1. Entangle qubits for the two hosts
			//auto ops = CreateBellStateCircuit(ctrlEntangledQubit, tgtEntangledQubit);
			ops[0] = CreateGate(QuantumGateType::kHadamardGateType, ctrlEntangledQubit);
			ops[1] = CreateGate(QuantumGateType::kCXGateType, ctrlEntangledQubit, tgtEntangledQubit);
			
			// 2. Now apply cnot from ctrl to entangled qubit on ctrl host
			//ops.emplace_back(std::make_shared<Circuits::CXGate<Time>>(ctrlQubit, ctrlEntangledQubit));
			ops[2] = CreateGate(QuantumGateType::kCXGateType, ctrlQubit, ctrlEntangledQubit);
			// 3. Measure the entangled qubit on ctrl host (then the measurement result is sent to the other host)
			const std::vector<std::pair<Types::qubit_t, size_t>> measureOps{ { ctrlEntangledQubit, ctrlEntangledMeasureBit } };
			//ops.emplace_back(std::make_shared<Circuits::MeasurementOperation<Time>>(measureOps));
			ops[3] = CreateMeasurement(measureOps);

			// 4. Send the measurement result to the target host, use the result to apply not on the target entangled qubit
			//ops.emplace_back(std::make_shared<Circuits::ConditionalGate<Time>>(std::make_shared<Circuits::XGate<Time>>(tgtEntangledQubit),
			//	std::make_shared<Circuits::EqualCondition>(std::vector<size_t>{ ctrlEntangledMeasureBit }, std::vector<bool>{true})));
			ops[4] = std::make_shared<Circuits::ConditionalGate<Time>>(std::make_shared<Circuits::XGate<Time>>(tgtEntangledQubit),
				std::make_shared<Circuits::EqualCondition>(std::vector<size_t>{ ctrlEntangledMeasureBit }, std::vector<bool>{true}));

			return ops;
		}

		/**
		 * @brief Creates the circuit for distribution end
		 * 
		 * Creates the circuit operations for the end of the distribution.
		 * 
		 * @param ctrlQubit The control qubit for the operation to distribute.
		 * @param tgtEntangledQubit The entangled qubit on the target host.
		 * @param tgtEntangledMeasureBit The classical bit to store the measurement of the entangled qubit on the target host.
		 * @return The distribution end circuit operations.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateEndDistributionCircuit(size_t ctrlQubit, size_t tgtEntangledQubit, size_t tgtEntangledMeasureBit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);

			ops[0] = std::make_shared<Circuits::HadamardGate<Time>>(tgtEntangledQubit);
			const std::vector<std::pair<Types::qubit_t, size_t>> measureOps{ { tgtEntangledQubit, tgtEntangledMeasureBit } };
			ops[1] = std::make_shared<Circuits::MeasurementOperation<Time>>(measureOps);
			ops[2] = std::make_shared<Circuits::ConditionalGate<Time>>(std::make_shared<Circuits::ZGate<Time>>(ctrlQubit),
				std::make_shared<Circuits::EqualCondition>(std::vector<size_t>{ tgtEntangledMeasureBit }, std::vector<bool>{true}));

			return ops;
		}


		/**
		 * @brief Creates the circuit for measuring a qubit in the X basis.
		 *
		 * Creates the circuit operations for measuring a qubit in the X basis.
		 * 
		 * @param qubit The qubit to measure.
		 * @param cbit The classical bit to store the measurement result.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateMeasureX(size_t qubit, size_t cbit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(2);
			ops[0] = CreateGate(QuantumGateType::kHadamardGateType, qubit);
			ops[1] = CreateMeasurement({ {qubit, cbit} });

			return ops;
		}

		/**
		 * @brief Creates the circuit for measuring a qubit in the Y basis.
		 *
		 * Creates the circuit operations for measuring a qubit in the Y basis.
		 *
		 * @param qubit The qubit to measure.
		 * @param cbit The classical bit to store the measurement result.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateMeasureY(size_t qubit, size_t cbit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);
			ops[0] = CreateGate(QuantumGateType::kSdgGateType, qubit);
			ops[1] = CreateGate(QuantumGateType::kHadamardGateType, qubit);
			ops[2] = CreateMeasurement({ {qubit, cbit} });

			return ops;
		}

		/**
	     * @brief Creates the circuit for measuring a qubit in the Z basis.
		 *
		 * Creates the circuit operations for measuring a qubit in the Z basis.
		 *
		 * @param qubit The qubit to measure.
		 * @param cbit The classical bit to store the measurement result.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateMeasureZ(size_t qubit, size_t cbit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(1);
			ops[0] = CreateMeasurement({ {qubit, cbit} });

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |0> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |0> state.
		 * 
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitZero(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(1);
			ops[0] = CreateReset({ qubit });

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |1> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |1> state.
		 *
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitOne(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(2);
			ops[0] = CreateReset({ qubit });
			ops[1] = CreateGate(QuantumGateType::kXGateType, qubit);

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |+> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |+> state.
		 *
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitPlus(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(2);
			ops[0] = CreateReset({ qubit });
			ops[1] = CreateGate(QuantumGateType::kHadamardGateType, qubit);

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |-> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |-> state.
		 *
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitMinus(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);
			ops[0] = CreateReset({ qubit });
			ops[1] = CreateGate(QuantumGateType::kXGateType, qubit);
			ops[2] = CreateGate(QuantumGateType::kHadamardGateType, qubit);

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |i> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |i> state.
		 *
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitPlusI(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);
			ops[0] = CreateReset({ qubit });
			ops[1] = CreateGate(QuantumGateType::kHadamardGateType, qubit);
			ops[2] = CreateGate(QuantumGateType::kSGateType, qubit);

			return ops;
		}

		/**
		 * @brief Creates the circuit for initializing a qubit in the |-i> state.
		 *
		 * Creates the circuit operations for initializing a qubit in the |-i> state.
		 *
		 * @param qubit The qubit to initialize.
		 */
		static std::vector<std::shared_ptr<IOperation<Time>>> CreateInitMinusI(size_t qubit)
		{
			std::vector<std::shared_ptr<IOperation<Time>>> ops(3);
			ops[0] = CreateReset({ qubit });
			ops[1] = CreateGate(QuantumGateType::kHadamardGateType, qubit);
			ops[2] = CreateGate(QuantumGateType::kSdgGateType, qubit);
	
			return ops;
		}
	};
}

#endif // !_CIRCUIT_FACTORY_H_
