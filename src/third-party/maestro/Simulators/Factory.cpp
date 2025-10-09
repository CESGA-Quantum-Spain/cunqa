/**
 * @file Factory.cpp
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Simulator factory implementation
 * 
 * Call CreateSimulator with the desired simulator type to create a simulator returned as a shared pointer.
 * Currently only two simulators are supported: qiskit aer and qcsim.
 * Can be esily extended to support more simulators. Just implement the interface for another simulator, add its type to the enum and add another case to the switch statement.
 */

#define _CRT_SECURE_NO_WARNINGS 1

#ifndef NO_QISKIT_AER
#ifndef __APPLE__
#ifndef _QV_AVX2_IMPL
#define _QV_AVX2_IMPL
#include "simulators/statevector/qv_avx2.cpp"
#endif
#endif
#endif

#include "Factory.h"

#define INCLUDED_BY_FACTORY
#include "AerSimulator.h"
#include "QCSimSimulator.h"
#include "Composite.h"
#include "GpuSimulator.h"

namespace Simulators {

#ifdef __linux__
	std::shared_ptr<GpuLibrary> SimulatorsFactory::gpuLibrary = nullptr;

	bool SimulatorsFactory::InitGpuLibrary()
	{
		if (!gpuLibrary)
		{
			gpuLibrary = std::make_shared<GpuLibrary>();

			if (gpuLibrary->Init("libcomposer_gpu_simulators.so"))
				return true;
			else
				gpuLibrary = nullptr;
		}

		return false;
	}
#endif

	std::shared_ptr<ISimulator> SimulatorsFactory::CreateSimulator(SimulatorType t, SimulationType m)
	{
		switch (t)
		{
		case SimulatorType::kQCSim:
		{
			auto sim = std::make_shared<Private::QCSimSimulator>();
			if (m == SimulationType::kMatrixProductState)
				sim->Configure("method", "matrix_product_state");
			else if (m == SimulationType::kStabilizer)
				sim->Configure("method", "stabilizer");
			else if (m == SimulationType::kTensorNetwork)
				sim->Configure("method", "tensor_network");

			return sim;
		}
#ifndef NO_QISKIT_AER
		case SimulatorType::kQiskitAer:
		{
			auto sim = std::make_shared<Private::AerSimulator>();
			if (m == SimulationType::kMatrixProductState)
				sim->Configure("method", "matrix_product_state");
			else if (m == SimulationType::kStabilizer)
				sim->Configure("method", "stabilizer");
			else if (m == SimulationType::kTensorNetwork)
				sim->Configure("method", "tensor_network");
			else
				sim->Configure("method", "statevector");

			return sim;
		}
		case SimulatorType::kCompositeQiskitAer:
			return std::make_shared<Private::CompositeSimulator>(SimulatorType::kQiskitAer);
#endif
		case SimulatorType::kCompositeQCSim:
			return std::make_shared<Private::CompositeSimulator>(SimulatorType::kQCSim);
#ifdef __linux__			
		case SimulatorType::kGpuSim:
			if (gpuLibrary && gpuLibrary->IsValid() && (m == SimulationType::kStatevector || m == SimulationType::kMatrixProductState))
			{
				auto sim = std::make_shared<Private::GpuSimulator>();
				if (m == SimulationType::kMatrixProductState)
					sim->Configure("method", "matrix_product_state");

				return sim;
			}

			return nullptr;
#endif
		default:
			break;
		}

		throw std::invalid_argument("Simulator Type not supported");

		return nullptr; // keep compillers happy
	}

	std::unique_ptr<ISimulator> SimulatorsFactory::CreateSimulatorUnique(SimulatorType t, SimulationType m)
	{
		switch (t)
		{
		case SimulatorType::kQCSim:
		{
			auto sim = std::make_unique<Private::QCSimSimulator>();
			if (m == SimulationType::kMatrixProductState)
				sim->Configure("method", "matrix_product_state");
			else if (m == SimulationType::kStabilizer)
				sim->Configure("method", "stabilizer");
			else if (m == SimulationType::kTensorNetwork)
				sim->Configure("method", "tensor_network");

			return sim;
		}
#ifndef NO_QISKIT_AER
		case SimulatorType::kQiskitAer:
		{
			auto sim = std::make_unique<Private::AerSimulator>();
			if (m == SimulationType::kMatrixProductState)
				sim->Configure("method", "matrix_product_state");
			else if (m == SimulationType::kStabilizer)
				sim->Configure("method", "stabilizer");
			else if (m == SimulationType::kTensorNetwork)
				sim->Configure("method", "tensor_network");
			else
				sim->Configure("method", "statevector");

			return sim;
		}
		case SimulatorType::kCompositeQiskitAer:
			return std::make_unique<Private::CompositeSimulator>(SimulatorType::kQiskitAer);
#endif
		case SimulatorType::kCompositeQCSim:
			return std::make_unique<Private::CompositeSimulator>(SimulatorType::kQCSim);
#ifdef __linux__
		case SimulatorType::kGpuSim:
			if (gpuLibrary && gpuLibrary->IsValid() && (m == SimulationType::kStatevector || m == SimulationType::kMatrixProductState))
			{
				auto sim = std::make_unique<Private::GpuSimulator>();
				if (m == SimulationType::kMatrixProductState)
					sim->Configure("method", "matrix_product_state");

				return sim;
			}

			return nullptr;
#endif
		default:
			break;
		}

		throw std::invalid_argument("Simulator Type not supported");

		return nullptr; // keep compillers happy
	}
}
