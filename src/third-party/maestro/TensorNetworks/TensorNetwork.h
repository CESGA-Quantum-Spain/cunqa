/**
 * @file TensorNetwork.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Tensor Network class.
 *
 * A class containing the tensor network, that is, a graph of tensors as nodes and contractions as edges.
 */

#pragma once

#ifndef __TENSOR_NETWORK_H_
#define __TENSOR_NETWORK_H_ 1

#include <Eigen/Eigen>
#include <vector>
#include <unordered_set>

#include "TensorNode.h"
#include "TensorContractor.h"

namespace TensorNetworks {

	class TensorNetwork {
	public:
		using Index = Eigen::Index;

		TensorNetwork() = delete;

		/**
		 * @brief Constructor.
		 *
		 * @param numQubits The number of qubits in the network.
		 */
		TensorNetwork(size_t numQubits)
			: rng(std::random_device{}()), uniformZeroOne(0, 1)
		{
			lastTensors.resize(numQubits);
			lastTensorsSuper.resize(numQubits);
			lastTensorIndices.resize(numQubits);
			lastTensorIndicesSuper.resize(numQubits);

			qubitsMap.resize(numQubits);
		
			Clean(numQubits);
		}

		void Clear()
		{
			// cannot do this if the one qubit gates are contracted into qubit tensors!!!!!!
			/*
			tensors.resize(2 * lastTensors.size()); // keep only the qubit tensors (both normal and super)

			qubitsGroups.clear();

			SetQubitsTensorsClear(lastTensors.size());
			*/
			tensors.clear();
			qubitsGroups.clear();
			Clean(GetNumQubits());
		}

		void AddGate(const QC::Gates::QuantumGateWithOp<TensorNode::MatrixClass>& gate, Types::qubit_t q1, Types::qubit_t q2 = 0)
		{
			const auto gateQubitsNumber = gate.getQubitsNumber();
			if (gateQubitsNumber > 1) 
				AddTwoQubitsGate(gate, q1, q2);
			else
				AddOneQubitGate(gate, q1);
		}

		double Probability(Types::qubit_t qubit, bool zero = true)
		{
			if (!contractor) return 0;
			contractor->SetMultithreading(enableMultithreading);

			// save the qubit positions, to be able to restore them back after the addition of the projector and the contraction
			const auto lastTensorIdOnQubit = lastTensors[qubit];
			const auto lastTensorIndexOnQubit = lastTensorIndices[qubit];

			// add the projection tensor to the network
			AddProjector(qubit, zero);

			Connect();

			// contract the network, save result to return
			const double result = contractor->Contract(*this, qubit);

			// restore back the network to the previous state (disconnect the connection to super tensors and remove the added projector)

			lastTensors[qubit] = lastTensorIdOnQubit;
			lastTensorIndices[qubit] = lastTensorIndexOnQubit;

			// get rid of the added projection tensor
			tensors.resize(tensors.size() - 1);

			// no need to actually disconnect the indices, as they are not used unless they are connected again
			// when adding a new gate/projection for measurement, they are reconnected anyway, the same goes for doing another contraction

			// return the result
			return result;
		}

		/**
		 * @brief Returns the expected value of a Pauli string.
		 *
		 * Use it to obtain the expected value of a Pauli string.
		 * The Pauli string is a string of characters representing the Pauli operators, e.g. "XIZY".
		 * The length of the string should be less or equal to the number of qubits (if it's less, it's completed with I).
		 *
		 * @param pauliString The Pauli string to obtain the expected value for.
		 * @return The expected value of the specified Pauli string.
		 */
		double ExpectationValue(const std::string& pauliString)
		{
			if (pauliString.empty()) return 1.;
			else if (!contractor) return 0.;

			auto savedTensorsNrLocal = tensors.size();
			auto saveLastTensorsLocal = lastTensors;
			auto saveLastTensorIndicesLocal = lastTensorIndices;

			// add the gates from the Pauli string
			const QC::Gates::PauliXGate<TensorNode::MatrixClass> XGate;
			const QC::Gates::PauliYGate<TensorNode::MatrixClass> YGate;
			const QC::Gates::PauliZGate<TensorNode::MatrixClass> ZGate;

			std::unordered_set<Types::qubit_t> usedQubits;

			for (Types::qubit_t q = 0; q < pauliString.size(); ++q)
			{
				const char op = toupper(pauliString[q]);
				switch (op)
				{
				case 'X':
					AddOneQubitExpectationValueOp(XGate, q);
					usedQubits.insert(q);
					break;
				case 'Y':
					AddOneQubitExpectationValueOp(YGate, q);
					usedQubits.insert(q);
					break;
				case 'Z':
					AddOneQubitExpectationValueOp(ZGate, q);
					usedQubits.insert(q);
					break;
				case 'I':
					[[fallthrough]];
				default:
					break;
				}
			}

			if (usedQubits.empty()) return 1.;

			const double result = Contract(usedQubits);

			tensors.resize(savedTensorsNrLocal);
			lastTensors.swap(saveLastTensorsLocal);
			lastTensorIndices.swap(saveLastTensorIndicesLocal);

			Disconnect();

			return result;
		}


