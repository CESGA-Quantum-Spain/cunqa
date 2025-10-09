/**
 * @file Circuit.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The circuit class.
 *
 * Contains a list of operations that can be executed on a simulator.
 * Allows setting, getting, adding, replacing and cloning.
 * Also has a function to convert the circuit for distributed computing, replacing the three qubit gates
 * with several gates on less qubits and the swap gates with three cnots.
 */

#pragma once

#ifndef _CIRCUIT_H_
#define _CIRCUIT_H_

#define _USE_MATH_DEFINES
#include <math.h>
#include <set>

#include <vector>
#include "Operations.h"
#include "Conditional.h"
#include "Reset.h"
#include "QuantumGates.h"

namespace Circuits {

	/**
	 * @class Circuit
	 * @brief Circuit class for holding the sequence of operations.
	 *
	 * Contains a sequence of operations that can be executed, supplying a function that allows executing them in a simulator.
	 * Allows adding operations and converting them to prepare the circuit for distributed computing.
	 * @tparam Time The time type used for operation timing.
	 * @sa IOperation
	 * @sa ISimulator
	 */
	template<typename Time = Types::time_type> class Circuit : public IOperation<Time> {
	public:
		using ExecuteResults = std::unordered_map<std::vector<bool>, size_t>; /**< The results of the execution of the circuit. */
		using BitMapping = std::unordered_map<Types::qubit_t, Types::qubit_t>; /**< The (qu)bit mapping for remapping. */

		using Operation = IOperation<Time>; /**< The operation type. */
		using OperationPtr = std::shared_ptr<Operation>; /**< The shared pointer to the operation type. */
		using OperationsVector = std::vector<OperationPtr>; /**< The vector of operations. */

		using value_type = typename OperationsVector::value_type;
		using allocator_type = typename OperationsVector::allocator_type;
		using pointer = typename OperationsVector::pointer;
		using const_pointer = typename OperationsVector::const_pointer;
		using reference = typename OperationsVector::reference;
		using const_reference = typename OperationsVector::const_reference;
		using size_type = typename OperationsVector::size_type;
		using difference_type = typename OperationsVector::difference_type;

		using iterator = typename OperationsVector::iterator;
		using const_iterator = typename OperationsVector::const_iterator;
		using reverse_iterator = typename OperationsVector::reverse_iterator;
		using const_reverse_iterator = typename OperationsVector::const_reverse_iterator;

		/**
		 * @brief Construct a new Circuit object.
		 *
		 * Constructs a new Circuit object with the given operations.
		 * @param ops The operations to add to the circuit.
		 * @sa IOperation
		 */
		Circuit(const OperationsVector& ops = {})
			: Operation(), operations(ops)
		{
		}

		/**
		 * @brief Execute the circuit on the given simulator.
		 *
		 * Executes the circuit on the given simulator.
		 * @param sim The simulator to execute the circuit on.
		 * @param state The classical state containing the classical bits.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void Execute(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const override
		{
			state.Reset();
			if (!sim) return;

			for (const auto& op : operations)
				op->Execute(sim, state);
			//sim->Flush();
		}

		/**
		 * @brief Get the type of the circuit.
		 *
		 * Returns the type of the circuit.
		 * @return The type of the circuit.
		 * @sa OperationType
		 */
		OperationType GetType() const override
		{
			return OperationType::kComposite;
		}

		/**
		 * @brief Adds an operation to the circuit.
		 *
		 * Adds an operation to the circuit.
		 * @param op The operation to add.
		 * @sa IOperation
		 */
		void AddOperation(const OperationPtr& op)
		{
			operations.push_back(op);
		}

		/**
		 * @brief Replaces an operation in the circuit.
		 *
		 * Replaces an operation in the circuit.
		 * @param index The index of the operation to replace.
		 * @param op The operation to replace with.
		 * @sa IOperation
		 */
		void ReplaceOperation(size_t index, const OperationPtr& op)
		{
			if (index >= operations.size())
				return;
			operations[index] = op;
		}

		/**
		 * @brief Set the operations in the circuit.
		 *
		 * Sets the operations in the circuit.
		 * @param ops The operations to set.
		 * @sa IOperation
		 */
		void SetOperations(const OperationsVector& ops)
		{
			operations = ops;
		}

		/**
		 * @brief Adds operations to the circuit.
		 *
		 * Adds operations to the circuit.
		 * @param ops The operations to add.
		 * @sa IOperation
		 */
		void AddOperations(const OperationsVector& ops)
		{
			operations.insert(operations.end(), ops.begin(), ops.end());
		}

		/**
		 * @brief Adds operations from another circuit to the circuit.
		 *
		 * Adds operations from another circuit to the circuit.
		 * @param circuit The circuit to add operations from.
		 */
		void AddCircuit(const std::shared_ptr<Circuit<Time>>& circuit)
		{
			AddOperations(circuit->GetOperations());
		}

		/**
		 * @brief Get the operations in the circuit.
		 *
		 * Returns the operations in the circuit.
		 * @return The operations in the circuit.
		 * @sa IOperation
		 */
		const OperationsVector& GetOperations() const
		{
			return operations;
		}

		/**
		 * @brief Clears the operations from the circuit.
		 *
		 * Removes all operations from the circuit.
		 */
		void Clear()
		{
			operations.clear();
		}

		/**
		 * @brief Get a shared pointer to a clone of this object.
		 *
		 * Returns a shared pointer to a copy of this object.
		 * @return A shared pointer to this object.
		 */
		OperationPtr Clone() const override
		{
			OperationsVector newops;

			for (auto& op : operations)
				newops.emplace_back(op->Clone());

			return std::make_shared<Circuit<Time>>(newops);
		}

		/**
		 * @brief Get a shared pointer to a clone of this object, but without cloning the operations.
		 *
		 * Returns a shared pointer to a copy of this object, flyweight stype.
		 * @return A shared pointer to this object.
		 */
		OperationPtr CloneFlyweight() const
		{
			OperationsVector newops;

			for (auto& op : operations)
				newops.push_back(op);

			return std::make_shared<Circuit<Time>>(newops);
		}

		/**
		 * @brief Get a shared pointer to a circuit remapped.
		 *
		 * Returns a shared pointer to a copy of the circuit with qubits and classical bits changed according to the provided maps.
		 * 
		 * @param qubitsMap The map of qubits to remap.
		 * @param bitsMap The map of classical bits to remap.
		 * @return A shared pointer to the remapped circuit.
		 */
		OperationPtr Remap(const BitMapping& qubitsMap, const BitMapping& bitsMap = {}) const override
		{
			OperationsVector newops;

			for (const auto& op : operations)
				newops.emplace_back(op->Remap(qubitsMap, bitsMap));

			return std::make_shared<Circuit<Time>>(newops);
		}

		/**
		 * @brief Get a shared pointer to a circuit remapped to a continuous interval starting from zero.
		 *
		 * Returns a shared pointer to a copy of the circuit with qubits and classical bits changed according to the provided maps.
		 *
         * @param bitsMap The map of classical bits, to allow remapping the results back to the original circuit results.
         * @return A shared pointer to the remapped circuit.
         */
		std::shared_ptr<Circuit<Time>> RemapToContinuous(BitMapping& newQubitsMap, BitMapping& reverseBitsMap, size_t& nrQubits, size_t& nrCbits) const
		{
			OperationsVector newops;

			BitMapping newBitsMap;

			nrQubits = 0;
			nrCbits = 0;

			for (const auto& op : operations)
			{
				const auto affectedBits = op->AffectedBits();
				const auto affectedQubits = op->AffectedQubits();

				for (const auto qubit : affectedQubits)
				{
					const auto it = newQubitsMap.find(qubit);
					if (it == newQubitsMap.end())
					{
						newQubitsMap[qubit] = nrQubits;
						++nrQubits;
					}
				}

				for (const auto bit : affectedBits)
				{
					const auto it = newBitsMap.find(bit);
					if (it == newBitsMap.end())
					{
						newBitsMap[bit] = nrCbits;
						reverseBitsMap[nrCbits] = bit;
						++nrCbits;
					}
				}

				newops.emplace_back(op->Remap(newQubitsMap, newBitsMap));
			}
			
			return std::make_shared<Circuit<Time>>(newops);
		}

		/**
		 * @brief Map back the results for a remapped circuit.
		 * 
		 * Maps back the results for a remapped circuit, using the provided map.
		 * The results are the results of the execution of the circuit without remapping.
		 * @param results The results to map back.
		 * @param bitsMap The map of classical bits to remap.
		 * @param ignoreNotMapped If true, the results that are not in the map will be ignored.
		 * @param sz The size of the results vector.
		 * @return The mapped back results.
		 * @sa Circuit::Remap
		 */
		static ExecuteResults RemapResultsBack(const ExecuteResults& results, const BitMapping& bitsMap = {}, bool ignoreNotMapped = false, size_t sz = 0)
		{
			ExecuteResults newResults;

			if (!ignoreNotMapped && sz == 0)
			{
				for (const auto& [from, to] : bitsMap)
					if (to > sz) sz = to;

				++sz;
			}
			
			for (const auto& res : results)
			{
				Circuits::OperationState mappedState(res.first);
				
				mappedState.Remap(bitsMap, ignoreNotMapped, ignoreNotMapped ? bitsMap.size() : sz);
				newResults[mappedState.GetAllBits()] += res.second;
			}

			return newResults;
		}

		/**
		 * @brief Accumulate the results of a circuit execution to already existing results.
		 * 
		 * Accumulates the results of a circuit execution to already existing results.
		 * @param results The existing results to accumulate to.
		 * @param newResults The new results to add to the existing results.
		 */
		static void AccumulateResults(ExecuteResults& results, const ExecuteResults& newResults)
		{
			for (const auto& res : newResults)
				results[res.first] += res.second;
		}

		/**
		 * @brief Accumulate the results of a circuit execution to already existing results with remapping.
		 * 
		 * Accumulates the results of a circuit execution to already existing results with remapping back.
		 *
		 * @param results The existing results to accumulate to.
		 * @param newResults The new results to add to the existing results.
		 * @param bitsMap The map of classical bits to remap.
		 * @param ignoreNotMapped If true, the results that are not in the map will be ignored.
		 * @param sz The size of the results vector.
		 */
		static void AccumulateResultsWithRemapBack(ExecuteResults& results, const ExecuteResults& newResults, const BitMapping& bitsMap = {}, bool ignoreNotMapped = true, size_t sz = 0)
		{
			if (!ignoreNotMapped && sz == 0)
			{
				for (const auto& [from, to] : bitsMap)
					if (to > sz) sz = to;

				++sz;
			}

			for (const auto& res : newResults)
			{
				Circuits::OperationState mappedState(res.first);
				
				mappedState.Remap(bitsMap, ignoreNotMapped, ignoreNotMapped ? bitsMap.size() : sz);
				results[mappedState.GetAllBits()] += res.second;
				/*
				const auto it = bitsMap.find(res.first);
				if (it != bitsMap.end())
					results[it->second] += res.second;
				*/
			}
		}

		// TODO: This converts all swap, cswap and ccnot gates
		// it's not really needed to convert them all,
		// only those that are not applied locally, that is, on a single host
		// the local ones can remain as they are
		// so use network topology to decide which ones to convert
		// that's for later, when we have the network part implemented
		// and also the code that splits the circuit

		/**
		 * @brief Converts the circuit for distributed computing.
		 *
		 * Converts the circuit for distributed computing using quantum entanglement between hosts.
		 * Converts the swap gates and three qubit gates to prepare the circuit for distributed computing.
		 */
		void ConvertForDistribution()
		{
			// this will make the circuit better for distributed computing
			// TODO: if composite operations will be implemented, those need to be optimized as well

			ReplaceThreeQubitAndSwapGates();
		}

		/**
		 * @brief Converts the circuit for distributed computing.
		 *
		 * Converts the circuit for distributed computing using circuit cutting.
		 * Converts the three qubit gates to prepare the circuit for distributed computing.
		 */
		void ConvertForCutting()
		{
			ReplaceThreeQubitAndSwapGates(true);
		}

		/*
		 * @brief Splits the measurements to measurements on individual qubits and tries to order them as needed by the following clasically conditional gates.
		 *
		 * Splits the measurements to measurements on individual qubits and tries to order them as needed by the following clasically conditional gates.
		 * This is needed for netqasm, which requires the measurements to be in the right order, if the following clasically conditional gates need sending to another host.
		 * This wouldn't be needed if they are local, but we don't know that at this point.
		 * 
		 * So in short, all measurements on more than one qubit are converted to one qubit measurements,
		 * and then all measurements that are grouped together (not separated by some other operation) are ordered in the same order as the conditions on the following clasically conditional gates.
		 */
		void EnsureProperOrderForMeasurements()
		{
			// TODO: Maybe this should be moved at the netqasm level circuit conversion, since it's not needed for all kinds of distributed computing
			// currently there is a virtual function in controller that does nothing in the base class, but for netqasm it calls this method
			OperationsVector newops;

			for (size_t i = 0; i < operations.size(); ++i)
			{
				const auto op = operations[i];
				if (op->GetType() != OperationType::kMeasurement)
				{
					newops.emplace_back(op);
					continue;
				}

				// ok, if it's a measurement, look ahead, accumulate all measurements and then add them in the right order
				std::unordered_set<size_t> bits;
				std::unordered_map<size_t, Types::qubit_t> measQubits;
				std::unordered_map<size_t, Time> measDelays;

				auto affectedBits = op->AffectedBits();
				auto affectedQubits = op->AffectedQubits();

				for (size_t q = 0; q < affectedQubits.size(); ++q)
				{
					bits.insert(affectedBits[q]);
					measQubits[affectedBits[q]] = affectedQubits[q];
					measDelays[affectedBits[q]] = op->GetDelay();
				}

				size_t j = i + 1;
				for (; j < operations.size(); ++j)
				{
					const auto op2 = operations[j];

					if (op2->GetType() != OperationType::kMeasurement)
						break;

					affectedQubits = op2->AffectedQubits();

					const auto meas = std::static_pointer_cast<MeasurementOperation<Time>>(op2);
					affectedBits = meas->GetBitsIndices();
					for (size_t q = 0; q < affectedBits.size(); ++q)
					{
						bits.insert(affectedBits[q]);
						measQubits[affectedBits[q]] = affectedQubits[q];
						measDelays[affectedBits[q]] = op2->GetDelay();
					}
				}

				i = j - 1;

				// the right order is the one following in the classically controlled gates
				for (; j < operations.size(); ++j)
				{
					const auto op2 = operations[j];
					if (op2->GetType() == OperationType::kConditionalGate || op2->GetType() == OperationType::kConditionalMeasurement || op2->GetType() == OperationType::kConditionalRandomGen)
					{
						auto condop = std::static_pointer_cast<IConditionalOperation<Time>>(op2);
						const auto condbits = condop->AffectedBits();
						for (const auto bit : condbits)
							if (bits.find(bit) != bits.end())
							{
								newops.emplace_back(std::make_shared<MeasurementOperation<Time>>(std::vector{ std::make_pair(measQubits[bit], bit) }, measDelays[bit]));
								bits.erase(bit);
							}
					}
					if (bits.empty()) break;
				}

				// now add the measurements that were left in any order
				for (auto bit : bits)
					newops.emplace_back(std::make_shared<MeasurementOperation<Time>>(std::vector{ std::make_pair(measQubits[bit], bit) }, measDelays[bit]));
			}

			operations.swap(newops);
		}

		/**
		 * @brief Returns the max qubit id for all operations
		 *
		 * Returns the max qubit id for all operations.
		 * 
		 * @return The max qubit id for all operations
		 */
		size_t GetMaxQubitIndex() const
		{
			size_t mx = 0;
			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				for (auto q : qbits)
					if (q > mx) mx = q;
			}

			return mx;
		}

