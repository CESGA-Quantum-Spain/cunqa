#pragma once

#include "MaestroLib.hpp"

class SimpleSimulator : protected MaestroLibrary
{
public:
	SimpleSimulator() noexcept
	{
	}

	virtual ~SimpleSimulator()
	{
		if (handle)
			DestroySimpleSimulator(handle);
	}

	bool Init(const char* libName) noexcept override
	{
		if (MaestroLibrary::Init(libName))
			return true;

		return false;
	}

	unsigned long int CreateSimpleSimulator(int nrQubits) override
	{
		if (handle)
			DestroySimpleSimulator(handle);

		handle = MaestroLibrary::CreateSimpleSimulator(nrQubits);

		return handle;
	}

	bool RemoveAllOptimizationSimulatorsAndAdd(int simType, int simExecType)
	{
		if (handle)
			return MaestroLibrary::RemoveAllOptimizationSimulatorsAndAdd(handle, simType, simExecType) == 1;
		
		return false;
	}

	bool AddOptimizationSimulator(int simType, int simExecType)
	{
		if (handle)
			return MaestroLibrary::AddOptimizationSimulator(handle, simType, simExecType) == 1;
		
		return false;
	}

	char* SimpleExecute(const char* jsonCircuit, const char* jsonConfig)
	{
		return MaestroLibrary::SimpleExecute(handle, jsonCircuit, jsonConfig);
	}

	void FreeResult(char* result) override
	{
		MaestroLibrary::FreeResult(result);
	}

private:
	unsigned long int handle = 0;
};

class Simulator : protected MaestroLibrary
{
public:
	Simulator() noexcept
	{
	}

	virtual ~Simulator()
	{
		if (handle)
			DestroySimulator(handle);
	}

	bool Init(const char* libName) noexcept override
	{
		if (MaestroLibrary::Init(libName))
			return true;
		
		return false;
	}

	unsigned long int CreateSimulator(int simType, int simExecType) override
	{
		if (handle)
			DestroySimulator(handle);

		handle = MaestroLibrary::CreateSimulator(simType, simExecType);
		
		if (handle)
			simulatorPtr = MaestroLibrary::GetSimulator(handle);
		
		return handle;
	}

	void* GetSimulator()
	{
		return MaestroLibrary::GetSimulator(handle);
	}
	
	void FreeResult(char* result)
	{
		MaestroLibrary::FreeResult(result);
	}

	int InitializeSimulator()
	{
		if (simulatorPtr) return MaestroLibrary::InitializeSimulator(simulatorPtr);
		return 0;
	}

	int ResetSimulator()
	{
		if (simulatorPtr) return MaestroLibrary::ResetSimulator(simulatorPtr);
		return 0;
	}

	int ConfigureSimulator(const char* key, const char* value)
	{
		if (simulatorPtr) return MaestroLibrary::ConfigureSimulator(simulatorPtr, key, value);
		return 0;
	}

	char* GetConfiguration(const char* key)
	{
		if (simulatorPtr) return MaestroLibrary::GetConfiguration(simulatorPtr, key);
		return nullptr;
	}

	unsigned long int AllocateQubits(unsigned long int nrQubits)
	{
		if (simulatorPtr) return MaestroLibrary::AllocateQubits(simulatorPtr, nrQubits);
		return 0;
	}

	unsigned long int GetNumberOfQubits()
	{
		if (simulatorPtr) return MaestroLibrary::GetNumberOfQubits(simulatorPtr);
		return 0;
	}

	int ClearSimulator()
	{
		if (simulatorPtr) return MaestroLibrary::ClearSimulator(simulatorPtr);
		return 0;
	}

	unsigned long long int Measure(const unsigned long int* qubits, unsigned long int nrQubits)
	{
		if (simulatorPtr) return MaestroLibrary::Measure(simulatorPtr, qubits, nrQubits);

		return 0;
	}

