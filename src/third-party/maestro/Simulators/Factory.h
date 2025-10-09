/**
 * @file Factory.h
 * @version 1.0
 * 
 * @section DESCRIPTION
 * 
 * Factory for simulators.
 * 
 * Currently only two simulators are supported: qiskit aer and qcsim.
 * Can be esily extended to support more simulators.
 * Call CreateSimulator with the desired simulator type to create a simulator returned as a shared pointer.
 */

#pragma once

#ifndef _SIMULATORS_FACTORY_H_
#define _SIMULATORS_FACTORY_H_

#include "Simulator.h"
#include "GpuLibrary.h"
#include "GpuLibStateVectorSim.h"
#include "GpuLibMPSSim.h"

namespace Simulators {

	/**
	 * @class SimulatorsFactory 
	 * @brief Factory for simulators.
	 * 
	 * Create either a qiskit aer or qcsim simulator.
	 */
	class SimulatorsFactory {
	public:
		/**
		 * @brief Create a quantum computing simulator.
		 * 
		 * @param t The type of simulator to create.
		 * @return The simulator wrapped in a shared pointer.
		 */
		static std::shared_ptr<ISimulator> CreateSimulator(SimulatorType t = 
			SimulatorType::kQCSim
			, SimulationType method = SimulationType::kMatrixProductState
		);

		/**
		 * @brief Create a quantum computing simulator.
		 *
		 * @param t The type of simulator to create.
		 * @return The simulator wrapped in a unique pointer.
		 */
		static std::unique_ptr<ISimulator> CreateSimulatorUnique(SimulatorType t = 
			SimulatorType::kQCSim	
			, SimulationType method = SimulationType::kMatrixProductState
		);

#ifdef __linux__
		static std::unique_ptr<GpuLibStateVectorSim> CreateGpuLibStateVectorSim()
		{
			if (!gpuLibrary || !gpuLibrary->IsValid())
				return nullptr;

			return std::make_unique<GpuLibStateVectorSim>(gpuLibrary);
		}

		static std::unique_ptr<GpuLibMPSSim> CreateGpuLibMPSSim()
		{
			if (!gpuLibrary || !gpuLibrary->IsValid())
				return nullptr;

			return std::make_unique<GpuLibMPSSim>(gpuLibrary);
		}

		static std::shared_ptr<GpuLibrary> GetGpuLibrary()
		{
			if (!gpuLibrary || !gpuLibrary->IsValid())
				return nullptr;
			return gpuLibrary;
		}

		static bool IsGpuLibraryAvailable()
		{
			return gpuLibrary && gpuLibrary->IsValid();
		}

		static bool InitGpuLibrary();

	private:
		static std::shared_ptr<GpuLibrary> gpuLibrary;
#else		
		static bool IsGpuLibraryAvailable()
		{
			return false;
		}

		static bool InitGpuLibrary()
		{
			return false;
		}
#endif
	};

}

#endif // !_SIMULATORS_FACTORY_H_