		double getBasisStateProbability(size_t outcome)
		{
			SaveStateMinimal();
	
			size_t mask = 1ULL;

			for (Types::qubit_t q = 0; q < GetNumQubits(); ++q)
			{
				const bool expected = (outcome & mask) == 0;

				AddProjector(q, expected);

				mask <<= 1;
			}

			const double prob = Contract();

			RestoreSavedStateMinimalDestructive();

			return prob;
		}

		bool Measure(Types::qubit_t qubit)
		{
			const double p0 = Probability(qubit, false);
			const double prob = uniformZeroOne(rng);
			if (prob < p0)
			{
				AddProjectorOp(qubit, false, p0);

				return true;
			}

			AddProjectorOp(qubit, true, 1. - p0);
			
			return false;
		}

		void AddProjector(Types::qubit_t qubit, bool zero = true)
		{
			// add a projector tensor for the qubit, either zero or one
			auto tensorNode = std::make_shared<TensorNode>();
			tensorNode->SetProjector(qubit, zero);

			const auto newTensorId = static_cast<Index>(tensors.size());
			tensorNode->SetId(newTensorId);

			// connect the tensor to the last tensors on the qubits

			// first, tensors from the network are connected to the new tensor
			// that is, their output indices are connected to the input indices of the new tensor
			// second, the new tensor is connected to the tensors from the network

			const auto tensorOnQubitId = lastTensors[qubit];
			const auto indexOnQubit = lastTensorIndices[qubit];

			// connect the last tensor on qubit with the projection tensor
			const auto& lastTensor = tensors[tensorOnQubitId];
			lastTensor->connections[indexOnQubit] = newTensorId;
			lastTensor->connectionsIndices[indexOnQubit] = 0;

			// also do the connection between the projection tensor back to the last tensor
			tensorNode->connections[0] = tensorOnQubitId;
			tensorNode->connectionsIndices[0] = indexOnQubit;

			// add the tensor to the network
			tensors.emplace_back(std::move(tensorNode));

			// and set the last tensor on the qubit to be the new tensor
			lastTensors[qubit] = newTensorId;
			lastTensorIndices[qubit] = 1; // the next index is 1, this is going to be the 'connect' one
		}

		void AddProjectorOp(Types::qubit_t qubit, bool zero = true, double prob = 1.)
		{
			TensorNode::MatrixClass projMat = TensorNode::MatrixClass::Zero(2, 2);

			const double renorm = 1. / sqrt(prob);
			if (zero)
				projMat(0, 0) = renorm;
			else
				projMat(1, 1) = renorm;

			const QC::Gates::SingleQubitGate<TensorNode::MatrixClass> projOp(projMat);

			AddGate(projOp, qubit);
		}

		size_t GetNumQubits() const
		{
			return lastTensors.size();
		}

		void SetContractor(const std::shared_ptr<ITensorContractor>& c)
		{
			contractor = c;
		}

		std::shared_ptr<ITensorContractor> GetContractor() const
		{
			return contractor;
		}

		const std::vector<std::shared_ptr<TensorNode>>& GetTensors() const
		{
			return tensors;
		}

