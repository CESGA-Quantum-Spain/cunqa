/**
 * @file EstimatorInterface.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Interface class for the execution estimators, exposes a common interface.
 */

#pragma once

#ifndef __ESTIMATOR_INTERFACE_H_
#define __ESTIMATOR_INTERFACE_H_

#include "Simulators/Simulator.h"

#include <vector>
#include <string>

namespace Estimators {

	/**
	 * @class EstimatorInterface
	 * @brief An interface for various runtime estimators.
	 *
	 * This interface defines a method for estimating the execution time of quantum simulations.
	 * There could be various implementations of this interface that provide different estimation strategies,
	 * also they might be specialized for certain simulators and/or simulation types (for example clifford, mps, statevector).
	 * It's not used directly, but in an implementation of the SimulatorsEstimatorInterface.
	 * @sa SimulatorsEstimatorInterface
	 */
	class EstimatorInterface {
	public:
		virtual ~EstimatorInterface() = default;

		virtual double EstimateTime(Simulators::SimulatorType type, Simulators::SimulationType method) const = 0;
		virtual double EstimateExpectationValuesTime(Simulators::SimulatorType type, Simulators::SimulationType method, const std::vector<std::string> &paulis) = 0;
	};
}

#endif // !__ESTIMATOR_INTERFACE_H_
