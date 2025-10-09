/**
 * @file ForestContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Forest Tensor Contractor.
 * A forest circuit (in particular, a tree) is a quantum circuit that can be simulated efficiently on a classical computer.
 * The contraction of the tensors in the network can be done efficiently without increasing the tensors' rank.
 * Just start with contracting the leaves of the circuit tree with the leaves of the 'super' tree, getting a rank-2 tensor (that is, a matrix - equivalent with a one qubit gate if a projector is not involved)
 * which can be contracted with the new leaf... and so on until the root is also contracted out.
 *
 * Tensor contractions using the Forest contraction method.
 */

#pragma once

#ifndef  __FOREST_CONTRACTOR_H_
#define __FOREST_CONTRACTOR_H_ 1

#include "BaseContractor.h"

#include <boost/container_hash/hash.hpp>

namespace TensorNetworks {

	/**
	 * @brief The Forest Tensor Contractor.
	 *
	 * Tensor contractions using the Forest contraction method.
	 */
	class ForestContractor : public BaseContractor {
	public:
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
			std::map<Eigen::Index, std::shared_ptr<TensorNode>> tensorsMap(tensors.begin(), tensors.end());
			
			using TensorPair = std::pair<Eigen::Index, Eigen::Index>;
			std::unordered_set<TensorPair, boost::hash<TensorPair>> visitedPairs;

			Eigen::Index maxRank = 4;
			Eigen::Index nextRank = 4;

			// the first two initialized just to keep the compiler happy
			Eigen::Index tensor1Id = 0;
			Eigen::Index tensor2Id = 1;
			Eigen::Index resultRank = 4;
			Eigen::Index bestCost = 4;

			// while there is more than one tensor...
			while (tensors.size() > 1)
			{
				// by going like this we pick up the tensors that are closer to the end of the circuit, to contract with the 'super' ones
				// can be very efficient for forest kind of circuits

				bool found = false;
				bool nextRankSet = false;

				for (auto tensorIt = tensorsMap.rbegin(); tensorIt != tensorsMap.rend(); ++tensorIt)
				{
					const auto& tensor = tensorIt->second;
					const auto curTensorId = tensor->GetId();
					
					// despite having a bigger rank, the contraction could lead to a smaller one to match the rank limit, so don't do this
					//if (tensor->GetRank() > maxRank) continue;

					for (Eigen::Index ti = 0; ti < static_cast<Eigen::Index>(tensor->connections.size()); ++ti)
					{
						const auto nextTensorId = tensor->connections[ti];
						
						if (nextTensorId != TensorNode::NotConnected)
						{
							auto t1 = curTensorId;
							auto t2 = nextTensorId;
							if (t1 > t2)
								std::swap(t1, t2);

							const auto p = std::make_pair(t1, t2);
							if (visitedPairs.find(p) != visitedPairs.end())
								continue;
							else visitedPairs.insert(p);

							const Eigen::Index newRank = GetResultRank(tensor, tensors[nextTensorId]);

							if (newRank <= maxRank)
							{
								const Eigen::Index Cost = newRank - (tensor->GetRank() + tensors[nextTensorId]->GetRank()) / 2;

								if (!found || newRank < resultRank || (newRank == resultRank && Cost < bestCost))
								{
									tensor1Id = curTensorId;
									tensor2Id = nextTensorId;
									resultRank = newRank;
									bestCost = Cost;
									found = true;
								}
							}
							
							if (!found)
							{
								if (!nextRankSet)
								{
									nextRankSet = true;
									nextRank = newRank;
								}
								else
									nextRank = std::min(nextRank, newRank);
							}
						}
					}
				}

				visitedPairs.clear();

				if (!found) 
				{
					maxRank = nextRank;
					continue;
				}

				ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);
				tensorsMap[tensor1Id] = tensors[tensor1Id];
				tensorsMap.erase(tensor2Id);

				if (resultRank == 0)
				{
					if (tensors.size() == 1 || tensors[tensor1Id]->contractsTheNeededQubit)
						return std::real(tensors[tensor1Id]->tensor->atOffset(0));

					// erasing this tensor happens because (not the case anymore, it's avoided) the tensor network might be a disjoint one and a subnetwork is contracted that does not contain the needed qubit

					tensors.erase(tensor1Id);
					tensorsMap.erase(tensor1Id);
				}
			}

			return std::real(tensors.begin()->second->tensor->atOffset(0));
		}

		/**
		 * @brief Clone the tensor contractor.
		 *
		 * @return A shared pointer to the cloned tensor contractor.
		 */
		std::shared_ptr<ITensorContractor> Clone() const override
		{
			auto cloned = std::make_shared<ForestContractor>();

			cloned->maxTensorRank = maxTensorRank;
			cloned->enableMultithreading = enableMultithreading;

			return cloned;
		}
	};

}

#endif // __FOREST_CONTRACTOR_H_