		const std::unordered_set<Types::qubit_t>& GetQubitGroup(Types::qubit_t q) const
		{
			return qubitsGroups.at(qubitsMap[q]);
		}

		void SaveState()
		{
			saveTensors.resize(tensors.size());
			for (size_t i = 0; i < tensors.size(); ++i)
				saveTensors[i] = tensors[i]->CloneWithoutTensorCopy();

			saveQubitsMap = qubitsMap;
			saveQubitsGroups = qubitsGroups;

			SaveStateMinimal();
		}

		void SaveStateMinimal()
		{
			savedTensorsNr = tensors.size();
			saveLastTensors = lastTensors;
			saveLastTensorsSuper = lastTensorsSuper;
			saveLastTensorIndices = lastTensorIndices;
			saveLastTensorIndicesSuper = lastTensorIndicesSuper;
		}

		void RestoreState()
		{
			RestoreStateMinimal();

			tensors.resize(saveTensors.size());
			for (size_t i = 0; i < tensors.size(); ++i)
				tensors[i] = saveTensors[i]->CloneWithoutTensorCopy();

			qubitsMap = saveQubitsMap;
			qubitsGroups = saveQubitsGroups;
		}

		void RestoreStateMinimal()
		{
			tensors.resize(savedTensorsNr);
			lastTensors = saveLastTensors;
			lastTensorsSuper = saveLastTensorsSuper;
			lastTensorIndices = saveLastTensorIndices;
			lastTensorIndicesSuper = saveLastTensorIndicesSuper;
		}

		void RestoreSavedStateDestructive()
		{
			tensors.swap(saveTensors);

			qubitsMap.swap(saveQubitsMap);
			qubitsGroups.swap(saveQubitsGroups);

			RestoreSavedStateMinimalDestructive();

			ClearSavedState();
		}

		void RestoreSavedStateMinimalDestructive()
		{
			tensors.resize(savedTensorsNr);
			savedTensorsNr = 0;
			lastTensors.swap(saveLastTensors);
			lastTensorsSuper.swap(saveLastTensorsSuper);
			lastTensorIndices.swap(saveLastTensorIndices);
			lastTensorIndicesSuper.swap(saveLastTensorIndicesSuper);

			ClearSavedStateMinimal();
		}

		void ClearSavedState()
		{
			saveTensors.clear();

			saveQubitsMap.clear();
			saveQubitsGroups.clear();

			ClearSavedStateMinimal();
		}

		void ClearSavedStateMinimal()
		{
			saveLastTensors.clear();
			saveLastTensorsSuper.clear();

			saveLastTensorIndices.clear();
			saveLastTensorIndicesSuper.clear();
		}

		void Connect()
		{
			// connect all last tensors to the corresponding super tensors
			for (Types::qubit_t q = 0; q < GetNumQubits(); ++q)
			{
				const auto lastTensorId = lastTensors[q];
				const auto lastTensorSuperId = lastTensorsSuper[q];
				const auto& lastTensor = tensors[lastTensorId];
				const auto& lastTensorSuper = tensors[lastTensorSuperId];

				const auto tensorIndexOnQubit = lastTensorIndices[q];
				const auto tensorSuperIndexOnQubit = lastTensorIndicesSuper[q];

				// tensor connection to the super one
				lastTensor->connections[tensorIndexOnQubit] = lastTensorSuperId;
				lastTensor->connectionsIndices[tensorIndexOnQubit] = tensorSuperIndexOnQubit;

				// the super tensor connected to the left tensor
				lastTensorSuper->connections[tensorSuperIndexOnQubit] = lastTensorId;
				lastTensorSuper->connectionsIndices[tensorSuperIndexOnQubit] = tensorIndexOnQubit;
			}
		}

