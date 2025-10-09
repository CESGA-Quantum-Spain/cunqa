/**
 * @file BaseContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Base class for tensor contractors.
 *
 * Contains common functionality for tensor contractors.
 */

#pragma once

#ifndef  __BASE_CONTRACTOR_H_
#define __BASE_CONTRACTOR_H_ 1

#include "TensorContractor.h"

#include <random>

namespace TensorNetworks {

	/**
	 * @brief The Base Class Tensor Contractor.
	 *
	 * Tensor contractions common functionality.
	 */
	class BaseContractor : public ITensorContractor {
	public:
		using TensorsMap = ITensorContractor::TensorsMap;

		TensorsMap InitializeTensors(const TensorNetwork& network, Types::qubit_t qubit, std::vector<Eigen::Index>& keys, std::unordered_map<Eigen::Index, Eigen::Index>& keysKeys, bool fillKeys = true, bool contract = true) override
		{
			maxTensorRank = 0;

			TensorsMap tensors;

			if (fillKeys) keys.reserve(network.GetTensors().size());

			const auto& qubitGroup = network.GetQubitGroup(qubit);

			if (contract)
			{
				for (const auto& tensor : network.GetTensors())
				{
					if (!tensor) continue;

					const auto firstQubit = tensor->qubits[0];
					if (qubitGroup.find(firstQubit) == qubitGroup.end())
						continue;

					const auto tensorId = tensor->GetId();
					tensors[tensorId] = tensor->CloneWithoutTensorCopy();
				}

				for (const auto& [tensor1Id, tensor1] : tensors)
				{
					// check the tensor, see if it can be contracted, contract it if possible
					const auto qubitsNr = tensor1->qubits.size();
					if (qubitsNr <= 2)
					{
						// try to contract with something either that is in a previous position or something following (there should be always something following)
						const auto tensor2Id = tensor1->connections[0];
						const auto& tensor2 = tensors[tensor2Id];

						const auto resultRank = GetResultRank(tensor1, tensor2);
						ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);

						tensors.erase(tensor2Id);
					}
					else
					{
						// could be preceeded by another two qubit tensor that can be contracted without increasing the rank (that is, two two qubit gates on the same qubits) 
						const auto tensor2Id = tensor1->connections[0];
						const auto tensor3Id = tensor1->connections[1];

						if (tensor2Id == tensor3Id)
						{
							const auto& tensor2 = tensors[tensor2Id];

							const auto resultRank = GetResultRank(tensor1, tensor2);
							ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);

							tensors.erase(tensor2Id);
						}
					}
				}

				// now build the keys
				for (const auto& [tensorId, tensor] : tensors)
				{
					if (fillKeys)
					{
						keysKeys[tensorId] = keys.size();
						keys.push_back(tensorId);
					}

					maxTensorRank = std::max(maxTensorRank, tensor->GetRank());
				}
			}
			else
			{
				for (const auto& tensor : network.GetTensors())
				{
					if (!tensor) continue;

					const auto firstQubit = tensor->qubits[0];
					if (qubitGroup.find(firstQubit) == qubitGroup.end())
						continue;

					const auto tensorId = tensor->GetId();
					tensors[tensorId] = tensor->CloneWithoutTensorCopy();
					
					if (fillKeys)
					{
						keysKeys[tensorId] = keys.size();
						keys.push_back(tensorId);
					}

					maxTensorRank = std::max(maxTensorRank, tensor->GetRank());
				}
			}

