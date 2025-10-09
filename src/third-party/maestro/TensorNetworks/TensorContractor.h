/**
 * @file TensorContractor.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Tensor Contractor interface.
 *
 * Defines an interface for tensor contractions.
 * There might be different implementations for different types of contractions.
 */

#pragma once

#ifndef  __TENSOR_CONTRACTOR_H_
#define __TENSOR_CONTRACTOR_H_ 1

#include <memory>

#include "../Utils/Tensor.h"
#include "TensorNetwork.h"

namespace TensorNetworks {

	class TensorNetwork;

	/**
	 * @brief Tensor Contractor interface.
	 *
	 * Defines an interface for tensor contractions.
	 * There might be different implementations for different types of contractions.
	 */
	class ITensorContractor {
	public:
		using TensorsMap = std::unordered_map<Eigen::Index, std::shared_ptr<TensorNode>>;

		virtual TensorsMap InitializeTensors(const TensorNetwork& network, Types::qubit_t qubit, std::vector<Eigen::Index>& keys, std::unordered_map<Eigen::Index, Eigen::Index>& keysKeys, bool fillKeys = true, bool contract = true) = 0;
		
		/**
		 * @brief Contract the tensor network.
		 *
		 * @param network The tensor network to contract.
		 * @return The result of the contraction.
		 */
		virtual double Contract(const TensorNetwork& network, Types::qubit_t qubit) = 0;

		virtual size_t GetMaxTensorRank() const = 0;

		/**
		 * @brief Enable/disable multithreading.
		 *
		 * Enable/disable multithreading. Default is enabled.
		 *
		 * @param multithreading A flag to indicate if multithreading should be enabled.
		 */
		virtual void SetMultithreading(bool multithreading = true) = 0;

		/**
		 * @brief Get the multithreading flag.
		 *
		 * Returns the multithreading flag.
		 *
		 * @return The multithreading flag.
		 */
		virtual bool GetMultithreading() const = 0;

		/**
		 * @brief Clone the tensor contractor.
		 *
		 * @return A shared pointer to the cloned tensor contractor.
		 */
		virtual std::shared_ptr<ITensorContractor> Clone() const = 0;
	};

}

#endif // __TENSOR_CONTRACTOR_H_