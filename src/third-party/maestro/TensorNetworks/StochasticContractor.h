/**
 * @file StochasticContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Stochastic Tensor Contractor.
 * Idea is from the 'Algorithm 2' from arXiv:1709.03636v2 [quant-ph] 22 Dec 2018
 * qTorch: The quantum tensor contraction handler
 *
 * Tensor contractions using a stochastic method.
 */

#pragma once

#ifndef  __STOCHASTIC_CONTRACTOR_H_
#define __STOCHASTIC_CONTRACTOR_H_ 1

#include "BaseContractor.h"

#include <random>
#include <boost/container_hash/hash.hpp>

namespace TensorNetworks {

	/**
	 * @brief The Stochastic Tensor Contractor.
	 *
	 * Tensor contractions using a stochastic method.
	 */
	class StochasticContractor : public BaseContractor {
	public:
		/**
		 * @brief Constructor.
		 * 
		 * Constructs the Stochastic Tensor Contractor.
		 * Initializes the random number generator.
		 */
		StochasticContractor() : gen(std::random_device{}()) 
		{
		}

		/**
		 * @brief Contract the tensor network.
		 *
		 * @param network The tensor network to contract.
		 * @return The result of the contraction.
		 */
		double Contract(const TensorNetwork& network, Types::qubit_t qubit) override
		{
			// algorithm very similar with the 'Algorithm 2' from arXiv:1709.03636v2 [quant-ph] 22 Dec 2018
			// qTorch: The quantum tensor contraction handler
			// there are some changes, though, in picking up the contraction

			long long int CostThreshold = -1;
			size_t rejections = 0;

			std::vector<Eigen::Index> keys;
			std::unordered_map<Eigen::Index, Eigen::Index> keysKeys;
		
			auto tensors = InitializeTensors(network, qubit, keys, keysKeys);

			using TensorPair = std::pair<Eigen::Index, Eigen::Index>;
			std::unordered_set<TensorPair, boost::hash<TensorPair>> visitedPairs;
						
			// while there is more than one tensor...
			while (tensors.size() > 1)
			{
				// choose a random tensor
				std::uniform_int_distribution<Eigen::Index> tensor1Dist(0LL, static_cast<Eigen::Index>(keys.size()) - 1);
				auto pos1 = tensor1Dist(gen);
				Eigen::Index tensor1Id = keys[pos1];
				const auto tensor1 = tensors[tensor1Id];

				// then a random tensor it is connected to
				// this favors the ones that have more connection with the chosen one

				std::uniform_int_distribution<Eigen::Index> tensor2Dist(0LL, static_cast<Eigen::Index>(tensor1->connections.size()) - 1);
				auto pos2 = tensor2Dist(gen);
				Eigen::Index tensor2Id = tensor1->connections[pos2];

				auto t1 = tensor1Id;
				auto t2 = tensor2Id;
				if (t1 > t2)
					std::swap(t1, t2);
				if (visitedPairs.find(std::make_pair(t1, t2)) != visitedPairs.end())
				{
					++rejections;
					if (rejections >= MaxRejections)
					{
						++CostThreshold;
						rejections = 0;
						visitedPairs.clear();
					}
					continue;
				}
				else visitedPairs.insert(std::make_pair(t1, t2));

				const auto tensor2 = tensors[tensor2Id];

				const auto keysKeysIt = keysKeys.find(tensor2Id);
				if (keysKeysIt != keysKeys.end())
					pos2 = keysKeysIt->second;

				const Eigen::Index resultRank = GetResultRank(tensor1, tensor2);
				//const Eigen::Index Cost = resultRank - std::max(tensor1->GetRank(), tensor2->GetRank());
				const Eigen::Index Cost =  resultRank - (tensor1->GetRank() + tensor2->GetRank()) / 2;
				if (Cost <= CostThreshold)
				{
					rejections = 0;
					CostThreshold = -1;

					ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);

					visitedPairs.clear();

					// also remove them from 'keys' vector
					// actually only one needs to be removed, the other one will be replaced by the result tensor
					if (pos1 == static_cast<Eigen::Index>(keys.size()) - 1)
						pos1 = pos2;

					keys[pos2] = keys.back();
					keysKeys[keys[pos2]] = pos2;
					keysKeys.erase(tensor2Id);
					keys.resize(keys.size() - 1);

					if (resultRank == 0)
					{
						if (tensors.size() == 1 || tensors[tensor1Id]->contractsTheNeededQubit)
							return std::real(tensors[tensor1Id]->tensor->atOffset(0));

						// erasing this tensor happens because (not the case anymore, it's avoided) the tensor network might be a disjoint one and a subnetwork is contracted that does not contain the needed qubit
						keys[pos1] = keys.back();
						keysKeys[keys[pos1]] = pos1;
						keysKeys.erase(tensor1Id);
						keys.resize(keys.size() - 1);

						tensors.erase(tensor1Id);
					}
				}
				else
				{
					++rejections;
					if (rejections >= MaxRejections)
					{
						++CostThreshold;
						rejections = 0;
						visitedPairs.clear();
					}
				}
			}

			return std::real(tensors.begin()->second->tensor->atOffset(0));
		}

		size_t GetMaxRejections() const
		{
			return MaxRejections;
		}

		void SetMaxRejections(size_t maxRejections)
		{
			MaxRejections = maxRejections;
		}

		/**
		 * @brief Clone the tensor contractor.
		 *
		 * @return A shared pointer to the cloned tensor contractor.
		 */
		std::shared_ptr<ITensorContractor> Clone() const override
		{
			auto cloned = std::make_shared<StochasticContractor>();

			cloned->maxTensorRank = maxTensorRank;
			cloned->enableMultithreading = enableMultithreading;
			cloned->MaxRejections = MaxRejections;

			return cloned;
		}

	protected:		
		std::mt19937 gen;
		size_t MaxRejections = 15;
	};

}



#endif // __STOCHASTIC_CONTRACTOR_H_
