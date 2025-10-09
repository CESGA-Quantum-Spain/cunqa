/**
 * @file Remapper.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Remapper class for converting a circuit to a distributed one.
 *
 * It's an abstract class, the actual implementation is in the derived classes, they must implement the Remap function.
 * It contains common functionality for the derived classes, specifically, splitting composite operations into ones that act on a single qubit/classical bit.
 */

#pragma once

#ifndef _REMAPPER_H_
#define _REMAPPER_H_

#include "../Circuit/Circuit.h"
#include "../Circuit/Operations.h"
#include "../Circuit/Reset.h"
#include "../Network/Network.h"


namespace Distribution {

	/**
	 * @enum RemapperType
	 * @brief The type of remapper to use.
	 */
	enum class RemapperType {
		kLayersRemapper,       /**< Remapper that uses intermediate layers for conversion */
		kGreedyDirectRemapper  /**< Remapper that uses a greedy algorithm for conversion, avoiding conversion to intermediate layers */
	};

	/**
	 * @class IRemapper
	 * @brief Remapper abstract class.
	 * 
	 * Remapper abstract class. Derived classes must implement the Remap function.
	 * @tparam Time The type of the time. Typically either double or unsigned long long int. Default is double. 
	 */
	template<typename Time = Types::time_type> class IRemapper : public std::enable_shared_from_this<IRemapper<Time>>
	{
	public:
		/**
		 * @brief Default virtual destructor.
		 *
		 * Default destructor, virtual because it's an abstract class that must be derived from.
		 */
		virtual ~IRemapper() = default;

		/**
		 * @brief Remap the circuit.
		 *
		 * Remaps the circuit to a distributed one.
		 * @param network The network to use for remapping.
		 * @param circuit The circuit to remap.
		 * @return A shared pointer to the remapped circuit.
		 * @sa Network::INetwork
		 * @sa Circuits::Circuit
		 */
		virtual std::shared_ptr<Circuits::Circuit<Time>> Remap(const std::shared_ptr<Network::INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& circuit) = 0;

		/**
		 * @brief Get the number of operations for distribution.
		 *
		 * Returns the number of operations for distribution.
		 * This is the total number of operations that are added to distribute a gate (or a group of gates that share the control qubit and have the target on the same host) 
		 * using the qubits for entanglement between the hosts.
		 * @return The number of operations for distribution.
		 */
		virtual unsigned int GetNumberOfOperationsForDistribution() const = 0;

		/**
		 * @brief Get the number of distribution circuits for the last remapped circuit
		 *
		 * Returns the number of distribution circuits for the last remapped circuit.
		 * @return The number of distribution circuits.
		 */
		virtual unsigned int GetNumberOfDistributions() const = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IRemapper<Time>> getptr()
		{
			return std::enable_shared_from_this<IRemapper<Time>>::shared_from_this();
		}

		/**
		* @brief Returns the type of the remapper.
		*
		* Returns the type of the remapper.
		* @return The type of the remapper
		* @sa RemapperType
		*/
		virtual RemapperType GetType() const = 0;

		/**
		 * @brief Checks if an operation is non local.
		 *
		 * An operation is non local if it needs entanglement for distribution, affecting qubits on different hosts.
		 *
		 * @param op The operation to check.
		 * @param network The network to use for the check.
		 * @return True if the operation is non local, false otherwise.
		 * @sa Network::INetwork
		 * @sa Circuits::IOperation
		 */
		static bool IsNonLocalOperation(const std::shared_ptr<Circuits::IOperation<Time>>& op, const std::shared_ptr<Network::INetwork<Time>>& network)
		{
			if (op->NeedsEntanglementForDistribution())
			{
				auto qubits = op->AffectedQubits();

				if (qubits.size() >= 2) // there should be no 3 qubit gates at this point, though
				{
					const size_t ctrlQubit = qubits[0];
					const size_t targetQubit = qubits[1];

					if (!network->AreQubitsOnSameHost(ctrlQubit, targetQubit))
						return true;
				}
			}

			return false;
		}

		/**
		 * @brief Split composite operations.
		 *
		 * Splits some composite operations.
		 * There are composite operations - other than multiple qubits quantum gates - that act on multiple qubits/classical bits that might act on qubits that are not local.
		 * This function splits those operations into ones that act on a single qubit/classical bit.
		 * Operations that are split are: Measurement, RandomGen, ConditionalMeasurement, ConditionalRandomGen, Reset.
		 * For example measurements on several qubits are split into several measurements on a single qubit.
		 * @param network The network to use for remapping.
		 * @param distCirc The circuit to have its operations split if needed.
		 * @return A shared pointer to the new circuit with converted operations.
		 * @sa Network::INetwork
		 * @sa Circuits::Circuit
		 */
		virtual std::shared_ptr<Circuits::Circuit<Time>> SplitCompositeOperations(const std::shared_ptr<Network::INetwork<Time>>& network, const std::shared_ptr<Circuits::Circuit<Time>>& distCirc) const
		{
			// Measurement, RandomGen, ConditionalMeasurement, ConditionalRandomGen, Reset... might involve qubits/classical bits that are on different hosts - split them so they could be executed in any order independently
			// on top of that, conditional gates might need classical bits from different hosts, handle that as well
			// in the future such classical bits transfers might be simulated using 'resources' involving queues or something like that to simulate the classical network

			std::shared_ptr<Circuits::Circuit<Time>> newDistributedCircuit = std::make_shared<Circuits::Circuit<Time>>();

			for (const auto& op : distCirc->GetOperations())
			{
				// the measurement, conditional measurement and reset are split based on the qubits they operate on
				// the others based on the classical bits involved 

				switch (op->GetType())
				{
				case Circuits::OperationType::kMeasurement:
				{
					std::unordered_map<size_t, std::vector<std::pair<Types::qubit_t, size_t>>> bits;
					const auto qbits = op->AffectedQubits();
					const auto cbits = op->AffectedBits();

					for (size_t q = 0; q < qbits.size(); ++q)
					{
						const size_t host = network->GetHostIdForAnyQubit(qbits[q]);
						bits[host].push_back({ qbits[q], cbits[q] });
					}

					for (const auto& hostQubits : bits)
						newDistributedCircuit->AddOperation(std::make_shared<Circuits::MeasurementOperation<Time>>(hostQubits.second));
				}
				break;
				case Circuits::OperationType::kConditionalMeasurement:
				{
					std::unordered_map<size_t, std::vector<std::pair<Types::qubit_t, size_t>>> bits;
					auto condOp = std::static_pointer_cast<Circuits::ConditionalMeasurement<Time>>(op);

					const auto qbits = condOp->GetOperation()->AffectedQubits();
					const auto cbits = condOp->GetOperation()->AffectedBits();

					for (size_t q = 0; q < qbits.size(); ++q)
					{
						const size_t host = network->GetHostIdForAnyQubit(qbits[q]);
						bits[host].push_back({ qbits[q], cbits[q] });
					}

					for (const auto& hostQubits : bits)
					{
						auto measOp = std::make_shared<Circuits::MeasurementOperation<Time>>(hostQubits.second);
						newDistributedCircuit->AddOperation(std::make_shared<Circuits::ConditionalMeasurement<Time>>(measOp, condOp->GetCondition())); // condition stays the same
					}
				}
				break;
				case Circuits::OperationType::kReset:
				{
					std::unordered_map<size_t, Types::qubits_vector> bits;
					const auto qbits = op->AffectedQubits();

					for (const auto q : qbits)
					{
						const size_t host = network->GetHostIdForAnyQubit(q);
						bits[host].emplace_back(q);
					}

					for (const auto& hostQubits : bits)
						newDistributedCircuit->AddOperation(std::make_shared<Circuits::Reset<Time>>(hostQubits.second));
				}
				break;
				case Circuits::OperationType::kRandomGen:
				{
					std::unordered_map<size_t, std::vector<size_t>> bits;
					const std::vector<size_t> cbits = op->AffectedBits();

					for (const size_t c : cbits)
					{
						const size_t host = network->GetHostIdForClassicalBit(c);
						bits[host].emplace_back(c);
					}

					for (const auto& hostQubits : bits)
						newDistributedCircuit->AddOperation(std::make_shared<Circuits::Random<Time>>(hostQubits.second));
				}
				break;
				case Circuits::OperationType::kConditionalRandomGen:
				{
					std::unordered_map<size_t, std::vector<size_t>> bits;
					// split by operation (as in random gen), not by condition, that should stay whole and be dealt with accordingly if classical network simulation details are to be done
					auto condOp = std::static_pointer_cast<Circuits::ConditionalRandomGen<Time>>(op);
					const std::vector<size_t> cbits = condOp->GetOperation()->AffectedBits();

					for (const size_t c : cbits)
					{
						const size_t host = network->GetHostIdForClassicalBit(c);
						bits[host].emplace_back(c);
					}

					for (const auto& hostQubits : bits)
					{
						auto randomOp = std::make_shared<Circuits::Random<Time>>(hostQubits.second);
						newDistributedCircuit->AddOperation(std::make_shared<Circuits::ConditionalRandomGen<Time>>(randomOp, condOp->GetCondition())); // condition stays the same
					}
				}
				break;
				default:
					newDistributedCircuit->AddOperation(op);
					break;
				}
			}

			return newDistributedCircuit;
		}
	};

}


#endif // !_REMAPPER_H_

