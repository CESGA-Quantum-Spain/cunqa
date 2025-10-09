/**
 * @file LookaheadContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Lookahead Tensor Contractor.
 * This is a contractor that 'looks ahead' several levels deep in order to pick the best tensors to contract.
 *
 * Tensor contractions using the Lookahead contraction method.
 */

#pragma once

#ifndef  __LOOKAHEAD_CONTRACTOR_H_
#define __LOOKAHEAD_CONTRACTOR_H_ 1

#include "BaseContractor.h"

#include <boost/container_hash/hash.hpp>

namespace TensorNetworks {

	/**
	 * @brief The Lookahead Tensor Contractor.
	 *
	 * Tensor contractions using the Lookahead contraction method.
	 */
	class LookaheadContractor : public BaseContractor {
	public:
		using TensorPair = std::pair<Eigen::Index, Eigen::Index>;
		using VisitedPairType = std::unordered_set<TensorPair, boost::hash<TensorPair>>;
		using DummyTensorsMap = std::map<Eigen::Index, std::shared_ptr<TensorNode>>;

		/**
		 * @brief Contract the tensor network.
		 *
		 * @param network The tensor network to contract.
		 * @return The result of the contraction.
		 */
		double Contract(const TensorNetwork& network, Types::qubit_t qubit) override
		{
			std::vector<Eigen::Index> keys;
			std::unordered_map<Eigen::Index, Eigen::Index> keysKeys;

			TensorsMap tensors = InitializeTensors(network, qubit, keys, keysKeys, false);
			DummyTensorsMap dummyTensors = CloneTensorsToDummy(tensors);


			// while there is more than one tensor...
			while (tensors.size() > 1)
			{
				// just to have them set to something
				auto it = tensors.begin();
				Eigen::Index tensor1Id = it->first;
				++it;
				Eigen::Index tensor2Id = it->first;

				const auto saveMaxTensorRank = maxTensorRank;

				Eigen::Index maxCost = std::numeric_limits<Eigen::Index>::max();
				size_t bestDepth = numberOfLevels;
				Lookahead(qubit, tensor1Id, tensor2Id, bestDepth, 0, 0, dummyTensors, 1, maxCost, 0);

				const auto& tensor = tensors[tensor1Id];
				const auto resultRank = GetResultRank(tensor, tensors[tensor2Id]);

				// also contract the dummies for the next step
				ContractNodes(qubit, dummyTensors, tensor1Id, tensor2Id, resultRank);
				
				maxTensorRank = saveMaxTensorRank;
				
				ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);

				if (resultRank == 0)
				{
					if (tensors.size() == 1 || tensor->contractsTheNeededQubit)
						return std::real(tensor->tensor->atOffset(0));
					// erasing this tensor happens because (not the case anymore, it's avoided) the tensor network might be a disjoint one and a subnetwork is contracted that does not contain the needed qubit
					tensors.erase(tensor1Id);
					dummyTensors.erase(tensor1Id);
				}
			}

			return std::real(tensors.begin()->second->tensor->atOffset(0));
		}


		size_t GetNumberOfLevels() const
		{
			return numberOfLevels;
		}

		void SetNumberOfLevels(size_t levels)
		{
			numberOfLevels = levels;
		}

		bool GetUseMaxRankCost() const
		{
			return useMaxRankCost;
		}

		void SetUseMaxRankCost(bool val = true)
		{
			useMaxRankCost = val;
		}


		/**
		 * @brief Clone the tensor contractor.
		 *
		 * @return A shared pointer to the cloned tensor contractor.
		 */
		std::shared_ptr<ITensorContractor> Clone() const override
		{
			auto cloned = std::make_shared<LookaheadContractor>();

			cloned->maxTensorRank = maxTensorRank;
			cloned->enableMultithreading = enableMultithreading;
			cloned->numberOfLevels = numberOfLevels;
			cloned->useMaxRankCost = useMaxRankCost;

			return cloned;
		}