		void Disconnect()
		{
			for (Types::qubit_t q = 0; q < GetNumQubits(); ++q)
			{
				const auto lastTensorId = lastTensors[q];
				const auto lastTensorSuperId = lastTensorsSuper[q];
				const auto& lastTensor = tensors[lastTensorId];
				const auto& lastTensorSuper = tensors[lastTensorSuperId];

				const auto tensorIndexOnQubit = lastTensorIndices[q];
				const auto tensorSuperIndexOnQubit = lastTensorIndicesSuper[q];

				// tensor connection to the super one
				lastTensor->connections[tensorIndexOnQubit] = TensorNode::NotConnected;
				lastTensor->connectionsIndices[tensorIndexOnQubit] = TensorNode::NotConnected;

				// the super tensor connected to the left tensor
				lastTensorSuper->connections[tensorSuperIndexOnQubit] = TensorNode::NotConnected;
				lastTensorSuper->connectionsIndices[tensorSuperIndexOnQubit] = TensorNode::NotConnected;
			}
		}

		/**
		 * @brief Enable/disable multithreading.
		 *
		 * Enable/disable multithreading. Default is enabled.
		 *
		 * @param multithreading A flag to indicate if multithreading should be enabled.
		 */
		void SetMultithreading(bool multithreading = true)
		{
			enableMultithreading = multithreading;
		}

		/**
		 * @brief Get the multithreading flag.
		 *
		 * Returns the multithreading flag.
		 *
		 * @return The multithreading flag.
		 */
		bool GetMultithreading() const
		{
			return enableMultithreading;
		}

		std::unique_ptr<TensorNetwork> Clone() const
		{
			auto cloned = std::make_unique<TensorNetwork>(0);

			cloned->tensors = tensors; // all tensors in the network

			cloned->lastTensors = lastTensors; // the indices of the last tensors in the network
			cloned->lastTensorsSuper = lastTensorsSuper; // the indices of the last super tensors in the network

			cloned->lastTensorIndices = lastTensorIndices; // the indices of the last tensors in the network
			cloned->lastTensorIndicesSuper = lastTensorIndicesSuper; // the indices of the last super tensors in the network

			cloned->qubitsMap = qubitsMap; /**< A map between qubits (as identified from outside) and the qubits group ids */
			cloned->qubitsGroups = qubitsGroups; /**< A map between qubits group ids and the qubits in that group */


			cloned->savedTensorsNr = savedTensorsNr;
			cloned->saveTensors = saveTensors; // all tensors in the network

			cloned->saveLastTensors = saveLastTensors; // the indices of the last tensors in the network
			cloned->saveLastTensorsSuper = saveLastTensorsSuper; // the indices of the last super tensors in the network

			cloned->saveLastTensorIndices = saveLastTensorIndices; // the indices of the last tensors in the network
			cloned->saveLastTensorIndicesSuper = saveLastTensorIndicesSuper; // the indices of the last super tensors in the network

			cloned->saveQubitsMap = saveQubitsMap; /**< A map between qubits (as identified from outside) and the qubits group ids */
			cloned->saveQubitsGroups = saveQubitsGroups; /**< A map between qubits group ids and the qubits in that group */

			if (contractor) cloned->contractor = contractor->Clone();

			return cloned;
		}

	private:
		void AddOneQubitExpectationValueOp(const QC::Gates::QuantumGateWithOp<TensorNode::MatrixClass>& gate, Types::qubit_t q)
		{
			auto tensorNode = std::make_shared<TensorNode>();
			tensorNode->SetGate(gate, q);

			auto lastTensorId = lastTensors[q];
			const auto& lastTensor = tensors[lastTensorId];

			// can be either 0 if the last tensor is for a single qubit (the first tensor)
			// or 1 if it's from a one qubit gate tensor
			// or either 2 or 3 if it corresponds to a two qubit gate tensor
			// for 0 or 1 do nothing to qubits or connections in the altered node, nothing changes
			// for 3 also nothing should be done, as the qubit is the second one in the gate and contraction doesn't change anything
			// but if it's 2, the order changes
			auto lastTensorIndexForQubit = lastTensorIndices[q];

			auto tensorId = static_cast<Index>(tensors.size());
			tensorNode->SetId(tensorId);

			// connect the tensor to the last tensors on the qubits

			// first, tensors from the network are connected to the new tensor
			// that is, their output indices are connected to the input indices of the new tensor
			// second, the new tensor is connected to the tensors from the network

			lastTensor->connections[lastTensorIndexForQubit] = tensorId;
			lastTensor->connectionsIndices[lastTensorIndexForQubit] = 0; // connect it to the zero index of the new tensor

			tensorNode->connections[0] = lastTensorId;
			tensorNode->connectionsIndices[0] = lastTensorIndexForQubit;

			tensors.emplace_back(std::move(tensorNode));

			// now set the last tensors and indices
			lastTensors[q] = tensorId;

			lastTensorIndices[q] = 1;
		}