			return tensors;
		}

		template<class PassedTensorsMap = TensorsMap> inline Eigen::Index ContractNodes(Types::qubit_t qubit, PassedTensorsMap& tensors, Eigen::Index tensor1Id, Eigen::Index tensor2Id, Eigen::Index resultRank)
		{
			const auto& tensor1 = tensors[tensor1Id];
			const auto& tensor2 = tensors[tensor2Id];

			// contract them to get the result tensor
			std::vector<std::pair<size_t, size_t>> indices;
			for (size_t i = 0; i < tensor1->connections.size(); ++i)
				if (tensor1->connections[i] == tensor2Id)
					indices.emplace_back(i, tensor1->connectionsIndices[i]);


			maxTensorRank = std::max<size_t>(maxTensorRank, resultRank);

			const auto resultNode = std::make_shared<TensorNode>();
			resultNode->tensor = std::make_shared<Utils::Tensor<>>(std::move(tensor1->tensor->Contract(*(tensor2->tensor), indices, enableMultithreading)));
			resultNode->SetId(tensor1Id);

			const auto newRank = resultNode->GetRank();
			
			resultNode->connections.resize(newRank);
			resultNode->connectionsIndices.resize(newRank);
			resultNode->qubits.resize(newRank);


			// update tensors graph with the new result, removing the two old ones

			// for the old ones, all connections of other tensors must be updated to point to the result tensor
			// pay attention to the proper indices positions

			// the first indices of the new tensor come from the first tensor, the following ones from the second tensor
			bool properQubit = false;

			size_t pos = 0;
			for (size_t i = 0; i < tensor1->connections.size(); ++i)
			{
				if (qubit == tensor1->qubits[i]) properQubit = true;

				const auto connectedTensorId = tensor1->connections[i];
				if (connectedTensorId != tensor2Id)
				{
					// update connection of the tensor
					const auto otherTensorIndex = tensor1->connectionsIndices[i];

					resultNode->connections[pos] = connectedTensorId;
					resultNode->connectionsIndices[pos] = otherTensorIndex;
					resultNode->qubits[pos] = tensor1->qubits[i];

					// also need to update the 'other' tensor that is connected to the first tensor
					//tensors[connectedTensorId]->connections[otherTensorIndex] = tensor1Id; // no need to update this, the new tensor inherits the id from tensor1
					tensors[connectedTensorId]->connectionsIndices[otherTensorIndex] = pos;

					++pos;
				}
			}

			for (size_t i = 0; i < tensor2->connections.size(); ++i)
			{
				if (qubit == tensor2->qubits[i]) properQubit = true;

				const auto connectedTensorId = tensor2->connections[i];
				if (connectedTensorId != tensor1Id)
				{
					// update connection of the tensor
					const auto otherTensorIndex = tensor2->connectionsIndices[i];

					resultNode->connections[pos] = connectedTensorId;
					resultNode->connectionsIndices[pos] = otherTensorIndex;
					resultNode->qubits[pos] = tensor2->qubits[i];

					// also need to update the 'other' tensor that is connected to the second tensor
					tensors[connectedTensorId]->connections[otherTensorIndex] = tensor1Id;
					tensors[connectedTensorId]->connectionsIndices[otherTensorIndex] = pos;

					++pos;
				}
			}

			resultNode->contractsTheNeededQubit = properQubit || tensor1->contractsTheNeededQubit || tensor2->contractsTheNeededQubit;

			// now update the tensors map
			tensors.erase(tensor2Id);
			tensors[tensor1Id] = resultNode;

			return tensor1Id;
		}

		static inline size_t GetResultRank(const std::shared_ptr<TensorNode>& tensor1, const std::shared_ptr<TensorNode>& tensor2)
		{
			size_t rank = 0;

			const Eigen::Index tensor1Id = tensor1->GetId();
			const Eigen::Index tensor2Id = tensor2->GetId();

			for (size_t i = 0; i < tensor1->connections.size(); ++i)
				if (tensor1->connections[i] != tensor2Id)
					++rank;

			for (size_t i = 0; i < tensor2->connections.size(); ++i)
				if (tensor2->connections[i] != tensor1Id)
					++rank;

			// even if the true rank of the result is 1 in such a case, return 0 as it has inside only a scalar
			//if (rank == 0) rank = 1;

			return rank;
		}

		size_t GetMaxTensorRank() const override
		{
			return maxTensorRank;
		}

		/**
		 * @brief Enable/disable multithreading.
		 *
		 * Enable/disable multithreading. Default is enabled.
		 *
		 * @param multithreading A flag to indicate if multithreading should be enabled.
		 */
		void SetMultithreading(bool multithreading = true) override
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
		bool GetMultithreading() const override
		{
			return enableMultithreading;
		}

		protected:
			size_t maxTensorRank = 0; /**< The maximum rank of the tensors in the network. */
			bool enableMultithreading = true; /**< A flag to indicate if multithreading should be enabled. */
	};

} // namespace TensorNetworks

#endif // __BASE_CONTRACTOR_H_