	private:
		void Lookahead(Types::qubit_t qubit, Eigen::Index& tensor1Id, Eigen::Index& tensor2Id, size_t& bestDepth, Eigen::Index curt1, Eigen::Index curt2, DummyTensorsMap& dummyTensors, size_t curLevel, Eigen::Index& maxCost, Eigen::Index CostOnPath)
		{			
			if (dummyTensors.size() > 1 && curLevel < numberOfLevels)
			{
				std::unordered_set<TensorPair, boost::hash<TensorPair>> visitedPairs;

				for (auto tensorIt = dummyTensors.begin(); tensorIt != dummyTensors.end(); ++tensorIt)
				{
					auto tensor = tensorIt->second;
					const auto curTensorId = tensor->GetId();

					assert(tensor->tensor->IsDummy());

					// TODO: should I really try outer products here as well?
					for (Eigen::Index ti = 0; ti < static_cast<Eigen::Index>(tensor->connections.size()); ++ti)
					{
						const auto nextTensorId = tensor->connections[ti];

						if (nextTensorId != TensorNode::NotConnected)
						{
							auto t1 = curTensorId;
							auto t2 = nextTensorId;
							if (t1 > t2)
								std::swap(t1, t2);
							if (visitedPairs.find(std::make_pair(t1, t2)) != visitedPairs.end())
								continue;

							visitedPairs.insert(std::make_pair(t1, t2));
							
							const Eigen::Index newRank = GetResultRank(tensor, dummyTensors[nextTensorId]);
							//const Eigen::Index Cost = newRank - (tensor->GetRank() + dummyTensors[nextTensorId]->GetRank()) / 2;
							//const Eigen::Index newCostOnPath = CostOnPath + Cost;
							const Eigen::Index newCostOnPath = useMaxRankCost ? std::max(newRank, CostOnPath) : CostOnPath + newRank;
							if (newCostOnPath < maxCost)
							{
								// 1. if the curLevel is not the last level
								// contract the dummy tensors
								// then recursively call Lookahead (with CostOnPath = max(newRank, CostOnPath)

								// TODO: improve this, this is slow - actually save only the contracted tensors and restore them (and the linked ones as well) after the recursive call
								//auto newTensors = CloneTensorsToDummy(dummyTensors);

								auto saveTensor2 = dummyTensors[nextTensorId];

								ContractNodes(qubit, dummyTensors, curTensorId, nextTensorId, newRank);

								Lookahead(qubit, tensor1Id, tensor2Id, bestDepth, curLevel == 1 ? curTensorId : curt1, curLevel == 1 ? nextTensorId : curt2, dummyTensors, curLevel + 1, maxCost, newCostOnPath);

								// then restore the contraction for the next iteration

								dummyTensors[curTensorId] = tensor;
								dummyTensors[nextTensorId] = saveTensor2;

								// need to restore the connections back from the other connected tensors, too, because the contracting affected them

								for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(tensor->connections.size()); ++i)
								{
									if (tensor->connections[i] != TensorNode::NotConnected && tensor->connections[i] != nextTensorId)
									{
										const auto connectedTensorId = tensor->connections[i];
										const auto otherTensorIndex = tensor->connectionsIndices[i];
										//dummyTensors[otherTensorId]->connections[otherTensorIndex] = curTensorId;
										dummyTensors[connectedTensorId]->connectionsIndices[otherTensorIndex] = i;
									}
								}

								for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(saveTensor2->connections.size()); ++i)
								{
									if (saveTensor2->connections[i] != TensorNode::NotConnected && saveTensor2->connections[i] != curTensorId)
									{
										const auto connectedTensorId = saveTensor2->connections[i];
										const auto otherTensorIndex = saveTensor2->connectionsIndices[i];
										dummyTensors[connectedTensorId]->connections[otherTensorIndex] = nextTensorId;
										dummyTensors[connectedTensorId]->connectionsIndices[otherTensorIndex] = i;
									}
								}

								// otherwise ugly things will happen
								//tensorIt = std::make_reverse_iterator(dummyTensors.find(curTensorId));
								tensorIt = dummyTensors.find(curTensorId);
							}
						}
					}
				}
			}

			if ((curLevel == numberOfLevels && CostOnPath < maxCost) ||
				(dummyTensors.size() <= 1 && (CostOnPath < maxCost || (maxCost == CostOnPath && curLevel < bestDepth))))
			{
				maxCost = CostOnPath;
				bestDepth = curLevel;
				// set the tensor1Id and tensor2Id to the best ones (so far)
				tensor1Id = curt1;
				tensor2Id = curt2;
			}
		}

		static DummyTensorsMap CloneTensorsToDummy(const TensorsMap& tensors)
		{
			DummyTensorsMap clonedTensors;
			for (const auto& [tensorId, tensor] : tensors)
				clonedTensors[tensorId] = tensor->CloneWithADummyTensor();

			return clonedTensors;
		}

		size_t numberOfLevels = 3;
		bool useMaxRankCost = true;
	};

}

#endif // __LOOKAHEAD_CONTRACTOR_H_