		void AddOneQubitGate(const QC::Gates::QuantumGateWithOp<TensorNode::MatrixClass>& gate, Types::qubit_t q, bool contractWithTwoQubitTensors = false, bool contract = true)
		{
			// instead of adding it to the network, simply contract it with the previous tensor on the qubit
			auto tensorNode = std::make_shared<TensorNode>();
			tensorNode->SetGate(gate, q);

			auto lastTensorId = lastTensors[q];
			const auto& lastTensor = tensors[lastTensorId];

			// can be either 0 if the last tensor is for a single qubit (the first tensor)
			// or 1 if it's from a one qubit gate tensor
			// or either 2 or 3 if it corresponds to a two qubit gate tensor
			// for 0 or 1 do nothing to qubits or connections in the altered node, nothing changes
			// for 3 also nothing should be done, as the qubit is the second one in the gate and contraction doesn't change anything
			// but if it's 2, the order changes
			auto lastTensorIndexForQubit = lastTensorIndices[q];

			// if contraction with two qubit tensors is desired, if it's on the last index it's easy
			// if not, it's more complex because contraction changes the order of indices
			// it's not easy to deal with that as the other qubit might have already another tensor connected/over it
			// maybe using Shuffle is easier than changing connections and indices and qubits and so on...
			if (contract && (contractWithTwoQubitTensors || lastTensor->qubits.size() <= 2))
			{
				// previous tensor is a one qubit tensor, contract it with the new one
				
				// TODO: Should I also contract them if the previous tensor is a two qubit tensor?
				// probably yes, but probably only when the 'last tensor' is a one qubit one... and it's hit with a two qubit tensor
				// because contracting together the one qubit tensors is quite fast
				
				// tensors, lastTensors, lastTensorsSuper do not need change (except the new tensor inside the tensorNode)
				// in the node, the connections do not need to change, index 0 and 1 remain the same, the input ones and do not change order (so also the connected nodes are not affected)

				lastTensor->tensor = std::make_shared<Utils::Tensor<>>(std::move(lastTensor->tensor->Contract(*(tensorNode->tensor), lastTensorIndexForQubit, 0, enableMultithreading)));

				// TODO: Instead of Shuffle try by swapping qubits and indices... but also take care of connections if on the other qubit there is already a gate added
				static const std::vector<size_t> indices{ 0,1,3,2 };
				if (lastTensorIndexForQubit == 2)
				{
					// this works, but it's slow, so let's try something else
					//lastTensor->tensor = std::make_shared<Utils::Tensor<>>(std::move(lastTensor->tensor->Shuffle(indices)));
					const auto otherQubit = lastTensor->qubits[3];
					const auto otherTensorId = lastTensors[otherQubit];

					std::swap(lastTensor->qubits[2], lastTensor->qubits[3]);
					std::swap(lastTensor->connections[2], lastTensor->connections[3]);
					std::swap(lastTensor->connectionsIndices[2], lastTensor->connectionsIndices[3]);

					lastTensorIndices[q] = 3;
					
					if (otherTensorId == lastTensorId)
						lastTensorIndices[otherQubit] = 2;
					else
					{
						// another tensor went on top of the other qubit/leg, so update connections
						const auto& otherTensor = tensors[lastTensor->connections[2]]; // 3 was swapped with 2
						
						const auto otherIndex = lastTensor->connectionsIndices[2]; // 3 was swapped with 2
						otherTensor->connectionsIndices[otherIndex] = 2; // 3 was swapped with 2
					}
				}

				tensorNode->SetSuper();

				lastTensorId = lastTensorsSuper[q];
				const auto& lastTensorSuper = tensors[lastTensorId];
				lastTensorIndexForQubit = lastTensorIndicesSuper[q];

				lastTensorSuper->tensor = std::make_shared<Utils::Tensor<>>(std::move(lastTensorSuper->tensor->Contract(*(tensorNode->tensor), lastTensorIndexForQubit, 0, enableMultithreading)));

				if (lastTensorIndexForQubit == 2)
				{
					//lastTensorSuper->tensor = std::make_shared<Utils::Tensor<>>(std::move(lastTensorSuper->tensor->Shuffle(indices)));
					const auto otherQubit = lastTensorSuper->qubits[3];
					const auto otherTensorId = lastTensorsSuper[otherQubit];

					std::swap(lastTensorSuper->qubits[2], lastTensorSuper->qubits[3]);
					std::swap(lastTensorSuper->connections[2], lastTensorSuper->connections[3]);
					std::swap(lastTensorSuper->connectionsIndices[2], lastTensorSuper->connectionsIndices[3]);

					lastTensorIndicesSuper[q] = 3;
					
					if (otherTensorId == lastTensorId)
						lastTensorIndicesSuper[otherQubit] = 2;
					else
					{
						// another tensor went on top of the other qubit/leg, so update connections
						const auto& otherTensor = tensors[lastTensorSuper->connections[2]]; // 3 was swapped with 2

						const auto otherIndex = lastTensorSuper->connectionsIndices[2]; // 3 was swapped with 2
						otherTensor->connectionsIndices[otherIndex] = 2; // 3 was swapped with 2
					}
				}
			}
			else
			{
				auto tensorId = static_cast<Index>(tensors.size());
				tensorNode->SetId(tensorId);

				// connect the tensor to the last tensors on the qubits

				// first, tensors from the network are connected to the new tensor
				// that is, their output indices are connected to the input indices of the new tensor
				// second, the new tensor is connected to the tensors from the network

				lastTensor->connections[lastTensorIndexForQubit] = tensorId;
				lastTensor->connectionsIndices[lastTensorIndexForQubit] = 0; // connect it to the zero index of the new tensor

				tensorNode->connections[0] = lastTensorId;
				tensorNode->connectionsIndices[0] = lastTensorIndexForQubit;

				tensors.emplace_back(std::move(tensorNode));

				// now set the last tensors and indices
				lastTensors[q] = tensorId;

				lastTensorIndices[q] = 1;

				// add the 'super' now

				tensorNode = std::make_shared<TensorNode>();

				tensorNode->SetGate(gate, q);
				tensorNode->SetSuper();

				tensorId = static_cast<Index>(tensors.size());
				tensorNode->SetId(tensorId);

				// connect the tensor to the last tensors on the qubits

				// first, tensors from the network are connected to the new tensor
				// that is, their output indices are connected to the input indices of the new tensor
				// second, the new tensor is connected to the tensors from the network

				lastTensorId = lastTensorsSuper[q];
				const auto& lastTensorSuper = tensors[lastTensorId];
				lastTensorIndexForQubit = lastTensorIndicesSuper[q];
				lastTensorSuper->connections[lastTensorIndexForQubit] = tensorId;
				lastTensorSuper->connectionsIndices[lastTensorIndexForQubit] = 0;

				tensorNode->connections[0] = lastTensorId;
				tensorNode->connectionsIndices[0] = lastTensorIndexForQubit;

				tensors.emplace_back(std::move(tensorNode));

				// now set the last tensors and indices
				lastTensorsSuper[q] = tensorId;

				lastTensorIndicesSuper[q] = 1;
			}
		}

