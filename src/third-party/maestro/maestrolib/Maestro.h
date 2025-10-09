/**
 * @file Maestro.h
 * @version 1.0
 *
 * @section DESCRIPTION
 * Maestro class exposing functionality to Maestro.
 */

#pragma once

#ifdef COMPOSER
#include "../../composer/composer/Network/SimpleFullyConnectedNetwork.h"
#else
#include "../Network/SimpleDisconnectedNetwork.h"
#endif

#include "../Simulators/Factory.h"

class Maestro
{
public:
	Maestro() = default;
	~Maestro() = default;
	
	// Add methods to expose functionality to Maestro here.
	// For example, methods to create circuits, simulators, run simulations, etc.
	unsigned long int CreateSimpleSimulator(int nrQubits)
	{
		if (nrQubits <= 0)
			return 0;

		const std::vector<Types::qubit_t> qubits{ static_cast<Types::qubit_t>(nrQubits) };
		const std::vector<size_t> cbits{ static_cast<size_t>(nrQubits) };
		
#ifdef COMPOSER
		auto network = std::make_shared<Network::SimpleNetwork<>>(qubits, cbits);
#else
		auto network = std::make_shared<Network::SimpleDisconnectedNetwork<>>(qubits, cbits);
#endif

		// lock the mutex to safely add the simulator
		std::lock_guard<std::mutex> lock(simpleSimulatorsMutex);

		if (curHandle == std::numeric_limits<unsigned long int>::max())
		{
			// Handle overflow, reset to 0
			curHandle = 0;
		}
		const unsigned long int handle = ++curHandle;
		
		simpleSimulators[handle] = network;

		return handle;
	}

	void DestroySimpleSimulator(unsigned long int simHandle)
	{
		std::lock_guard<std::mutex> lock(simpleSimulatorsMutex);

		simpleSimulators.erase(simHandle);
	}

	std::shared_ptr<Network::INetwork<>> GetSimpleSimulator(unsigned long int simHandle)
	{
		std::lock_guard<std::mutex> lock(simpleSimulatorsMutex);
		auto it = simpleSimulators.find(simHandle);
		if (it != simpleSimulators.end())
			return it->second;

		return nullptr;
	}


	int RemoveAllOptimizationSimulatorsAndAdd(unsigned long int simHandle, Simulators::SimulatorType simType, Simulators::SimulationType simExecType)
	{
		auto sim = GetSimpleSimulator(simHandle);
		if (!sim)
			return 0;

		sim->RemoveAllOptimizationSimulatorsAndAdd(simType, simExecType);

		return 1;
	}

	int AddOptimizationSimulator(unsigned long int simHandle, Simulators::SimulatorType simType, Simulators::SimulationType simExecType)
	{
		auto sim = GetSimpleSimulator(simHandle);
		if (!sim)
			return 0;
		sim->AddOptimizationSimulator(simType, simExecType);
		return 1;
	}

	unsigned long int CreateSimulator(Simulators::SimulatorType simType = Simulators::SimulatorType::kQCSim, Simulators::SimulationType simExecType = Simulators::SimulationType::kMatrixProductState)
	{
		auto simulator = Simulators::SimulatorsFactory::CreateSimulator(simType, simExecType);

		std::lock_guard<std::mutex> lock(simulatorsMutex);
		if (curSimulatorHandle == std::numeric_limits<unsigned long int>::max())
		{
			// Handle overflow, reset to 0
			curSimulatorHandle = 0;
		}
		const unsigned long int handle = ++curSimulatorHandle;
		
		simulators[handle] = simulator;
		
		return handle;
	}

	void *GetSimulator(unsigned long int simHandle)
	{
		std::lock_guard<std::mutex> lock(simulatorsMutex);
		auto it = simulators.find(simHandle);
		if (it != simulators.end())
			return it->second.get();
		
		return nullptr;
	}

	void DestroySimulator(unsigned long int simHandle)
	{
		std::lock_guard<std::mutex> lock(simulatorsMutex);
		simulators.erase(simHandle);
	}

private:
	// allow multithreaded access
	std::mutex simpleSimulatorsMutex;
	std::mutex simulatorsMutex;

	std::unordered_map<unsigned long int, std::shared_ptr<Network::INetwork<>>> simpleSimulators; // map for network simulators
	std::unordered_map<unsigned long int, std::shared_ptr<Simulators::ISimulator>> simulators; // map for simulators
	
	unsigned long int curHandle = 0;
	unsigned long int curSimulatorHandle = 0; // current handle for simulators
};