	int ApplyReset(const unsigned long int* qubits, unsigned long int nrQubits)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyReset(simulatorPtr, qubits, nrQubits);
		return 0;
	}

	double Probability(unsigned long long int outcome)
	{
		if (simulatorPtr) return MaestroLibrary::Probability(simulatorPtr, outcome);

		return 0.0;
	}

	void FreeDoubleVector(double* vec) override
	{
		MaestroLibrary::FreeDoubleVector(vec);
	}

	void FreeULLIVector(unsigned long long int* vec) override
	{
		MaestroLibrary::FreeULLIVector(vec);
	}

	double* Amplitude(unsigned long long int outcome)
	{
		if (simulatorPtr) return MaestroLibrary::Amplitude(simulatorPtr, outcome);
		return nullptr;
	}

	double* AllProbabilities()
	{
		if (simulatorPtr) return MaestroLibrary::AllProbabilities(simulatorPtr);
		return nullptr;
	}

	double* Probabilities(const unsigned long long int* qubits, unsigned long int nrQubits)
	{
		if (simulatorPtr) return MaestroLibrary::Probabilities(simulatorPtr, qubits, nrQubits);
		return nullptr;
	}

	unsigned long long int* SampleCounts(const unsigned long long int* qubits, unsigned long int nrQubits, unsigned long int shots)
	{
		if (simulatorPtr) return MaestroLibrary::SampleCounts(simulatorPtr, qubits, nrQubits, shots);
		return nullptr;
	}

	int GetSimulatorType()
	{
		if (simulatorPtr) return MaestroLibrary::GetSimulatorType(simulatorPtr);
		return -1;
	}

	int GetSimulationType()
	{
		if (simulatorPtr) return MaestroLibrary::GetSimulationType(simulatorPtr);
		return -1;
	}

	int FlushSimulator()
	{
		if (simulatorPtr) return MaestroLibrary::FlushSimulator(simulatorPtr);
		return 0;
	}

	int SaveStateToInternalDestructive()
	{
		if (simulatorPtr) return MaestroLibrary::SaveStateToInternalDestructive(simulatorPtr);
		return 0;
	}

	int RestoreInternalDestructiveSavedState()
	{
		if (simulatorPtr) return MaestroLibrary::RestoreInternalDestructiveSavedState(simulatorPtr);
		return 0;
	}

	int SaveState()
	{
		if (simulatorPtr) return MaestroLibrary::SaveState(simulatorPtr);
		return 0;
	}

	int RestoreState()
	{
		if (simulatorPtr) return MaestroLibrary::RestoreState(simulatorPtr);
		return 0;
	}

	int SetMultithreading(int multithreading)
	{
		if (simulatorPtr) return MaestroLibrary::SetMultithreading(simulatorPtr, multithreading);
		return 0;
	}

	int GetMultithreading()
	{
		if (simulatorPtr) return MaestroLibrary::GetMultithreading(simulatorPtr);
		return 0;
	}

	int IsQcsim()
	{
		if (simulatorPtr) return MaestroLibrary::IsQcsim(simulatorPtr);
		return 0;
	}

	unsigned long long int MeasureNoCollapse()
	{
		if (simulatorPtr) return MaestroLibrary::MeasureNoCollapse(simulatorPtr);
		return 0;
	}

	int ApplyX(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyX(simulatorPtr, qubit);
		return 0;
	}

	int ApplyY(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyY(simulatorPtr, qubit);
		return 0;
	}

	int ApplyZ(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyZ(simulatorPtr, qubit);
		return 0;
	}

	int ApplyH(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyH(simulatorPtr, qubit);
		return 0;
	}

	int ApplyS(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyS(simulatorPtr, qubit);
		return 0;
	}

	int ApplySDG(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplySDG(simulatorPtr, qubit);
		return 0;
	}

	int ApplyT(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyT(simulatorPtr, qubit);
		return 0;
	}

	int ApplyTDG(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyTDG(simulatorPtr, qubit);
		return 0;
	}

	int ApplySX(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplySX(simulatorPtr, qubit);
		return 0;
	}

	int ApplySXDG(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplySXDG(simulatorPtr, qubit);
		return 0;
	}

	int ApplyK(int qubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyK(simulatorPtr, qubit);
		return 0;
	}

	int ApplyP(int qubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyP(simulatorPtr, qubit, theta);
		return 0;
	}

	int ApplyRx(int qubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyRx(simulatorPtr, qubit, theta);
		return 0;
	}

	int ApplyRy(int qubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyRy(simulatorPtr, qubit, theta);
		return 0;
	}

	int ApplyRz(int qubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyRz(simulatorPtr, qubit, theta);
		return 0;
	}

	int ApplyU(int qubit, double theta, double phi, double lambda, double gamma)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyU(simulatorPtr, qubit, theta, phi, lambda, gamma);
		return 0;
	}

	int ApplyCX(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCX(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCY(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCY(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCZ(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCZ(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCH(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCH(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCSX(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCSX(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCSXDG(int controlQubit, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCSXDG(simulatorPtr, controlQubit, targetQubit);
		return 0;
	}

	int ApplyCP(int controlQubit, int targetQubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCP(simulatorPtr, controlQubit, targetQubit, theta);
		return 0;
	}

	int ApplyCRx(int controlQubit, int targetQubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCRx(simulatorPtr, controlQubit, targetQubit, theta);
		return 0;
	}

	int ApplyCRy(int controlQubit, int targetQubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCRy(simulatorPtr, controlQubit, targetQubit, theta);
		return 0;
	}

	int ApplyCRz(int controlQubit, int targetQubit, double theta)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCRz(simulatorPtr, controlQubit, targetQubit, theta);
		return 0;
	}

	int ApplyCCX(int controlQubit1, int controlQubit2, int targetQubit)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCCX(simulatorPtr, controlQubit1, controlQubit2, targetQubit);
		return 0;
	}

	int ApplySwap(int qubit1, int qubit2)
	{
		if (simulatorPtr) return MaestroLibrary::ApplySwap(simulatorPtr, qubit1, qubit2);
		return 0;
	}

	int ApplyCSwap(int controlQubit, int qubit1, int qubit2)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCSwap(simulatorPtr, controlQubit, qubit1, qubit2);
		return 0;
	}

	int ApplyCU(int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma)
	{
		if (simulatorPtr) return MaestroLibrary::ApplyCU(simulatorPtr, controlQubit, targetQubit, theta, phi, lambda, gamma);
		return 0;
	}

private:
	unsigned long int handle = 0;
	void* simulatorPtr = nullptr;
};