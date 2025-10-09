/**
 * @file VerticalContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Vertical Variable Elimination Tensor Contractor.
 * Idea is from 'variable elimination algorithm', A. Vertical variable elimination for low-depth circuits
 * Sergio Boixo et al, Simulation of low-depth quantum circuits as complex undirected graphical models
 * arxiv:1712.05384v2 [quant-ph] 19 Jan 2018
 *
 * Tensor contractions using the Vertical Variable Elimination method.
 */

#pragma once

#ifndef  __VERTICAL_CONTRACTOR_H_
#define __VERTICAL_CONTRACTOR_H_ 1

#include "BaseContractor.h"

#include <boost/container_hash/hash.hpp>

namespace  TensorNetworks {

	/**
	 * @brief The Vertical Variable Elimination Tensor Contractor.
	 *
	 * Tensor contractions using the Vertical Variable Elimination method.
	 */
	class VerticalContractor : public BaseContractor {
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

			const auto& qubitGroup = network.GetQubitGroup(qubit);
			const std::set<Types::qubit_t> qubitGroupSet(qubitGroup.begin(), qubitGroup.end());

			using TensorPair = std::pair<Eigen::Index, Eigen::Index>;
			std::unordered_set<TensorPair, boost::hash<TensorPair>> visitedPairs;

			auto qit = qubitGroupSet.begin();

			// while there is more than one tensor...
			while (tensors.size() > 1)
			{
				if (qit == qubitGroupSet.end())
					throw std::out_of_range("The contraction qubit is out of range.");

				const auto q = *qit;

				bool found = false;

				Eigen::Index tensor1IdBest;
				Eigen::Index tensor2IdBest;
				Eigen::Index resultRankBest;
				Eigen::Index bestCost;

				for (auto tensorIt = tensors.begin(); tensorIt != tensors.end(); ++tensorIt)
				{
					const auto& tensor = tensorIt->second;
					Eigen::Index tensor1Id = tensor->GetId();

					for (Eigen::Index qi = 0; qi < static_cast<Eigen::Index>(tensor->qubits.size()); ++qi)
					{
						if (tensor->qubits[qi] == q)
						{
							const Eigen::Index tensor2Id = tensor->connections[qi];

							if (tensor2Id != TensorNode::NotConnected)
							{
								auto t1 = tensor1Id;
								auto t2 = tensor2Id;
								if (t1 > t2)
									std::swap(t1, t2);
								if (visitedPairs.find(std::make_pair(t1, t2)) != visitedPairs.end())
									continue;
								else visitedPairs.insert(std::make_pair(t1, t2));

								const Eigen::Index resultRank = GetResultRank(tensors[tensor1Id], tensors[tensor2Id]);

								//const Eigen::Index Cost = resultRank - std::max(tensor->GetRank(), tensors[tensor2Id]->GetRank());
								const Eigen::Index Cost = resultRank - (tensor->GetRank() + tensors[tensor2Id]->GetRank()) / 2;

								if (!found || resultRank < resultRankBest || (resultRank == resultRankBest && Cost < bestCost))
								{
									resultRankBest = resultRank;
									bestCost = Cost;
									tensor1IdBest = tensor1Id;
									tensor2IdBest = tensor2Id;
									found = true;
								}
							}
						}
					}
				}

				visitedPairs.clear();
				
				if (!found)
				{
					++qit;
					continue;
				}

				ContractNodes(qubit, tensors, tensor1IdBest, tensor2IdBest, resultRankBest);

				if (resultRankBest == 0)
				{
					if (tensors.size() == 1 || tensors[tensor1IdBest]->contractsTheNeededQubit)
						return std::real(tensors[tensor1IdBest]->tensor->atOffset(0));
					// erasing this tensor happens because (not the case anymore, it's avoided) the tensor network might be a disjoint one and a subnetwork is contracted that does not contain the needed qubit
					tensors.erase(tensor1IdBest);
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
			auto cloned = std::make_shared<VerticalContractor>();
			cloned->maxTensorRank = maxTensorRank;
			cloned->enableMultithreading = enableMultithreading;
			return cloned;
		}
	};

} // namespace TensorNetworks

#endif // __VERTICAL_CONTRACTOR_H_