		/**
		 * @brief Returns the min qubit id for all operations
		 *
		 * Returns the min qubit id for all operations.
		 *
		 * @return The min qubit id for all operations
		 */
		size_t GetMinQubitIndex() const
		{
			size_t mn = std::numeric_limits<size_t>::max();
			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				for (auto q : qbits)
					if (q < mn) mn = q;
			}

			return mn;
		}

		/**
		 * @brief Returns the max classical bit id for all operations
		 *
		 * Returns the max classical bit id for all operations.
		 *
		 * @return The max classical bit id for all operations
		 */
		size_t GetMaxCbitIndex() const
		{
			size_t mx = 0;
			for (const auto& op : operations)
			{
				const auto cbits = op->AffectedBits();
				for (auto q : cbits)
					if (q > mx) mx = q;
			}

			return mx;
		}

		/**
		 * @brief Returns the min classical bit id for all operations
		 *
		 * Returns the min classical bit id for all operations.
		 *
		 * @return The min classical bit id for all operations
		 */
		size_t GetMinCbitIndex() const
		{
			size_t mn = std::numeric_limits<size_t>::max();
			for (const auto& op : operations)
			{
				const auto cbits = op->AffectedBits();
				for (auto q : cbits)
					if (q < mn) mn = q;
			}

			return mn;
		}

		/**
		 * @brief Returns the qubits affected by the operations
		 *
		 * Returns the qubits affected by the operations.
		 * They might not cover all qubits, there might be gaps.
		 * In such cases a remapping might be needed, for example in a scheduler.
		 * 
		 * @return The qubits affected by the operations
		 */
		std::set<size_t> GetQubits() const
		{
			std::set<size_t> qubits;
			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				qubits.insert(qbits.begin(), qbits.end());
			}

			return qubits;
		}

		/**
		 * @brief Returns the classical bits affected by the operations
		 *
		 * Returns the classical bits affected by the operations.
		 *
		 * @return The bits affected by the operations
		 */
		std::set<size_t> GetBits() const
		{
			std::set<size_t> cbits;
			for (const auto& op : operations)
			{
				const auto bits = op->AffectedBits();
				cbits.insert(bits.begin(), bits.end());
			}

			return cbits;
		}

		/**
		 * @brief Returns the affected qubits.
		 *
		 * Returns the affected qubits by the operation.
		 * @return A vector with the affected qubits.
		 */
		Types::qubits_vector AffectedQubits() const override
		{
			auto qubits = GetQubits();

			Types::qubits_vector qubitsVec;
			qubitsVec.reserve(qubits.size());

			for (auto q : qubits)
				qubitsVec.emplace_back(q);

			return qubitsVec;
		}

		/**
		 * @brief Returns the affected bits.
		 *
		 * Returns the affected classical bits.
		 * @return The affected bits.
		 */
		std::vector<size_t> AffectedBits() const override
		{
			auto bits = GetBits();

			std::vector<size_t> bitsVec;
			bitsVec.reserve(bits.size());

			for (auto b : bits)
				bitsVec.emplace_back(b);

			return bitsVec;
		}

		/**
		 * @brief Find if the circuit needs entanglement for distribution.
		 *
		 * Returns true if the circuit needs entanglement for distribution, false otherwise.
		 * 
		 * @return True if the circuit needs entanglement for distribution, false otherwise.
		 */
		bool NeedsEntanglementForDistribution() const override
		{
			for (const auto& op : operations)
				if (op->NeedsEntanglementForDistribution()) 
					return true;
			
			return false;
		}

		/**
		 * @brief Find if the circuit can affect the quantum state.
		 *
		 * Returns true if the circuit can affect the quantum state, false otherwise.
		 * 
		 * @return True if the circuit can affect the quantum state, false otherwise.
		 */
		bool CanAffectQuantumState() const override
		{
			for (const auto& op : operations)
				if (op->CanAffectQuantumState())
					return true;

			return false;
		}

		/**
		 * @brief Returns the last operations on circuit's qubits.
		 * 
		 * Returns the last operations on circuit's qubits.
		 * 
		 * @return The last operations on circuit's qubits
		 */
		std::unordered_map<size_t, OperationPtr> GetLastOperationsOnQubits() const
		{
			std::unordered_map<size_t, OperationPtr> lastOps;
			
			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				for (auto q : qbits)
					lastOps[q] = op;
			}

			return lastOps;
		}

		/**
		 * @brief Returns the first operations on circuit's qubits.
		 * 
		 * Returns the first operations on circuit's qubits.
		 * 
		 * @return The first operations on circuit's qubits
		 */
		std::unordered_map<size_t, OperationPtr> GetFirstOperationsOnQubits() const
		{
			std::unordered_map<size_t, OperationPtr> firstOps;
			
			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				for (auto q : qbits)
				{
					if (firstOps.find(q) == firstOps.end())
						firstOps[q] = op;
				}
			}

			return firstOps;
		}

		/**
		 * @brief Add resets at the end of the circuit
		 *
		 * Adds resets at the end of the circuit, on the qubits that don't have a reset operation already.
		 * 
		 * @param delay The delay for the reset operation
		 */
		void AddResetsIfNeeded(Time delay = 0)
		{
			const auto GetLastOps = GetLastOperationsOnQubits();

			for (const auto& [q, op] : GetLastOps)
				if (op->GetType() != OperationType::kReset) // don't add it if there is already a reset operation on the qubit
					operations.emplace_back(std::make_shared<Reset<Time>>(Types::qubits_vector{ q }, delay));
		}

		/**
		 * @brief Add resets at the beginning of the circuit
		 *
		 * Adds resets at the beginning of the circuit, on the qubits that don't have a reset operation already.
		 * 
		 * @param delay The delay for the reset operation
		 */
		void AddResetsAtBeginningIfNeeded(Time delay = 0)
		{
			const auto GetFirstOps = GetFirstOperationsOnQubits();

			for (const auto& [q, op] : GetFirstOps)
				if (op->GetType() != OperationType::kReset) // don't add it if there is already a reset operation on the qubit
					operations.insert(operations.begin(), std::make_shared<Reset<Time>>(Types::qubits_vector{ q }, delay));
		}

		/**
		 * @brief Circuit optimization
		 * 
		 * Optimizes the circuit.
		 * See qisikit aer for 'transpilling' when the circuit is flushed for some ideas.
		 */
		void Optimize(bool optimizeRotationGates = true)
		{
			// Some ideas, from simple to more complex:
			// 
			// IMPORTANT: Focus on the gates that are added for distributed computing, either for the one with entanglement or the one with cutting
			// the reason is that maybe the provided circuit is not that bad, but due of the supplementary gates added, duplicates (for example) will occur 
			// the most important one qubit ones added are hadamard
			// then X, S, Sdag and Z
			// 
			// 1. First, one qubit gates can be combined into a single gate or even no gate
			// a) Straightforward for those that are their own inverse (that is, hermitian/involutory): Hadamard and Pauli gates for example, if one finds two of them in sequence, they can be removed
			// other ones are the one that are followed by their 'dag' (inverse, since the gates are unitary) in the circuit, those can be removed, too

			// several resets can be also changed into a single one, also repeated measurements of the same qubit, with result in the same cbit can be replaced by a single measurement
			
			// b) other ones can be combined into a single gate, for example phase shift gates
			// or rotation gates 
			// two phase gates (not the general phase shift we have, but the one with 1, i on the diagonal) can be combined into a Z gate, for example, or two sqrtNot gates can be combined into a single X gate
			// combinations of phase gates and hadamard can be replaced by pauli gates in some cases, and so on
			// even the U gate could be used to join together several one qubit gates
			
			// c) even more complex... three or more one qubit gates could be combined into a single gate... an example is HXH = Z
			// other examples SXS^t = Y, SZS^t = Z

			// 2. Two qubit gates can be optimized, too
			// for example two CNOT gates in sequence can be removed if the control qubit is the same, the same goes for two CZ or CY or SWAP gates (this goes for the three qubit gates, CCX, CCY, CCZ, CSWAP, too)

			// 3. Some gates commute, the reorder can give some opportunities for more optimization

			// 4. Groups of gates
			// the possibilities are endless, but might be easier to focus first on Clifford gates
			// for example a X sandwiched between CNOTs can be replaced with two X on each qubit if the original X is on the control qubit
			// or with an X on the target qubit if the original X is on the target qubit
			// a similar thing happens if Z is sandwiched between CNOTs, but this time Z x Z appears if the original Z is on the target qubit
			// and if original Z is on the control qubit, then the CNOTs dissapear and the Z remains on the control qubit

			// three CNOT gates with the one in the middle turned upside down compare with the other two can be replaced by a single swap gate

			// first stage, take out duplicates of H, X, Y, Z

			bool changed;

			do
			{
				changed = false;

				std::vector<std::shared_ptr<IOperation<Time>>> newops;
				newops.reserve(operations.size());

				for (int i = 0; i < static_cast<int>(operations.size()); ++i)
				{
					const std::shared_ptr<IOperation<Time>>& op = operations[i];

					const auto type = op->GetType();
					if (type == OperationType::kNoOp) continue;
					else if (type == OperationType::kGate)
					{
						std::shared_ptr<IQuantumGate<Time>> gate = std::static_pointer_cast<IQuantumGate<Time>>(op);
						const auto qubits = gate->AffectedQubits();

						if (qubits.size() == 1)
						{
							// TODO: HXH = Z, SXS^t = Y, SZS^t = Z ????
							
							auto gateType = gate->GetGateType();
							bool replace = false;

							// if it's one of the interesting gates, look ahead to see if it's followed by the same gate on the same qubit
							// if yes, replace the next one with a nop and skip the current one (or replace the pair with a single gate, depending on the type)
							// set changed to true if something was changed
							switch (gateType)
							{
							case QuantumGateType::kRxGateType:
								[[fallthrough]];
							case QuantumGateType::kRyGateType:
								[[fallthrough]];
							case QuantumGateType::kRzGateType:
								if (!optimizeRotationGates)
								{
									newops.push_back(op);
									break;
								}
								[[fallthrough]];
							case QuantumGateType::kPhaseGateType:
								replace = true;
								[[fallthrough]];
							// those above will be replaced the pair with a single gate, all the following are the ones that get removed 
							case QuantumGateType::kHadamardGateType:
								[[fallthrough]];
							case QuantumGateType::kXGateType:
								[[fallthrough]];
							case QuantumGateType::kYGateType:
								[[fallthrough]];
							case QuantumGateType::kZGateType:
								[[fallthrough]];
							case QuantumGateType::kKGateType:
								[[fallthrough]];
							case QuantumGateType::kSGateType:
								[[fallthrough]];
							case QuantumGateType::kSdgGateType:
								[[fallthrough]];
							case QuantumGateType::kTGateType:
								[[fallthrough]];
							case QuantumGateType::kTdgGateType:
								[[fallthrough]];
							case QuantumGateType::kSxGateType:
								[[fallthrough]];
							case QuantumGateType::kSxDagGateType:
								{
									bool found = false;

									if (gateType == QuantumGateType::kSGateType)
										gateType = QuantumGateType::kSdgGateType;
									else if (gateType == QuantumGateType::kSdgGateType)
										gateType = QuantumGateType::kSGateType;
									else if (gateType == QuantumGateType::kTGateType)
										gateType = QuantumGateType::kTdgGateType;
									else if (gateType == QuantumGateType::kTdgGateType)
										gateType = QuantumGateType::kTGateType;
									else if (gateType == QuantumGateType::kSxGateType)
										gateType = QuantumGateType::kSxDagGateType;
									else if (gateType == QuantumGateType::kSxDagGateType)
										gateType = QuantumGateType::kSxGateType;
									
									for (size_t j = i + 1; j < operations.size(); ++j)
									{
										auto& nextOp = operations[j];
										if (!nextOp->CanAffectQuantumState())
											continue;

										const auto nextQubits = nextOp->AffectedQubits();
										bool hasQubit = false;

										for (auto q : nextQubits)
											if (q == qubits[0])
											{
												hasQubit = true;
												break;
											}

										if (!hasQubit) continue; // an op that does not touch the current qubit can be skipped
										else if (nextQubits.size() != 1) break; // if it touches the current qubit and it's something else than a single qubit gate, stop

										const auto nextType = nextOp->GetType();
										if (nextType != OperationType::kGate) break; // could be a classically conditioned gate, stop

										const auto& nextGate = std::static_pointer_cast<SingleQubitGate<Time>>(nextOp);
										if (nextGate->GetGateType() ==	gateType)
										{
											if (replace)
											{
												const auto params1 = gate->GetParams();
												const auto params2 = nextGate->GetParams();

												const double param = params1[0] + params2[0];
												const auto delay = gate->GetDelay() + nextGate->GetDelay();

												if (gateType == QuantumGateType::kPhaseGateType)
													newops.push_back(std::make_shared<PhaseGate<Time>>(qubits[0], param, delay));
												else if (gateType == QuantumGateType::kRxGateType)
													newops.push_back(std::make_shared<RxGate<Time>>(qubits[0], param, delay));
												else if (gateType == QuantumGateType::kRyGateType)
													newops.push_back(std::make_shared<RyGate<Time>>(qubits[0], param, delay));
												else
													newops.push_back(std::make_shared<RzGate<Time>>(qubits[0], param, delay));
											}
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if ((gateType == QuantumGateType::kSGateType && nextGate->GetGateType() == QuantumGateType::kSdgGateType) || 
											(gateType == QuantumGateType::kSdgGateType && nextGate->GetGateType() == QuantumGateType::kSGateType))
										{
											// if expecting an S gate (or a Sdg gate) and found the original one instead, replace the pair with a Z gate (S * S = Z, Sdag * Sdag = Z)
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											newops.push_back(std::make_shared<ZGate<Time>>(qubits[0], delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if ((gateType == QuantumGateType::kSxGateType && nextGate->GetGateType() == QuantumGateType::kSxDagGateType) ||
											(gateType == QuantumGateType::kSxDagGateType && nextGate->GetGateType() == QuantumGateType::kSxGateType))
										{
											// if expecting an S gate (or a Sdg gate) and found the original one instead, replace the pair with a X gate (Sx * Sx = X, SXdag * SXdag = X)
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											newops.push_back(std::make_shared<XGate<Time>>(qubits[0], delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if (gateType == QuantumGateType::kTGateType && nextGate->GetGateType() == QuantumGateType::kTdgGateType)
										{
											// if expecting a T gate and found the Tdgate instead, replace the pair with a Sdag gate (Tdg * Tdg = Sdag)
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											newops.push_back(std::make_shared<SdgGate<Time>>(qubits[0], delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if (gateType == QuantumGateType::kTdgGateType && nextGate->GetGateType() == QuantumGateType::kTGateType)
										{
											// if expecting a Tdg gate and found the T gate instead, replace the pair with a S gate (T * T = S)
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											newops.push_back(std::make_shared<SGate<Time>>(qubits[0], delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if (gateType == QuantumGateType::kPhaseGateType &&
											(nextGate->GetGateType() == QuantumGateType::kSGateType || nextGate->GetGateType() == QuantumGateType::kSdgGateType || nextGate->GetGateType() == QuantumGateType::kTGateType || nextGate->GetGateType() == QuantumGateType::kTdgGateType))
										{
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											double param2;
											if (nextGate->GetGateType() == QuantumGateType::kSGateType)
												param2 = 0.5 * M_PI;
											else if (nextGate->GetGateType() == QuantumGateType::kSdgGateType)
												param2 = -0.5 * M_PI;
											else if (nextGate->GetGateType() == QuantumGateType::kTGateType)
												param2 = 0.25 * M_PI;
											else
												param2 = -0.25 * M_PI;

											const auto param = gate->GetParams()[0] + param2;
											newops.push_back(std::make_shared<PhaseGate<Time>>(qubits[0], param, delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else if (nextGate->GetGateType() == QuantumGateType::kPhaseGateType &&
											(gateType == QuantumGateType::kSGateType || gateType == QuantumGateType::kSdgGateType || gateType == QuantumGateType::kTGateType || gateType == QuantumGateType::kTdgGateType))
										{
											const auto delay = gate->GetDelay() + nextGate->GetDelay();
											double param1;
											if (gateType == QuantumGateType::kSGateType)
												param1 = -0.5 * M_PI;
											else if (gateType == QuantumGateType::kSdgGateType)
												param1 = 0.5 * M_PI;
											else if (gateType == QuantumGateType::kTGateType)
												param1 = -0.25 * M_PI;
											else
												param1 = 0.25 * M_PI;

											const auto param = nextGate->GetParams()[0] + param1;
											newops.push_back(std::make_shared<PhaseGate<Time>>(qubits[0], param, delay));
											nextOp = std::make_shared<NoOperation<Time>>();
											changed = true;
											found = true;
											break;
										}
										else break; // not the expected gate, acting on same qubit, bail out
									}
									
									if (!found) newops.push_back(op);
								}
								break;
							default:
								// if no, just add it
								newops.push_back(op);
								break;
							}
						}
						else if (qubits.size() == 2)
						{
							auto gateType = gate->GetGateType();
							bool replace = false;

							// if it's one of the interesting gates, look ahead to see if it's followed by the same gate on the same qubit
							// if yes, replace the next one with a nop and skip the current one (or replace the pair with a single gate, depending on the type)
							// set changed to true if something was changed
							switch (gateType)
							{
							case QuantumGateType::kCRxGateType:
								[[fallthrough]];
							case QuantumGateType::kCRyGateType:
								[[fallthrough]];
							case QuantumGateType::kCRzGateType:
								if (!optimizeRotationGates)
								{
									newops.push_back(op);
									break;
								}
								[[fallthrough]];
							case QuantumGateType::kCPGateType:
								replace = true;
								[[fallthrough]];
								// those above will be replaced the pair with a single gate, all the following are the ones that get removed 
							case QuantumGateType::kCXGateType:
								[[fallthrough]];
							case QuantumGateType::kCYGateType:
								[[fallthrough]];
							case QuantumGateType::kCZGateType:
								[[fallthrough]];
							case QuantumGateType::kCHGateType:
								[[fallthrough]];
							case QuantumGateType::kSwapGateType:
								[[fallthrough]];
							case QuantumGateType::kCSxGateType:
								[[fallthrough]];
							case QuantumGateType::kCSxDagGateType:
							{
								bool found = false;

								if (gateType == QuantumGateType::kCSxGateType)
									gateType = QuantumGateType::kCSxDagGateType;
								else if (gateType == QuantumGateType::kCSxDagGateType)
									gateType = QuantumGateType::kCSxGateType;

								// looking forward for the next operation that acts on the same qubits
								for (size_t j = i + 1; j < operations.size(); ++j)
								{
									auto& nextOp = operations[j];
									if (!nextOp->CanAffectQuantumState())
										continue;

									const auto nextQubits = nextOp->AffectedQubits();

									bool hasQubit = false;

									for (auto q : nextQubits)
										if (q == qubits[0] || q == qubits[1])
										{
											hasQubit = true;
											break;
										}

									if (!hasQubit) continue; // an op that does not touch the current qubit can be skipped
									else if (nextQubits.size() != 2) break; // if it touches a current qubit and it's something else than a two qubits gate, stop
									// if it's not the same qubits, bail out
									else if (gateType == QuantumGateType::kSwapGateType && !((qubits[0] == nextQubits[0] && qubits[1] == nextQubits[1]) || (qubits[0] == nextQubits[1] && qubits[1] == nextQubits[0])))
										break;
									else if (!(qubits[0] == nextQubits[0] && qubits[1] == nextQubits[1])) break;

									const auto nextType = nextOp->GetType();
									if (nextType != OperationType::kGate) break; // could be a classically conditioned gate, stop

									const auto& nextGate = std::static_pointer_cast<TwoQubitsGate<Time>>(nextOp);
									if (nextGate->GetGateType() == gateType)
									{
										if (replace)
										{
											const auto params1 = gate->GetParams();
											const auto params2 = nextGate->GetParams();
											const double param = params1[0] + params2[0];
											const auto delay = gate->GetDelay() + nextGate->GetDelay();

											if (gateType == QuantumGateType::kCPGateType)
												newops.push_back(std::make_shared<CPGate<Time>>(qubits[0], qubits[1], param, delay));
											else if (gateType == QuantumGateType::kCRxGateType)
												newops.push_back(std::make_shared<CRxGate<Time>>(qubits[0], qubits[1], param, delay));
											else if (gateType == QuantumGateType::kCRyGateType)
												newops.push_back(std::make_shared<CRyGate<Time>>(qubits[0], qubits[1], param, delay));
											else
												newops.push_back(std::make_shared<CRzGate<Time>>(qubits[0], qubits[1], param, delay));
										}
										nextOp = std::make_shared<NoOperation<Time>>();
										changed = true; // continue merging gates, we found one that was merged/removed
										found = true; // don't put op in the new operations, we handled it
										break;
									}
									else break; // not the expected gate, acting on same qubits, bail out
								} // end for of looking forward

								if (!found) newops.push_back(op);
							}
							break;
							default:
								// if no, just add it
								newops.push_back(op);
								break;
							}
						}
						else if (qubits.size() == 3)
						{
							auto gateType = gate->GetGateType();

							// if it's one of the interesting gates, look ahead to see if it's followed by the same gate on the same qubit
							// if yes, replace the next one with a nop and skip the current one (or replace the pair with a single gate, depending on the type)
							// set changed to true if something was changed
							switch (gateType)
							{
							case QuantumGateType::kCSwapGateType:
								[[fallthrough]];
							case QuantumGateType::kCCXGateType:
							{
								bool found = false;

								for (size_t j = i + 1; j < operations.size(); ++j)
								{
									auto& nextOp = operations[j];
									if (!nextOp->CanAffectQuantumState())
										continue;

									const auto nextQubits = nextOp->AffectedQubits();
									
									bool hasQubit = false;

									for (auto q : nextQubits)
										if (q == qubits[0] || q == qubits[1] || q == qubits[2])
										{
											hasQubit = true;
											break;
										}

									if (!hasQubit) continue; // an op that does not touch the current qubit can be skipped
									else if (nextQubits.size() != 3) break; // if it touches a current qubit and it's something else than a three qubits gate, stop
									// if it's not the same qubits, bail out
									else if (gateType == QuantumGateType::kCSwapGateType && (qubits[0] != nextQubits[0] ||
										!((qubits[1] == nextQubits[1] && qubits[2] == nextQubits[2]) || (qubits[1] == nextQubits[2] && qubits[2] == nextQubits[1])))) break;
									else if (gateType == QuantumGateType::kCCXGateType && (qubits[2] != nextQubits[2] ||
										!(qubits[1] == nextQubits[1] && qubits[2] == nextQubits[2]) || !(qubits[1] == nextQubits[2] && qubits[2] == nextQubits[1]))) break;

									const auto nextType = nextOp->GetType();
									if (nextType != OperationType::kGate) break; // could be a classically conditioned gate, stop

									const auto& nextGate = std::static_pointer_cast<ThreeQubitsGate<Time>>(nextOp);
									if (nextGate->GetGateType() == gateType)
									{
										nextOp = std::make_shared<NoOperation<Time>>();
										changed = true;
										found = true;
										break;
									}
									else break; // not the expected gate, acting on same qubits, bail out
								}

								if (!found) newops.push_back(op);
							}
							break;
							default:
								// if no, just add it
								newops.push_back(op);
								break;
							}
						}
						else newops.push_back(op);
					}
					else newops.push_back(op);
				} // end for on circuit operations

				operations.swap(newops);
			} while (changed);
		}

		/**
		 * @brief Move the measurements and resets closer to the beginning of the circuit
		 *
		 * Moves the measurements and resets closer to the beginning of the circuit.
		 */
		void MoveMeasurementsAndResets()
		{
			OperationsVector newops;
			newops.reserve(operations.size());

			size_t qubitsNo = std::max(GetMaxQubitIndex(), GetMaxCbitIndex()) + 1;

			std::unordered_map<Types::qubit_t, std::vector<OperationPtr>> qubitOps;
			
			std::vector<OperationPtr> lastOps(qubitsNo);

			std::unordered_map<OperationPtr, std::unordered_set<OperationPtr>> dependenciesMap;

			for (const auto& op : operations)
			{
				std::unordered_set<OperationPtr> dependencies;

				const auto cbits = op->AffectedBits();
				for (auto c : cbits)
				{
					const auto lastOp = lastOps[c];
					if (lastOp)
						dependencies.insert(lastOp);
				}

				const auto qubits = op->AffectedQubits();
				for (auto q : qubits)
				{
					qubitOps[q].push_back(op);

					const auto lastOp = lastOps[q];
					if (lastOp)
						dependencies.insert(lastOp);

					lastOps[q] = op;
				}

				for (auto c : cbits)
					lastOps[c] = op;

				dependenciesMap[op] = dependencies;
			}
			lastOps.clear();

			
			std::vector<Types::qubit_t> indices(qubitsNo, 0);

			while (!dependenciesMap.empty())
			{
				OperationPtr nextOp;

				// try to locate a 'next' gate for a qubit that is either a measurement or a reset
				for (size_t q = 0; q < qubitsNo; ++q)
				{
					if (qubitOps.find(q) == qubitOps.end()) // no operation left on this qubit
						continue;

					// grab the current operation for this qubit
					const auto& ops = qubitOps[q];
					const auto& op = ops[indices[q]];

					// consider only measurements and resets
					if (op->GetType() == OperationType::kMeasurement || op->GetType() == OperationType::kReset)
					{
						bool hasDependencies = false;

						for (const auto& opd : dependenciesMap[op])
							if (dependenciesMap.find(opd) != dependenciesMap.end())
							{
								hasDependencies = true;
								break;
							}

						if (!hasDependencies)
						{
							nextOp = op;
							break;
						}
					}
				}

				if (nextOp)
				{
					dependenciesMap.erase(nextOp);

					const auto qubits = nextOp->AffectedQubits();
					for (auto q : qubits)
					{
						++indices[q];
						if (indices[q] >= qubitOps[q].size())
							qubitOps.erase(q);
					}

					newops.emplace_back(std::move(nextOp));
					continue;
				}

				// if there is no measurement or reset, add the next gate
				for (Types::qubit_t q = 0; q < qubitsNo; ++q)
				{
					if (qubitOps.find(q) == qubitOps.end()) // no operation left on this qubit
						continue;

					// grab the current operation for this qubit
					const auto& ops = qubitOps[q];
					const auto& op = ops[indices[q]];

					bool hasDependencies = false;
					
					for (const auto& opd : dependenciesMap[op])
						if (dependenciesMap.find(opd) != dependenciesMap.end())
						{
							hasDependencies = true;
							break;
						}

					if (!hasDependencies)
					{
						nextOp = op;
						break;
					}
				}

				if (nextOp)
				{
					dependenciesMap.erase(nextOp);

					const auto qubits = nextOp->AffectedQubits();
					for (auto q : qubits)
					{
						++indices[q];
						if (indices[q] >= qubitOps[q].size())
							qubitOps.erase(q);
					}

					newops.emplace_back(std::move(nextOp));
				}
			}

			assert(newops.size() == operations.size());

			operations.swap(newops);
		}


		/**
		 * @brief Get circuit depth
		 *
		 * Returns the depth of the circuit as a pair of the depth and an estimate of time cost for each qubit.
		 * Should be considered only as an estimate.
		 *
		 * @return The depth of the circuit as a pair of the depth and an estimate of time cost for each qubit.
		 */
		std::pair<std::vector<size_t>, std::vector<Time>> GetDepth() const
		{
			size_t maxDepth;
			Time maxTime;

			size_t qubitsNo = GetMaxQubitIndex() + 1;
			std::vector<Time> qubitTimes(qubitsNo, 0);
			std::vector<size_t> qubitDepths(qubitsNo, 0);

			std::unordered_map<size_t, size_t> fromQubits;

			for (const auto& op : operations)
			{
				const auto qbits = op->AffectedQubits();
				const auto delay = op->GetDelay();

				maxTime = 0;
				maxDepth = 0;
				for (auto q : qbits)
				{
					qubitTimes[q] += delay;
					++qubitDepths[q];
					if (qubitTimes[q] > maxTime) maxTime = qubitTimes[q];
					if (qubitDepths[q] > maxDepth) maxDepth = qubitDepths[q];
				}

				const auto t = op->GetType();
				std::vector<size_t> condbits;

				// TODO: deal with 'random gen' operations, those do not affect qubits directly, but they do affect the classical bits
				// and can be used in conditional operations

				if (t == OperationType::kConditionalGate || t == OperationType::kConditionalMeasurement)
				{
					condbits = op->AffectedBits();

					for (auto bit : condbits)
					{
						if (fromQubits.find(bit) != fromQubits.end())
							bit = fromQubits[bit];

						bool found = false;
						for (auto q : qbits)
						{
							if (q == bit)
							{
								found = true;
								break;
							}
						}
						if (found || bit >= qubitsNo) continue;

						qubitTimes[bit] += delay;
						++qubitDepths[bit];
						if (qubitTimes[bit] > maxTime) maxTime = qubitTimes[bit];
						if (qubitDepths[bit] > maxDepth) maxDepth = qubitDepths[bit];
					}

					if (t == OperationType::kConditionalMeasurement)
					{
						const auto condMeas = std::static_pointer_cast<ConditionalMeasurement<Time>>(op);
						const auto meas = condMeas->GetOperation();
						const auto measQubits = meas->AffectedQubits();
						const auto measBits = meas->AffectedBits();

						for (size_t i = 0; i < measQubits.size(); ++i)
						{
							if (i < measBits.size())
								fromQubits[measBits[i]] = measQubits[i];
							else
								fromQubits[measQubits[i]] = measQubits[i];
						}
					}
				}
				else if (t == OperationType::kMeasurement)
				{
					condbits = op->AffectedBits();

					for (size_t i = 0; i < qbits.size(); ++i)
					{
						if (i < condbits.size())
							fromQubits[condbits[i]] = qbits[i];
						else
							fromQubits[qbits[i]] = qbits[i];
					}
				}

				for (auto q : qbits)
				{
					qubitTimes[q] = maxTime;
					qubitDepths[q] = maxDepth;
				}

				for (auto bit : condbits)
				{
					qubitTimes[bit] = maxTime;
					qubitDepths[bit] = maxDepth;
				}
			}

			return std::make_pair(qubitDepths, qubitTimes);
		}

		/**
		 * @brief Get max circuit depth
		 *
		 * Returns the max depth of the circuit as a pair of the max depth and an estimate of max time cost.
		 * Should be considered only as an estimate.
		 * 
		 * @return The max depth of the circuit as a pair of the max depth and an estimate of time cost.
		 */
		std::pair<size_t, Time> GetMaxDepth() const
		{
			auto [qubitDepths, qubitTimes] = GetDepth();
			
			Time maxTime = 0;
			size_t maxDepth = 0;
			for (size_t qubit = 0; qubit < qubitDepths.size(); ++qubit)
			{
				if (qubitTimes[qubit] > maxTime) maxTime = qubitTimes[qubit];
				if (qubitDepths[qubit] > maxDepth) maxDepth = qubitDepths[qubit];
			}

			return std::make_pair(maxDepth, maxTime);
		}

		/**
		 * @brief Get the number of operations in the circuit.
		 *
		 * Returns the number of operations in the circuit.
		 * @return The number of operations in the circuit.
		 */
		size_t GetNumberOfOperations() const
		{
			return operations.size();
		}

		/**
		 * @brief Get an operation at a given position.
		 *
		 * Returns the operation at the given position.
		 * 
		 * @param pos The position of the operation to get.
		 * @return The operation at the given position.
		 */
		OperationPtr GetOperation(size_t pos) const
		{
			if (pos >= operations.size())
				return nullptr;

			return operations[pos];
		
		}

		/**
		 * @brief Get the circuit cut.
		 * 
		 * Cuts out a circuit from the current circuit, starting from the startQubit and ending at the endQubit.
		 * Throws an exception if there is an operation that affects both qubits inside and outside of the specified interval.
		 * The returned circuit contains only operations that act on the qubits in the specified interval.
		 * 
		 * @param startQubit The start qubit of the interval.
		 * @param endQubit The end qubit of the interval.
		 * @return The circuit cut.
		 */
		std::shared_ptr<Circuit<Time>> GetCircuitCut(Types::qubit_t startQubit, Types::qubit_t endQubit) const
		{
			OperationsVector newops;
			newops.reserve(operations.size());

			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();
				bool containsOutsideQubits = false;
				bool containsInsideQubits = false;
				for (const auto q : qubits)
				{
					if (q < startQubit || q > endQubit)
					{
						containsOutsideQubits = true;
						if (containsInsideQubits) break;
					}
					else
					{
						containsInsideQubits = true;
						if (containsOutsideQubits) break;
					}
				}

				if (containsInsideQubits)
				{
					if (containsOutsideQubits)
						throw std::runtime_error("Cannot cut the circuit with the specified interval");
					newops.emplace_back(op->Clone());
				}
			}

			return std::make_shared<Circuit<Time>>(newops);
		}

		/**
		* @brief Checks if the circuit has measurements that are followed by operations that affect the measured qubits.
		* 
		* Checks if the circuit has measurements that are followed by operations that affect the measured qubits.
		* 
		* @return True if the circuit has measurements that are followed by operations that affect the measured qubits, false otherwise.
		*/
		bool HasOpsAfterMeasurements() const
		{
			std::unordered_set<Types::qubit_t> measuredQubits;
			std::unordered_set<Types::qubit_t> affectedQubits;
			std::unordered_set<Types::qubit_t> resetQubits;

			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();

				if (op->GetType() == OperationType::kMeasurement)
				{
					for (const auto qbit : qubits)
						if (resetQubits.find(qbit) != resetQubits.end()) // there is a reset on this qubit already and it's not at the beginning of the circuit
							return true;

					/*
					const auto bits = op->AffectedBits();
					if (bits.size() != qubits.size())
						return true;

					for (size_t b = 0; b < bits.size(); ++b)
						if (bits[b] != qubits[b])
							return true;
					*/
					measuredQubits.insert(qubits.begin(), qubits.end());
				}
				else if (op->GetType() == OperationType::kConditionalGate || op->GetType() == OperationType::kConditionalMeasurement || op->GetType() == OperationType::kRandomGen || op->GetType() == OperationType::kConditionalRandomGen)
					return true;
				else if (op->GetType() == OperationType::kReset)
				{
					// resets in the middle of the circuit are treated as measurements
					for (const auto qbit : qubits)
					{
						// if there is already a gate applied on the qubit but no measurement yet, it's considered in the middle
						// if there is no gate applied, then it's the first operation on the qubit
						if (affectedQubits.find(qbit) != affectedQubits.end() || measuredQubits.find(qbit) != measuredQubits.end())
							resetQubits.insert(qbit);

						affectedQubits.insert(qbit);
					}
				}
				else 
				{
					for (const auto qbit : qubits)
					{
						if (measuredQubits.find(qbit) != measuredQubits.end()) // there is a measurement on this qubit already
							return true;

						if (resetQubits.find(qbit) != resetQubits.end()) // there is a reset on this qubit already and it's not at the beginning of the circuit
							return true;

						affectedQubits.insert(qbit);
					}
				}
			}

			return false;
		}

		/**
		 * @brief Execute the non-measurements operations from the circuit on the given simulator.
		 *
		 * Execute the non-measurements operations from the circuit on the given simulator.
		 * @param sim The simulator to execute the circuit on.
		 * @param state The classical state containing the classical bits.
		 * @return A bool vector with the executed operations marked. 
		 * @sa ISimulator
		 * @sa OperationState
		 */
		std::vector<bool> ExecuteNonMeasurements(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state) const
		{
			std::vector<bool> executedOps;
			executedOps.reserve(operations.size());

			std::unordered_set<Types::qubit_t> measuredQubits;
			std::unordered_set<Types::qubit_t> affectedQubits;

			bool executionStopped = false;

			for (size_t i = 0; i < operations.size(); ++i)
			{
				auto& op = operations[i];
				const auto qubits = op->AffectedQubits();

				bool executed = false;

				if (op->GetType() == OperationType::kMeasurement || op->GetType() == OperationType::kConditionalMeasurement || op->GetType() == OperationType::kRandomGen || op->GetType() == OperationType::kConditionalRandomGen)
					measuredQubits.insert(qubits.begin(), qubits.end());
				else if (op->GetType() == OperationType::kReset)
				{
					// if it's the first op on qubit(s), execute it, otherwise treat it as a measurement
					executed = true;
					for (auto qubit : qubits)
						if (affectedQubits.find(qubit) != affectedQubits.end())
						{
							executed = false;
							break;
						}

					if (executed) {
						if (sim) op->Execute(sim, state);
					}
					else
						measuredQubits.insert(qubits.begin(), qubits.end());
				}
				else // regular gate or conditional gate
				{
					const auto bits = op->AffectedBits();

					// a measurement on a qubit prevents execution of any following gate than affects the same qubit
					// also a gate that's not executed and it would affect certain qubits will prevent the execution of any following gate that affects those qubits

					bool canExecute = op->GetType() == OperationType::kGate;

					if (canExecute) // a conditional gate cannot be executed, it needs something executed at each shot, either a measurement or a random number generated
					{
						for (auto bit : bits)
							if (measuredQubits.find(bit) != measuredQubits.end())
							{
								canExecute = false;
								break;
							}

						for (auto qubit : qubits)
							if (measuredQubits.find(qubit) != measuredQubits.end())
							{
								canExecute = false;
								break;
							}
					}

					if (canExecute)
					{
						if (sim) op->Execute(sim, state);
						executed = true;
					}
					else
					{
						// this is a 'trick', if it cannot execute, then neither can any following gate that affects any of the already involved qubits
						measuredQubits.insert(bits.begin(), bits.end());
						measuredQubits.insert(qubits.begin(), qubits.end());
					}
				}

				affectedQubits.insert(qubits.begin(), qubits.end());
				
				if (!executed) executionStopped = true;
				if (executionStopped) 
					executedOps.emplace_back(executed);
			}

			//if (sim) sim->Flush();

			return executedOps;
		}

		/**
		 * @brief Execute the measurement operations from the circuit on the given simulator.
		 *
		 * Execute the measurements operations from the circuit on the given simulator.
		 * @param sim The simulator to execute the circuit on.
		 * @param state The classical state containing the classical bits.
		 * @sa ISimulator
		 * @sa OperationState
		 */
		void ExecuteMeasurements(const std::shared_ptr<Simulators::ISimulator>& sim, OperationState& state, const std::vector<bool>& executedOps) const
		{
			state.Reset();
			if (!sim) return;

			//if (executedOps.empty() && !operations.empty()) throw std::runtime_error("The executed operations vector is empty");

			const size_t dif = operations.size() - executedOps.size();

			for (size_t i = dif; i < operations.size(); ++i)
				if (!executedOps[i - dif])
					operations[i]->Execute(sim, state);

			//sim->Flush();
		}

		// used internally to optimize measurements in the case of having measurements only at the end of the circuit
		std::shared_ptr<MeasurementOperation<Time>> GetLastMeasurements(const std::vector<bool>& executedOps, bool sort = true) const
		{
			const size_t dif = operations.size() - executedOps.size();
			std::vector<std::pair<Types::qubit_t, size_t>> measurements;
			measurements.reserve(dif);

			for (size_t i = dif; i < operations.size(); ++i)
				if (!executedOps[i - dif] && operations[i]->GetType() == OperationType::kMeasurement)
				{
					auto measOp = std::static_pointer_cast<MeasurementOperation<Time>>(operations[i]);
					const auto& qubits = measOp->GetQubits();
					const auto& bits = measOp->GetBitsIndices();

					for (size_t j = 0; j < qubits.size(); ++j)
						measurements.emplace_back(qubits[j], bits[j]);
				}

			// qiskit aer expects sometimes to have them in sorted order, so...
			if (sort) std::sort(measurements.begin(), measurements.end(), [](const auto& p1, const auto& p2) { return p1.first < p2.first; });

			return std::make_shared<MeasurementOperation<Time>>(measurements);
		}

		/**
		 * @brief Checks if the circuit has clasically conditional operations.
		 *
		 * Checks if the circuit has clasically conditional operations.
		 * 
		 * @return True if the circuit has clasically conditional operations, false otherwise.
		 */
		bool HasConditionalOperations() const
		{
			for (const auto& op : operations)
				if (op->GetType() == OperationType::kConditionalGate || op->GetType() == OperationType::kConditionalMeasurement || op->GetType() == OperationType::kConditionalRandomGen)
					return true;

			return false;
		}

		/**
		 * @brief Checks if the circuit has only operations that act on adjacent qubits.
		 *
		 * Checks if the circuit has only operations that act on adjacent qubits.
		 * Can be useful in picking up a tensor network contractor, for example.
		 * Also such a circuit is easier to simulate in a MPS simulator, since swap gates are not needed to be added to bring up the qubits next to each other before applying a two qubits gate.
		 *
		 * @return True if the circuit has operations that are not gates, false otherwise.
		 */
		bool ActsOnlyOnAdjacentQubits() const
		{
			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();
				if (qubits.size() <= 1)
					continue;

				if (qubits.size() == 2)
				{
					if (std::abs(qubits[0] - qubits[1]) != 1)
						return false;
				}
				else
				{
					Types::qubit_t minQubit = qubits[0];
					Types::qubit_t maxQubit = qubits[0];
				
					for (size_t i = 1; i < qubits.size(); ++i)
					{
						if (qubits[i] < minQubit)
							minQubit = qubits[i];
						else if (qubits[i] > maxQubit)
							maxQubit = qubits[i];
					}

					if (maxQubit - minQubit >= qubits.size())
						return false;
				}
			}

			return true;
		}

		/**
		 * @brief Checks if the circuit is a forest circuit.
		 * 
		 * Checks if the circuit is a forest circuit.
		 * 
		 * @return True if the circuit is a forest circuit, false otherwise.
		 */
		bool IsForest() const
		{
			std::unordered_map<Types::qubit_t, size_t> qubits;
			std::unordered_map<Types::qubit_t, Types::qubits_vector> lastQubits;
			
			for (const auto& op : operations)
			{
				const auto q = op->AffectedQubits();
				// one qubit gates or other operations that do not affect qubits do not change anything
				if (q.size() <= 1)
					continue;

				bool allInTheLastQubits = true;

				for (const auto qubit : q)
				{
					if (lastQubits.find(qubit) == lastQubits.end())
					{
						allInTheLastQubits = false;
						break;
					}
					else {
						const auto& lastQ = lastQubits[qubit];

						for (const auto q1 : q)
							if (std::find(lastQ.cbegin(), lastQ.cend(), q1) == lastQ.cend())
							{
								allInTheLastQubits = false;
								break;
							}

						if (!allInTheLastQubits)
							break;
					}
				}
					
				if (allInTheLastQubits)
					continue;

				for (const auto qubit : q)
				{
					if (qubits[qubit] > 1) // if the qubit is affected again...
						return false;

					++qubits[qubit];

					lastQubits[qubit] = q;
				}
			}

			return true;
		}

		/**
		 * @brief Checks if the circuit is a Clifford circuit.
		 *
		 * Checks if the circuit is a Clifford circuit.
		 * Considers gates as Clifford if they are supported by the Clifford simulator.
		 *
		 * @return True if the circuit is a Clifford circuit, false otherwise.
		 */
		bool IsClifford() const override
		{
			for (const auto& op : operations)
				if (!op->IsClifford())
					return false;

			return true;
		}

		/**
		 * @brief Get the percentage of Clifford operations in the circuit.
		 *
		 * Returns the percentage of Clifford operations in the circuit.
		 * Considers gates as Clifford if they are supported by the Clifford simulator.
		 *
		 * @return The percentage of Clifford operations in the circuit.
		 */
		double CliffordPercentage() const
		{
			size_t cliffordOps = 0;
			for (const auto& op : operations)
				if (op->IsClifford())
					++cliffordOps;

			return static_cast<double>(cliffordOps) / operations.size();
		}

		/**
		 * @brief Get the qubits that are acted on by Clifford operations.
		 *
		 * Returns the qubits that are acted on only by Clifford operations.
		 * Considers gates as Clifford if they are supported by the Clifford simulator.
		 *
		 * @return The qubits that are acted on only by Clifford operations.
		 */
		std::unordered_set<Types::qubit_t> GetCliffordQubits() const
		{
			std::unordered_set<Types::qubit_t> cliffordQubits;
			std::unordered_set<Types::qubit_t> nonCliffordQubits;

			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();
				if (op->IsClifford())
				{
					for (const auto q : qubits)
						cliffordQubits.insert(q);
				}
				else
				{
					for (const auto q : qubits)
						nonCliffordQubits.insert(q);
				}
			}

			for (const auto q : nonCliffordQubits)
				cliffordQubits.erase(q);

			return cliffordQubits;
		}

		/**
		 * @brief Splits a circuit that has disjoint subcircuits in it into separate circuits
		 *
		 * Splits a circuit that has disjoint subcircuits in it into separate circuits.
		 * 
		 * @return A vector of shared pointers to the separate circuits.
		 */
		std::vector<std::shared_ptr<Circuit<Time>>> SplitCircuit() const
		{
			std::vector<std::shared_ptr<Circuit<Time>>> circuits;
			
			// find how many disjoint circuits we have in this circuit

			std::unordered_map<Types::qubit_t, std::unordered_set<Types::qubit_t>> circuitsMap;
			auto allQubits = GetQubits();
			std::unordered_map<Types::qubit_t, Types::qubit_t> qubitCircuitMap;

			// start with a bunch of disjoint sets of qubits, containing each a single qubit

			for (auto qubit : allQubits)
			{
				circuitsMap[qubit] = std::unordered_set<Types::qubit_t>{ qubit };
				qubitCircuitMap[qubit] = qubit;
			}

			// then the gates will join them together into circuits

			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();

				if (qubits.empty()) continue;

				auto qubitIt = qubits.cbegin();
				auto firstQubit = *qubitIt;
				// where is the first qubit in the disjoint sets?
				auto firstQubitCircuit = qubitCircuitMap[firstQubit];

				++qubitIt;

				for (; qubitIt != qubits.cend(); ++qubitIt)
				{
					auto qubit = *qubitIt;

					// where is the qubit in the disjoint sets?
					auto qubitCircuit = qubitCircuitMap[qubit];

					// join the circuits / qubits sets

					if (firstQubitCircuit != qubitCircuit)
					{
						// join the circuits
						circuitsMap[firstQubitCircuit].insert(circuitsMap[qubitCircuit].begin(), circuitsMap[qubitCircuit].end());

						// update the qubit to circuit map
						for (auto q : circuitsMap[qubitCircuit])
							qubitCircuitMap[q] = firstQubitCircuit;

						// remove the joined circuit
						circuitsMap.erase(qubitCircuit);
					}
				}
			}


			size_t circSize = 1ULL;
			circSize = std::max(circSize, circuitsMap.size());
			circuits.resize(circSize);

			for (size_t i = 0; i < circuits.size(); ++i)
				circuits[i] = std::make_shared<Circuit<Time>>();

			std::unordered_map<Types::qubit_t, size_t> qubitsSetsToCircuit;

			size_t circuitNo = 0;
			for (const auto& [id, qubitSet] : circuitsMap)
			{
				qubitsSetsToCircuit[id] = circuitNo;

				++circuitNo;
			}


			// now fill them up with the operations

			for (const auto& op : operations)
			{
				const auto qubits = op->AffectedQubits();

				if (qubits.empty()) 
				{
					circuits[0]->AddOperation(op->Clone());
					continue;
				}

				const auto circ = qubitsSetsToCircuit[qubitCircuitMap[*qubits.cbegin()]];

				circuits[circ]->AddOperation(op->Clone());
			}

			return circuits;
		}


		/**
		 * @brief Get the begin iterator for the operations.
		 *
		 * Returns the begin iterator for the operations.
		 * @return The begin iterator for the operations.
		 */
		iterator begin() noexcept { return operations.begin(); }
		
		/**
		 * @brief Get the end iterator for the operations.
		 *
		 * Returns the end iterator for the operations.
		 * @return The end iterator for the operations.
		 */
		iterator end() noexcept { return operations.end(); }

		/**
		 * @brief Get the const begin iterator for the operations.
		 *
		 * Returns the const begin iterator for the operations.
		 * @return The const begin iterator for the operations.
		 */
		const_iterator cbegin() const noexcept { return operations.cbegin(); }

		/**
		 * @brief Get the const end iterator for the operations.
		 *
		 * Returns the const end iterator for the operations.
		 * @return The const end iterator for the operations.
		 */
		const_iterator cend() const noexcept { return operations.cend(); }

		/**
		 * @brief Get the reverse begin iterator for the operations.
		 *
		 * Returns the reverse begin iterator for the operations.
		 * @return The reverse begin iterator for the operations.
		 */
		reverse_iterator rbegin() noexcept { return operations.rbegin(); }

		/**
		 * @brief Get the reverse end iterator for the operations.
		 *
		 * Returns the reverse end iterator for the operations.
		 * @return The reverse end iterator for the operations.
		 */
		reverse_iterator rend() noexcept { return operations.rend(); }

		/**
		 * @brief Get the const reverse begin iterator for the operations.
		 *
		 * Returns the const reverse begin iterator for the operations.
		 * @return The const reverse begin iterator for the operations.
		 */
		const_reverse_iterator crbegin() const noexcept { return operations.crbegin(); }

		/**
		 * @brief Get the const reverse end iterator for the operations.
		 *
		 * Returns the const reverse end iterator for the operations.
		 * @return The const reverse end iterator for the operations.
		 */
		const_reverse_iterator crend() const noexcept { return operations.crend(); }

		/**
		 * @brief Get the number of operations in the circuit.
		 *
		 * Returns the number of operations in the circuit.
		 * @return The number of operations in the circuit.
		 */
		auto size() const { return operations.size(); }

		/**
		 * @brief Check if the circuit is empty.
		 *
		 * Returns true if the circuit is empty, false otherwise.
		 * @return True if the circuit is empty, false otherwise.
		 */
		auto empty() const { return operations.empty(); }

		/**
		 * @brief Get the operation at a given position.
		 *
		 * Returns the operation at the given position.
		 * @param pos The position of the operation to get.
		 * @return The operation at the given position.
		 */
		auto& operator[](size_t pos) { return operations[pos]; }
		
		/**
		 * @brief Get the operation at a given position.
		 *
		 * Returns the operation at the given position.
		 * @param pos The position of the operation to get.
		 * @return The operation at the given position.
		 */
		const auto& operator[](size_t pos) const { return operations[pos]; }

		/**
		 * @brief Resizes the circuit.
		 *
		 * Resizes the circuit, but it cannot make it larger, only smaller, by removing the last operations.
		 * @param size The new size of the circuit.
		 */
		void resize(size_t size) { if (size < operations.size()) operations.resize(size); }

	private:
		/**
		 * @brief Replaces the swap gate and three qubit gates with other operations
		 *
		 * Replaces the swap gate and three qubit gates with other operations to prepare the circuit for distributed computing.
		 * Replaces all three qubit gates (just ccnot and cswap will exist in the first phase) with several gates on less qubits.
		 * Also replaces swap gates with three cnots (in the first phase) if onlyThreeQubits is false (default value).
		 * @param onlyThreeQubits If true, only three qubit gates will be replaced.
		 */
		void ReplaceThreeQubitAndSwapGates(bool onlyThreeQubits = false)
		{
			// just replace all three qubit gates(just ccnot and cswap will exist in the first phase) with several gates on less qubits
			// also replace swap gates with three cnots (in the first phase)
			// the controlled ones must be replaced as well

			// TODO: if composite operations will be implemented, those need to be optimized as well

			std::vector<std::shared_ptr<IOperation<Time>>> newops;
			newops.reserve(operations.size());

			for (std::shared_ptr<IOperation<Time>> op : operations)
			{
				if (op->GetType() == OperationType::kGate)
				{
					std::shared_ptr<IQuantumGate<Time>> gate = std::static_pointer_cast<IQuantumGate<Time>>(op);

					if (NeedsConversion(gate, onlyThreeQubits))
					{
						std::vector<std::shared_ptr<IGateOperation<Time>>> newgates = ConvertGate(gate, onlyThreeQubits);
						newops.insert(newops.end(), newgates.begin(), newgates.end());
					}
					else newops.push_back(op);
				}
				else if (op->GetType() == OperationType::kConditionalGate)
				{
					std::shared_ptr<ConditionalGate<Time>> condgate = std::static_pointer_cast<ConditionalGate<Time>>(op);
					std::shared_ptr<IQuantumGate<Time>> gate = std::static_pointer_cast<IQuantumGate<Time>>(condgate->GetOperation());

					if (NeedsConversion(gate, onlyThreeQubits))
					{
						std::vector<std::shared_ptr<IGateOperation<Time>>> newgates = ConvertGate(gate, onlyThreeQubits);
						std::shared_ptr<ICondition> cond = condgate->GetCondition();

						for (auto gate : newgates)
							newops.push_back(std::make_shared<ConditionalGate<Time>>(gate, cond));
					}
					else newops.push_back(op);
				}
				else newops.push_back(op);
			}

			operations.swap(newops);
		}

		/**
		 * @brief Checks if a gate needs to be converted before distribution.
		 *
		 * Checks if a gate needs to be converted before distribution.
		 * @param gate The gate to check.
		 * @param onlyThreeQubits If true, returns true only for three qubit gates.
		 * @return True if the gate needs to be converted, false otherwise.
		 * @sa IQuantumGate
		 */
		static bool NeedsConversion(const std::shared_ptr<IQuantumGate<Time>>& gate, bool onlyThreeQubits = false)
		{
			const bool hasThreeQubits = gate->GetNumQubits() == 3;
			if (onlyThreeQubits) return hasThreeQubits;

			return hasThreeQubits || gate->GetGateType() == QuantumGateType::kSwapGateType;
		}

		/**
		 * @brief Converts a gate for distributed computing.
		 *
		 * Converts a gate for distributed computing.
		 * Replaces the gate with several gates on less qubits or in the case of swap gates, with three cnots.
		 * @param gate The gate to convert.
		 * @param onlyThreeQubits If true, only three qubit gates will be replaced.
		 * @return The converted gates.
		 * @sa IQuantumGate
		 * @sa IGateOperation
		 */
		static std::vector<std::shared_ptr<IGateOperation<Time>>> ConvertGate(std::shared_ptr<IQuantumGate<Time>>& gate, bool onlyThreeQubits = false)
		{
			// TODO: if delays are used, how to transfer delays from the converted gate to the resulting gates?
			std::vector<std::shared_ptr<IGateOperation<Time>>> newops;

			if (gate->GetNumQubits() == 3)
			{
				// must be converted no matter what
				if (gate->GetGateType() == QuantumGateType::kCCXGateType)
				{
					const size_t q1 = gate->GetQubit(0); // control 1
					const size_t q2 = gate->GetQubit(1); // control 2
					const size_t q3 = gate->GetQubit(2); // target

					// Sleator-Weinfurter decomposition
					newops.push_back(std::make_shared<CSxGate<Time>>(q2, q3));
					newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
					newops.push_back(std::make_shared<CSxDagGate<Time>>(q2, q3));
					newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
					newops.push_back(std::make_shared<CSxGate<Time>>(q1, q3));
				}
				else if (gate->GetGateType() == QuantumGateType::kCSwapGateType)
				{
					const size_t q1 = gate->GetQubit(0); // control 1
					const size_t q2 = gate->GetQubit(1); // control 2
					const size_t q3 = gate->GetQubit(2); // target

					// TODO: find a better decomposition
					// this one I've got with the qiskit transpiler
					newops.push_back(std::make_shared<CXGate<Time>>(q3, q2));

					newops.push_back(std::make_shared<CSxGate<Time>>(q2, q3));
					newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
					newops.push_back(std::make_shared<PhaseGate<Time>>(q3, M_PI));

					newops.push_back(std::make_shared<PhaseGate<Time>>(q2, -M_PI_2));

					newops.push_back(std::make_shared<CSxGate<Time>>(q2, q3));
					newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
					newops.push_back(std::make_shared<PhaseGate<Time>>(q3, M_PI));

					newops.push_back(std::make_shared<CSxGate<Time>>(q1, q3));

					newops.push_back(std::make_shared<CXGate<Time>>(q3, q2));
				}
				else newops.push_back(gate);
			}
			else if (!onlyThreeQubits && gate->GetGateType() == QuantumGateType::kSwapGateType)
			{
				// must be converted no matter what
				const size_t q1 = gate->GetQubit(0);
				const size_t q2 = gate->GetQubit(1);

				// for now replace it with three cnots, but maybe later make it configurable
				// there are other possibilities, for example three cy gates
				newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
				newops.push_back(std::make_shared<CXGate<Time>>(q2, q1));
				newops.push_back(std::make_shared<CXGate<Time>>(q1, q2));
			}
			else newops.push_back(gate);

			return newops;
		}

		OperationsVector operations; /**< The operations in the circuit. */
	};


	/**
	 * @class ComparableCircuit
	 * @brief Circuit class for holding the sequence of operations that can be compared with another circuit.
	 *
	 * Contains a sequence of operations that can be executed, supplying a function that allows executing them in a simulator.
	 * Allows adding operations and converting them to prepare the circuit for distributed computing.
	 * @tparam Time The time type used for operation timing.
	 * @sa Circuit
	 * @sa IOperation
	 * @sa ISimulator
	 */
	template<typename Time = Types::time_type> class ComparableCircuit : public Circuit<Time> {
	public:
		using BaseClass = Circuit<Time>; /**< The base class type. */
		using Operation = IOperation<Time>; /**< The operation type. */
		using OperationPtr = std::shared_ptr<Operation>; /**< The shared pointer to the operation type. */
		using OperationsVector = std::vector<OperationPtr>; /**< The vector of operations. */

		/**
		 * @brief Construct a new ComparableCircuit object.
		 *
		 * Constructs a new ComparableCircuit object with the given operations.
		 * @param ops The operations to add to the circuit.
		 * @sa IOperation
		 */
		ComparableCircuit(const OperationsVector& ops = {})
			: BaseClass(ops)
		{
		}

		/**
		 * @brief Construct a new ComparableCircuit object.
		 *
		 * Constructs a new ComparableCircuit object with the given Circuit.
		 * @param circuit The circuit from where to add the operations to the comparable circuit.
		 * @sa Circuit
		 */
		ComparableCircuit(const BaseClass& circ)
			: BaseClass(circ.GetOperations())
		{
		}

		/**
		 * @brief Assignment operator
		 *
		 * Assigns the operations from the given circuit to this circuit.
		 * @param circ The circuit from where to add the operations to this circuit.
		 * @return A reference to this circuit.
		 */
		ComparableCircuit& operator=(const BaseClass& circ)
		{
			BaseClass::SetOperations(circ.GetOperations());

			return *this;
		}

		/**
		 * @brief Comparison operator
		 *
		 * Compares two circuits for equality.
		 * @param rhs The circuit to compare to.
		 */
		bool operator==(const BaseClass& rhs) const
		{
			if (BaseClass::GetOperations().size() != rhs.GetOperations().size())
				return false;

			for (size_t i = 0; i < BaseClass::GetOperations().size(); ++i)
			{
				if (BaseClass::GetOperations()[i]->GetType() != rhs.GetOperations()[i]->GetType())
					return false;

				switch (BaseClass::GetOperations()[i]->GetType())
				{
				case OperationType::kGate:
					if (std::static_pointer_cast<IQuantumGate<Time>>(BaseClass::GetOperations()[i])->GetGateType() != std::static_pointer_cast<IQuantumGate<Time>>(rhs.GetOperations()[i])->GetGateType() ||
						BaseClass::GetOperations()[i]->AffectedBits() != rhs.GetOperations()[i]->AffectedBits())
						return false;
					if (approximateParamsCheck)
					{
						const auto params1 = std::static_pointer_cast<IQuantumGate<Time>>(BaseClass::GetOperations()[i])->GetParams();
						const auto params2 = std::static_pointer_cast<IQuantumGate<Time>>(rhs.GetOperations()[i])->GetParams();
						if (params1.size() != params2.size())
							return false;

						for (size_t j = 0; j < params1.size(); ++j)
							if (std::abs(params1[j] - params2[j]) > paramsEpsilon)
								return false;
					}
					else if (std::static_pointer_cast<IQuantumGate<Time>>(BaseClass::GetOperations()[i])->GetParams() != std::static_pointer_cast<IQuantumGate<Time>>(rhs.GetOperations()[i])->GetParams())
						return false;
					break;
				case OperationType::kMeasurement:
					if (BaseClass::GetOperations()[i]->AffectedQubits() != rhs.GetOperations()[i]->AffectedQubits() ||
						BaseClass::GetOperations()[i]->AffectedBits() != rhs.GetOperations()[i]->AffectedBits())
						return false;
					break;
				case OperationType::kRandomGen:
					if (BaseClass::GetOperations()[i]->AffectedBits() != rhs.GetOperations()[i]->AffectedBits())
						return false;
					break;
				case OperationType::kConditionalGate:
				case OperationType::kConditionalMeasurement:
				case OperationType::kConditionalRandomGen:
				{
					// first, check the conditions
					const auto leftCondition = std::static_pointer_cast<IConditionalOperation<Time>>(BaseClass::GetOperations()[i])->GetCondition();
					const auto rightCondition = std::static_pointer_cast<IConditionalOperation<Time>>(rhs.GetOperations()[i])->GetCondition();
					if (leftCondition->GetBitsIndices() != rightCondition->GetBitsIndices())
						return false;

					const auto leftEqCondition = std::static_pointer_cast<EqualCondition>(leftCondition);
					const auto rightEqCondition = std::static_pointer_cast<EqualCondition>(rightCondition);
					if (!leftEqCondition || !rightEqCondition)
						return false;

					if (leftEqCondition->GetAllBits() != rightEqCondition->GetAllBits())
						return false;

					// now check the operations
					const auto leftOp = std::static_pointer_cast<IConditionalOperation<Time>>(BaseClass::GetOperations()[i])->GetOperation();
					const auto rightOp = std::static_pointer_cast<IConditionalOperation<Time>>(rhs.GetOperations()[i])->GetOperation();

					ComparableCircuit<Time> leftCircuit;
					BaseClass rightCircuit;
					leftCircuit.SetApproximateParamsCheck(approximateParamsCheck);
					leftCircuit.AddOperation(leftOp);
					rightCircuit.AddOperation(rightOp);

					if (leftCircuit != rightCircuit)
						return false;
				}
				break;
				case OperationType::kReset:
					if (BaseClass::GetOperations()[i]->AffectedQubits() != rhs.GetOperations()[i]->AffectedQubits() ||
						std::static_pointer_cast<Reset<Time>>(BaseClass::GetOperations()[i])->GetResetTargets() != std::static_pointer_cast<Reset<Time>>(rhs.GetOperations()[i])->GetResetTargets())
						return false;
					break;
				case OperationType::kNoOp:
					break;
				default:
					return false;
				}

				if (BaseClass::GetOperations()[i]->GetDelay() != rhs.GetOperations()[i]->GetDelay())
					return false;
			}

			return true;
		}

		/**
		 * @brief Comparison operator
		 *
		 * Compares two circuits for inequality.
		 * @param rhs The circuit to compare to.
		 */
		bool operator!=(const BaseClass& rhs) const
		{
			return !(*this == rhs);
		}

		/**
		 * @brief Sets whether to check approximate equality of gate parameters.
		 *
		 * Sets whether to check approximate equality of gate parameters.
		 * @param check Whether to check approximate equality of gate parameters.
		 */
		void SetApproximateParamsCheck(bool check)
		{
			approximateParamsCheck = check;
		}

		/**
		 * @brief Gets whether to check approximate equality of gate parameters.
		 *
		 * Gets whether to check approximate equality of gate parameters.
		 * @return Whether to check approximate equality of gate parameters.
		 */
		bool GetApproximateParamsCheck() const
		{
			return approximateParamsCheck;
		}

		/**
		 * @brief Sets the epsilon used for checking approximate equality of gate parameters.
		 *
		 * Sets the epsilon used for checking approximate equality of gate parameters.
		 * @param eps The epsilon used for checking approximate equality of gate parameters.
		 */
		void SetParamsEpsilon(double eps)
		{
			paramsEpsilon = eps;
		}

		/**
		 * @brief Gets the epsilon used for checking approximate equality of gate parameters.
		 *
		 * Gets the epsilon used for checking approximate equality of gate parameters.
		 * @return The epsilon used for checking approximate equality of gate parameters.
		 */
		double GetParamsEpsilon() const
		{
			return paramsEpsilon;
		}

	private:
		bool approximateParamsCheck = false; /**< Whether to check approximate equality of gate parameters. */
		double paramsEpsilon = 1e-8; /**< The epsilon used for checking approximate equality of gate parameters. */
	};


}

#endif // !_CIRCUIT_H_
