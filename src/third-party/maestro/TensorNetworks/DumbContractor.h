/**
 * @file DumbContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Dumb Tensor Contractor.
 * This is a trivial contractor that contracts the tensors very greedily, without any attempt to optimize the contraction order.
 * The idea is to pick the first tensor in the list and contract it with the first tensor it is connected to.
 * Or even simply the first two tensors in the list. If this is the case and they are not connected, the contraction will perform the outer product.
 * It works even like that, but usually worse than when picking out tensors that have some connections.
 * It's only to have something deterministic (for a given tensor network and its given order) to compare with the other contractors.
 * If they perform worse, it's bad.
 *
 * Tensor contractions using the Dumb contraction method.
 */

#pragma once

#ifndef  __DUMB_CONTRACTOR_H_
#define __DUMB_CONTRACTOR_H_ 1

#include "BaseContractor.h"

#include <boost/container_hash/hash.hpp>

namespace TensorNetworks {

	/**
	 * @brief The Dumb Tensor Contractor.
	 *
	 * Tensor contractions using the Dumb contraction method.
	 */
	class DumbContractor : public BaseContractor {
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

			Eigen::Index minId = std::numeric_limits<Eigen::Index>::max();

			if (contractTheLowestTensorId)
			{
				for (const auto& tensor : tensors)
				{
					if (tensor.first < minId)
						minId = tensor.first;
				}
			}

			//auto it = tensors.begin();

			// while there is more than one tensor...
			while (tensors.size() > 1)
			{
				Eigen::Index tensor1Id;
				Eigen::Index tensor2Id;

				if (contractTheLowestTensorId)
				{
					tensor1Id = minId;
					auto it = tensors.find(tensor1Id);
					++it;
					if (it == tensors.end())
						it = tensors.begin();

					tensor2Id = it->first;
				}
				else
				{
					auto it = tensors.begin();
					tensor1Id = it->first;
					++it;
					tensor2Id = it->first;
				}

				// you could even comment out this loop and it should work
				// the unconnected tensors will still be contracted, the contraction result is the outer product
				// this is a good chance to test the contraction that involves outer products, as the other contractors avoid it

				const auto& tensor = tensors[tensor1Id];
				Eigen::Index resultRank = std::numeric_limits<Eigen::Index>::max();
				for (Eigen::Index ti = 0; ti < static_cast<Eigen::Index>(tensor->connections.size()); ++ti)
				{
					const auto nextTensorId = tensor->connections[ti];
					if (nextTensorId != TensorNode::NotConnected)
					{
						tensor2Id = nextTensorId;

						const Eigen::Index newRank = GetResultRank(tensor, tensors[nextTensorId]);

						if (newRank <= resultRank)
						{
							if (newRank < resultRank)
							{
								tensor2Id = nextTensorId;
								resultRank = newRank;
							}
						}
					}
				}

				//it = tensors.find(tensor2Id);
				//++it;
				//if (it == tensors.end())
				//	it = tensors.begin();

				ContractNodes(qubit, tensors, tensor1Id, tensor2Id, resultRank);

				if (resultRank == 0) 
				{
					if (tensors.size() == 1 || tensor->contractsTheNeededQubit)
						return std::real(tensor->tensor->atOffset(0));
					// erasing this tensor happens because (not the case anymore, it's avoided) the tensor network might be a disjoint one and a subnetwork is contracted that does not contain the needed qubit
					tensors.erase(tensor1Id);

					if (contractTheLowestTensorId)
					{
						minId = std::numeric_limits<Eigen::Index>::max();
						for (const auto& tensor : tensors)
						{
							if (tensor.first < minId)
								minId = tensor.first;
						}
					}
				}
			}

			return std::real(tensors.begin()->second->tensor->atOffset(0));
		}

		void SetContractTheLowestTensorId(bool c)
		{
			contractTheLowestTensorId = c;
		}

		bool GetContractTheLowestTensorId() const
		{
			return contractTheLowestTensorId;
		}

		/**
		 * @brief Clone the tensor contractor.
		 *
		 * @return A shared pointer to the cloned tensor contractor.
		 */
		std::shared_ptr<ITensorContractor> Clone() const override
		{
			auto cloned = std::make_shared<DumbContractor>();

			cloned->maxTensorRank = maxTensorRank;
			cloned->enableMultithreading = enableMultithreading;
			cloned->contractTheLowestTensorId = contractTheLowestTensorId;

			return cloned;
		}

	private:
		bool contractTheLowestTensorId = true;
	};

}

#endif // __DUMB_CONTRACTOR_H_