		void AddTwoQubitsGate(const QC::Gates::QuantumGateWithOp<TensorNode::MatrixClass>& gate, Types::qubit_t q1, Types::qubit_t q2)
		{
			const size_t q1group = qubitsMap[q1];
			const size_t q2group = qubitsMap[q2];

			if (q1group != q2group)
			{
				// merge groups - move all qubits from the second group to the first one
				for (const auto qg : qubitsGroups[q2group])
				{
					qubitsMap[qg] = q1group;
					qubitsGroups[q1group].insert(qg);
				}
				qubitsGroups.erase(q2group); // remove the second group
			}

			// TODO: if first two qubits gate on the 1-qubit tensors of the qubits, contract them with this node?????
			// or even if only one of them is on a 1-qubit tensor, contract it with this one????

			auto tensorNode = std::make_shared<TensorNode>();
			tensorNode->SetGate(gate, q1, q2);

			auto tensorId = static_cast<Index>(tensors.size());
			tensorNode->SetId(tensorId);

			// connect the tensor to the last tensors on the qubits

			// first, tensors from the network are connected to the new tensor
			// that is, their output indices are connected to the input indices of the new tensor
			// second, the new tensor is connected to the tensors from the network

			auto lastTensorId = lastTensors[q1];

			const auto& lastTensor = tensors[lastTensorId];
			auto lastTensorIndexForQubit = lastTensorIndices[q1];
			lastTensor->connections[lastTensorIndexForQubit] = tensorId;
			lastTensor->connectionsIndices[lastTensorIndexForQubit] = 0; // connect it to the zero index of the new tensor

			tensorNode->connections[0] = lastTensorId;
			tensorNode->connectionsIndices[0] = lastTensorIndexForQubit;


			lastTensorId = lastTensors[q2];
			const auto& lastTensor2 = tensors[lastTensorId];
			lastTensorIndexForQubit = lastTensorIndices[q2];
			lastTensor2->connections[lastTensorIndexForQubit] = tensorId;
			lastTensor2->connectionsIndices[lastTensorIndexForQubit] = 1; // connect it to the first index of the new tensor

			tensorNode->connections[1] = lastTensorId;
			tensorNode->connectionsIndices[1] = lastTensorIndices[q2];

			tensors.emplace_back(std::move(tensorNode));

			// now set the last tensors and indices
			lastTensors[q1] = tensorId;
			lastTensors[q2] = tensorId;

			lastTensorIndices[q1] = 2;
			lastTensorIndices[q2] = 3;

			// add the 'super' now

			tensorNode = std::make_shared<TensorNode>();

			tensorNode->SetGate(gate, q1, q2);
			tensorNode->SetSuper();

			tensorId = static_cast<Index>(tensors.size());
			tensorNode->SetId(tensorId);

			// connect the tensor to the last tensors on the qubits

			// first, tensors from the network are connected to the new tensor
			// that is, their output indices are connected to the input indices of the new tensor
			// second, the new tensor is connected to the tensors from the network

			lastTensorId = lastTensorsSuper[q1];
			const auto& lastTensorSuper = tensors[lastTensorId];
			lastTensorIndexForQubit = lastTensorIndicesSuper[q1];
			lastTensorSuper->connections[lastTensorIndexForQubit] = tensorId;
			lastTensorSuper->connectionsIndices[lastTensorIndexForQubit] = 0;

			tensorNode->connections[0] = lastTensorId;
			tensorNode->connectionsIndices[0] = lastTensorIndexForQubit;


			lastTensorId = lastTensorsSuper[q2];
			const auto& lastTensor2Super = tensors[lastTensorId];
			lastTensorIndexForQubit = lastTensorIndicesSuper[q2];
			lastTensor2Super->connections[lastTensorIndexForQubit] = tensorId;
			lastTensor2Super->connectionsIndices[lastTensorIndexForQubit] = 1;

			tensorNode->connections[1] = lastTensorId;
			tensorNode->connectionsIndices[1] = lastTensorIndicesSuper[q2];

			tensors.emplace_back(std::move(tensorNode));

			// now set the last tensors and indices
			lastTensorsSuper[q1] = tensorId;
			lastTensorsSuper[q2] = tensorId;

			lastTensorIndicesSuper[q1] = 2;
			lastTensorIndicesSuper[q2] = 3;
		}

		double Contract()
		{
			if (!contractor) return 0;
			contractor->SetMultithreading(enableMultithreading);

			Connect();

			// contract the network, save result to return
			double result = 1;

			for (auto& group : qubitsGroups)
			{
				const double groupResult = contractor->Contract(*this, *group.second.begin());  // doesn't really matter which qubit is passed as long it belongs to the group
				if (groupResult == 0) return 0;
				result *= groupResult;
			}

			// no need to actually disconnect the indices, as they are not used unless they are connected again
			// when adding a new gate/projection for measurement, they are reconnected anyway, the same goes for doing another contraction

			// return the result
			return result;
		}

		double Contract(const std::unordered_set<Types::qubit_t>& qubits)
		{
			if (!contractor) return 0;
			contractor->SetMultithreading(enableMultithreading);

			Connect();

			// contract the network, save result to return
			double result = 1;

			for (auto& group : qubitsGroups)
			{
				// this is probably less costly than contracting
				for (const auto q : group.second)
					if (qubits.find(q) != qubits.end())
					{
						// this group has at least one qubit in the set, contract it
						// break to avoid contracting it multiple times
						const double groupResult = contractor->Contract(*this, q);  // doesn't really matter which qubit is passed as long it belongs to the group
						if (groupResult == 0) return 0;
						result *= groupResult; 
						break;
					}
			}

			// no need to actually disconnect the indices, as they are not used unless they are connected again
			// when adding a new gate/projection for measurement, they are reconnected anyway, the same goes for doing another contraction

			// return the result
			return result;
		}

		void Clean(size_t numQubits)
		{
			SetQubitsTensors(numQubits);
		}

		void SetQubitsTensors(size_t numQubits)
		{
			for (Types::qubit_t q = 0; q < numQubits; ++q)
			{
				qubitsMap[q] = q; // the qubit group id is the qubit number itself
				qubitsGroups[q].insert(q); // the qubit group contains only the qubit itself

				auto tensorNode = std::make_shared<TensorNode>();
				tensorNode->SetQubit(q);
				tensorNode->SetId(static_cast<Index>(tensors.size()));

				lastTensors[q] = tensorNode->GetId();
				lastTensorIndices[q] = 0;
				tensors.emplace_back(std::move(tensorNode));

				tensorNode = std::make_shared<TensorNode>();
				tensorNode->SetQubit(q);
				tensorNode->SetId(static_cast<Index>(tensors.size()));
				tensorNode->SetSuper();

				lastTensorsSuper[q] = tensorNode->GetId();
				lastTensorIndicesSuper[q] = 0;
				tensors.emplace_back(std::move(tensorNode));
			}
		}

		void SetQubitsTensorsClear(size_t numQubits)
		{
			for (Types::qubit_t q = 0; q < numQubits; ++q)
			{
				qubitsMap[q] = q; // the qubit group id is the qubit number itself
				qubitsGroups[q].insert(q); // the qubit group contains only the qubit itself

				lastTensors[q] = 2 * q;
				lastTensorIndices[q] = 0;
				
				lastTensorsSuper[q] = lastTensors[q] + 1;
				lastTensorIndicesSuper[q] = 0;
			}
		}

		std::vector<std::shared_ptr<TensorNode>> tensors; // all tensors in the network

		std::vector<Index> lastTensors; // the indices of the last tensors in the network
		std::vector<Index> lastTensorsSuper; // the indices of the last super tensors in the network

		std::vector<Index> lastTensorIndices; // the indices of the last tensors in the network
		std::vector<Index> lastTensorIndicesSuper; // the indices of the last super tensors in the network

		std::vector<size_t> qubitsMap; /**< A map between qubits (as identified from outside) and the qubits group ids */
		std::unordered_map<size_t, std::unordered_set<Types::qubit_t>> qubitsGroups; /**< A map between qubits group ids and the qubits in that group */


		size_t savedTensorsNr = 0;
		std::vector<std::shared_ptr<TensorNode>> saveTensors; // all tensors in the network

		std::vector<Index> saveLastTensors; // the indices of the last tensors in the network
		std::vector<Index> saveLastTensorsSuper; // the indices of the last super tensors in the network

		std::vector<Index> saveLastTensorIndices; // the indices of the last tensors in the network
		std::vector<Index> saveLastTensorIndicesSuper; // the indices of the last super tensors in the network

		std::vector<size_t> saveQubitsMap; /**< A map between qubits (as identified from outside) and the qubits group ids */
		std::unordered_map<size_t, std::unordered_set<Types::qubit_t>> saveQubitsGroups; /**< A map between qubits group ids and the qubits in that group */


		std::shared_ptr<ITensorContractor> contractor;
		
		bool enableMultithreading = true;

		std::mt19937_64 rng;
		std::uniform_real_distribution<double> uniformZeroOne;
	};

}

#endif // __TENSOR_NETWORK_H_
