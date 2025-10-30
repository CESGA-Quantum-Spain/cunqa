#pragma once

#include "Library.h"

class MaestroLibrary : public Utils::Library
{
public:
	MaestroLibrary(const MaestroLibrary&) = delete;
	MaestroLibrary& operator=(const MaestroLibrary&) = delete;

	MaestroLibrary(MaestroLibrary&&) = default;
	MaestroLibrary& operator=(MaestroLibrary&&) = default;

	MaestroLibrary() noexcept
	{
	}

	virtual ~MaestroLibrary()
	{
	}

	bool Init(const char* libName) noexcept override
	{
		if (Utils::Library::Init(libName))
		{
			fGetMaestroObject = (void* (*)())GetFunction("GetMaestroObject");
			CheckFunction((void*)fGetMaestroObject, __LINE__);
			if (fGetMaestroObject)
			{
				maestro = fGetMaestroObject();
				if (maestro)
				{
					fCreateSimpleSimulator = (unsigned long int (*)(int))GetFunction("CreateSimpleSimulator");
					CheckFunction((void*)fCreateSimpleSimulator, __LINE__);
					fDestroySimpleSimulator = (void (*)(unsigned long int))GetFunction("DestroySimpleSimulator");
					CheckFunction((void*)fDestroySimpleSimulator, __LINE__);
					
					fRemoveAllOptimizationSimulatorsAndAdd = (int (*)(unsigned long int simHandle, int simType, int simExecType))GetFunction("RemoveAllOptimizationSimulatorsAndAdd");
					CheckFunction((void*)fRemoveAllOptimizationSimulatorsAndAdd, __LINE__);
					fAddOptimizationSimulator = (int (*)(unsigned long int simHandle, int simType, int simExecType))GetFunction("AddOptimizationSimulator");
					CheckFunction((void*)fAddOptimizationSimulator, __LINE__);
					
					fSimpleExecute = (char* (*)(unsigned long int, const char*, const char*))GetFunction("SimpleExecute");
					CheckFunction((void*)fSimpleExecute, __LINE__);
					fFreeResult = (void (*)(char*))GetFunction("FreeResult");
					CheckFunction((void*)fFreeResult, __LINE__);


					fCreateSimulator = (unsigned long int (*)(int, int))GetFunction("CreateSimulator");
					CheckFunction((void*)fCreateSimulator, __LINE__);
					fGetSimulator = (void* (*)(unsigned long int))GetFunction("GetSimulator");
					CheckFunction((void*)fGetSimulator, __LINE__);
					fDestroySimulator = (void (*)(unsigned long int))GetFunction("DestroySimulator");
					CheckFunction((void*)fDestroySimulator, __LINE__);

					fInitializeSimulator = (int (*)(void*))GetFunction("InitializeSimulator");
					CheckFunction((void*)fInitializeSimulator, __LINE__);
					fResetSimulator = (int (*)(void*))GetFunction("ResetSimulator");
					CheckFunction((void*)fResetSimulator, __LINE__);
					fConfigureSimulator = (int (*)(void*, const char*, const char*))GetFunction("ConfigureSimulator");
					CheckFunction((void*)fConfigureSimulator, __LINE__);
					fGetConfiguration = (char* (*)(void*, const char*))GetFunction("GetConfiguration");
					CheckFunction((void*)fGetConfiguration, __LINE__);
					fAllocateQubits = (unsigned long int (*)(void*, unsigned long int))GetFunction("AllocateQubits");
					CheckFunction((void*)fAllocateQubits, __LINE__);
					fGetNumberOfQubits = (unsigned long int (*)(void*))GetFunction("GetNumberOfQubits");
					CheckFunction((void*)fGetNumberOfQubits, __LINE__);
					fClearSimulator = (int (*)(void*))GetFunction("ClearSimulator");
					CheckFunction((void*)fClearSimulator, __LINE__);
					fMeasure = (unsigned long long int (*)(void*, const unsigned long int*, unsigned long int))GetFunction("Measure");
					CheckFunction((void*)fMeasure, __LINE__);
					fApplyReset = (int (*)(void*, const unsigned long int*, unsigned long int))GetFunction("ApplyReset");
					CheckFunction((void*)fApplyReset, __LINE__);
					fProbability = (double (*)(void*, unsigned long long int))GetFunction("Probability");
					CheckFunction((void*)fProbability, __LINE__);
					fFreeDoubleVector = (void (*)(double*))GetFunction("FreeDoubleVector");
					CheckFunction((void*)fFreeDoubleVector, __LINE__);
					fFreeULLIVector = (void (*)(unsigned long long int*))GetFunction("FreeULLIVector");
					CheckFunction((void*)fFreeULLIVector, __LINE__);
					fAmplitude = (double* (*)(void*, unsigned long long int))GetFunction("Amplitude");
					CheckFunction((void*)fAmplitude, __LINE__);
					fAllProbabilities = (double* (*)(void*))GetFunction("AllProbabilities");
					CheckFunction((void*)fAllProbabilities, __LINE__);
					fProbabilities = (double* (*)(void*, const unsigned long long int*, unsigned long int))GetFunction("Probabilities");
					CheckFunction((void*)fProbabilities, __LINE__);
					fSampleCounts = (unsigned long long int* (*)(void*, const unsigned long long int*, unsigned long int, unsigned long int))GetFunction("SampleCounts");
					CheckFunction((void*)fSampleCounts, __LINE__);
					fGetSimulatorType = (int (*)(void*))GetFunction("GetSimulatorType");
					CheckFunction((void*)fGetSimulatorType, __LINE__);
					fGetSimulationType = (int (*)(void*))GetFunction("GetSimulationType");
					CheckFunction((void*)fGetSimulationType, __LINE__);
					fFlushSimulator = (int (*)(void*))GetFunction("FlushSimulator");
					CheckFunction((void*)fFlushSimulator, __LINE__);
					fSaveStateToInternalDestructive = (int (*)(void*))GetFunction("SaveStateToInternalDestructive");
					CheckFunction((void*)fSaveStateToInternalDestructive, __LINE__);
					fRestoreInternalDestructiveSavedState = (int (*)(void*))GetFunction("RestoreInternalDestructiveSavedState");
					CheckFunction((void*)fRestoreInternalDestructiveSavedState, __LINE__);
					fSaveState = (int (*)(void*))GetFunction("SaveState");
					CheckFunction((void*)fSaveState, __LINE__);
					fRestoreState = (int (*)(void*))GetFunction("RestoreState");
					CheckFunction((void*)fRestoreState, __LINE__);
					fSetMultithreading = (int (*)(void*, int))GetFunction("SetMultithreading");
					CheckFunction((void*)fSetMultithreading, __LINE__);
					fGetMultithreading = (int (*)(void*))GetFunction("GetMultithreading");
					CheckFunction((void*)fGetMultithreading, __LINE__);
					fIsQcsim = (int (*)(void*))GetFunction("IsQcsim");
					CheckFunction((void*)fIsQcsim, __LINE__);
					fMeasureNoCollapse = (unsigned long long int (*)(void*))GetFunction("MeasureNoCollapse");
					CheckFunction((void*)fMeasureNoCollapse, __LINE__);

					fApplyX = (int (*)(void*, int))GetFunction("ApplyX");
					CheckFunction((void*)fApplyX, __LINE__);
					fApplyY = (int (*)(void*, int))GetFunction("ApplyY");
					CheckFunction((void*)fApplyY, __LINE__);
					fApplyZ = (int (*)(void*, int))GetFunction("ApplyZ");
					CheckFunction((void*)fApplyZ, __LINE__);
					fApplyH = (int (*)(void*, int))GetFunction("ApplyH");
					CheckFunction((void*)fApplyH, __LINE__);
					fApplyS = (int (*)(void*, int))GetFunction("ApplyS");
					CheckFunction((void*)fApplyS, __LINE__);
					fApplySDG = (int (*)(void*, int))GetFunction("ApplySDG");
					CheckFunction((void*)fApplySDG, __LINE__);
					fApplyT = (int (*)(void*, int))GetFunction("ApplyT");
					CheckFunction((void*)fApplyT, __LINE__);
					fApplyTDG = (int (*)(void*, int))GetFunction("ApplyTDG");
					CheckFunction((void*)fApplyTDG, __LINE__);
					fApplySX = (int (*)(void*, int))GetFunction("ApplySX");
					CheckFunction((void*)fApplySX, __LINE__);
					fApplySXDG = (int (*)(void*, int))GetFunction("ApplySXDG");
					CheckFunction((void*)fApplySXDG, __LINE__);
					fApplyK = (int (*)(void*, int))GetFunction("ApplyK");
					CheckFunction((void*)fApplyK, __LINE__);
					fApplyP = (int (*)(void*, int, double))GetFunction("ApplyP");
					CheckFunction((void*)fApplyP, __LINE__);
					fApplyRx = (int (*)(void*, int, double))GetFunction("ApplyRx");
					CheckFunction((void*)fApplyRx, __LINE__);
					fApplyRy = (int (*)(void*, int, double))GetFunction("ApplyRy");
					CheckFunction((void*)fApplyRy, __LINE__);
					fApplyRz = (int (*)(void*, int, double))GetFunction("ApplyRz");
					CheckFunction((void*)fApplyRz, __LINE__);
					fApplyU = (int (*)(void*, int, double, double, double, double))GetFunction("ApplyU");
					CheckFunction((void*)fApplyU, __LINE__);
					fApplyCX = (int (*)(void*, int, int))GetFunction("ApplyCX");
					CheckFunction((void*)fApplyCX, __LINE__);
					fApplyCY = (int (*)(void*, int, int))GetFunction("ApplyCY");
					CheckFunction((void*)fApplyCY, __LINE__);
					fApplyCZ = (int (*)(void*, int, int))GetFunction("ApplyCZ");
					CheckFunction((void*)fApplyCZ, __LINE__);
					fApplyCH = (int (*)(void*, int, int))GetFunction("ApplyCH");
					CheckFunction((void*)fApplyCH, __LINE__);
					fApplyCSX = (int (*)(void*, int, int))GetFunction("ApplyCSX");
					CheckFunction((void*)fApplyCSX, __LINE__);
					fApplyCSXDG = (int (*)(void*, int, int))GetFunction("ApplyCSXDG");
					CheckFunction((void*)fApplyCSXDG, __LINE__);
					fApplyCP = (int (*)(void*, int, int, double))GetFunction("ApplyCP");
					CheckFunction((void*)fApplyCP, __LINE__);
					fApplyCRx = (int (*)(void*, int, int, double))GetFunction("ApplyCRx");
					CheckFunction((void*)fApplyCRx, __LINE__);
					fApplyCRy = (int (*)(void*, int, int, double))GetFunction("ApplyCRy");
					CheckFunction((void*)fApplyCRy, __LINE__);
					fApplyCRz = (int (*)(void*, int, int, double))GetFunction("ApplyCRz");
					CheckFunction((void*)fApplyCRz, __LINE__);
					fApplyCCX = (int (*)(void*, int, int, int))GetFunction("ApplyCCX");
					CheckFunction((void*)fApplyCCX, __LINE__);
					fApplySwap = (int (*)(void*, int, int))GetFunction("ApplySwap");
					CheckFunction((void*)fApplySwap, __LINE__);
					fApplyCSwap = (int (*)(void*, int, int, int))GetFunction("ApplyCSwap");
					CheckFunction((void*)fApplyCSwap, __LINE__);
					fApplyCU = (int (*)(void*, int, int, double, double, double, double))GetFunction("ApplyCU");
					CheckFunction((void*)fApplyCU, __LINE__);

					return true;
				}
			}
			else std::cout << "MaestroLibrary: Unable to get initialization function for library" << std::endl;
		}
		else std::cout << "MaestroLibrary: Unable to load the library" << std::endl;

		return false;
	}

	static void CheckFunction(void* func, int line)
	{
		if (!func)
		{
			std::cout << "MaestroLib: Unable to load function, line #: " << std::to_string(line);
			const char* dlsym_error = dlerror();
			if (dlsym_error)
				std::cout << ", error: " << dlsym_error;

			std::cout << std::endl;
		}
	}

	bool IsValid() const
	{
		return maestro != nullptr;
	}

	virtual unsigned long int CreateSimpleSimulator(int nrQubits)
	{
		if (maestro && fCreateSimpleSimulator)
			return fCreateSimpleSimulator(nrQubits);
		else
			throw std::runtime_error("MaestroLibrary: Unable to create the simple simulator.");

		return 0;
	}

	void DestroySimpleSimulator(unsigned long int simHandle)
	{
		if (maestro && fDestroySimpleSimulator)
			fDestroySimpleSimulator(simHandle);
		else
			throw std::runtime_error("MaestroLibrary: Unable to destroy the simple simulator.");
	}

	int RemoveAllOptimizationSimulatorsAndAdd(unsigned long int simHandle, int simType, int simExecType)
	{
		if (maestro && fRemoveAllOptimizationSimulatorsAndAdd)
			return fRemoveAllOptimizationSimulatorsAndAdd(simHandle, simType, simExecType);
		else
			throw std::runtime_error("MaestroLibrary: Unable to remove all optimization simulators and add a new one.");

		return 0;
	}

	int AddOptimizationSimulator(unsigned long int simHandle, int simType, int simExecType)
	{
		if (maestro && fAddOptimizationSimulator)
			return fAddOptimizationSimulator(simHandle, simType, simExecType);
		else
			throw std::runtime_error("MaestroLibrary: Unable to add an optimization simulator.");

		return 0;
	}

	char* SimpleExecute(unsigned long int simpleSim, const char* jsonCircuit, const char* jsonConfig)
	{
		if (maestro && fSimpleExecute)
			return fSimpleExecute(simpleSim, jsonCircuit, jsonConfig);
		else
			throw std::runtime_error("MaestroLibrary: Unable to execute the simple simulator.");

		return nullptr;
	}

	virtual void FreeResult(char* result)
	{
		if (maestro && fFreeResult)
			fFreeResult(result);
		else
			throw std::runtime_error("MaestroLibrary: Unable to free the result.");
	}

	virtual unsigned long int CreateSimulator(int simType, int simExecType)
	{
		if (maestro && fCreateSimulator)
			return fCreateSimulator(simType, simExecType);
		else
			throw std::runtime_error("MaestroLibrary: Unable to create the simulator.");

		return 0;
	}

	void* GetSimulator(unsigned long int simHandle)
	{
		if (maestro && fGetSimulator)
			return fGetSimulator(simHandle);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the simulator.");
	}

	void DestroySimulator(unsigned long int simHandle)
	{
		if (maestro && fDestroySimulator)
			fDestroySimulator(simHandle);
		else
			throw std::runtime_error("MaestroLibrary: Unable to destroy the simulator.");
	}

	int InitializeSimulator(void* sim)
	{
		if (maestro && sim && fInitializeSimulator)
			return fInitializeSimulator(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to initialize the simulator.");

		return 0;
	}

	int ResetSimulator(void* sim)
	{
		if (maestro && sim && fResetSimulator)
			return fResetSimulator(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to reset the simulator.");

		return 0;
	}

	int ConfigureSimulator(void* sim, const char* key, const char* value)
	{
		if (maestro && sim && fConfigureSimulator)
			return fConfigureSimulator(sim, key, value);
		else
			throw std::runtime_error("MaestroLibrary: Unable to configure the simulator.");

		return 0;
	}

	char* GetConfiguration(void* sim, const char* key)
	{
		if (maestro && sim && fGetConfiguration)
			return fGetConfiguration(sim, key);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the configuration of the simulator.");
		return nullptr;
	}
	
	unsigned long int AllocateQubits(void* sim, unsigned long int nrQubits)
	{
		if (maestro && sim && fAllocateQubits)
			return fAllocateQubits(sim, nrQubits);
		else
			throw std::runtime_error("MaestroLibrary: Unable to allocate qubits in the simulator.");
		return 0;
	}

	unsigned long int GetNumberOfQubits(void* sim)
	{
		if (maestro && sim && fGetNumberOfQubits)
			return fGetNumberOfQubits(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the number of qubits in the simulator.");
		return 0;
	}

	int ClearSimulator(void* sim)
	{
		if (maestro && sim && fClearSimulator)
			return fClearSimulator(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to clear the simulator.");
		return 0;
	}

	unsigned long long int Measure(void* sim, const unsigned long int* qubits, unsigned long int nrQubits)
	{
		if (maestro && sim && fMeasure)
			return fMeasure(sim, qubits, nrQubits);
		else
			throw std::runtime_error("MaestroLibrary: Unable to measure the simulator.");
		return 0;
	}

	int ApplyReset(void* sim, const unsigned long int* qubits, unsigned long int nrQubits)
	{
		if (maestro && sim && fApplyReset)
			return fApplyReset(sim, qubits, nrQubits);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply reset to the simulator.");
		return 0;
	}

	double Probability(void* sim, unsigned long long int outcome)
	{
		if (maestro && sim && fProbability)
			return fProbability(sim, outcome);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the probability of an outcome.");
		return 0.0;
	}

	virtual void FreeDoubleVector(double* vec)
	{
		if (maestro && fFreeDoubleVector)
			fFreeDoubleVector(vec);
		else
			throw std::runtime_error("MaestroLibrary: Unable to free the double vector.");
	}

	virtual void FreeULLIVector(unsigned long long int* vec)
	{
		if (maestro && fFreeULLIVector)
			fFreeULLIVector(vec);
		else
			throw std::runtime_error("MaestroLibrary: Unable to free the unsigned long long int vector.");
	}

	double* Amplitude(void* sim, unsigned long long int outcome)
	{
		if (maestro && sim && fAmplitude)
			return fAmplitude(sim, outcome);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the amplitude of an outcome.");
		return nullptr;
	}

	double* AllProbabilities(void* sim)
	{
		if (maestro && sim && fAllProbabilities)
			return fAllProbabilities(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get all probabilities.");
		return nullptr;
	}

	double* Probabilities(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits)
	{
		if (maestro && sim && fProbabilities)
			return fProbabilities(sim, qubits, nrQubits);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get probabilities for specified qubits.");
		return nullptr;
	}

	unsigned long long int* SampleCounts(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits, unsigned long int shots)
	{
		if (maestro && sim && fSampleCounts)
			return fSampleCounts(sim, qubits, nrQubits, shots);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get sample counts for specified qubits.");
		return nullptr;
	}

	int GetSimulatorType(void* sim)
	{
		if (maestro && sim && fGetSimulatorType)
			return fGetSimulatorType(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the simulator type.");
		return -1;
	}

	int GetSimulationType(void* sim)
	{
		if (maestro && sim && fGetSimulationType)
			return fGetSimulationType(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get the simulation type.");
		return -1;
	}

	int FlushSimulator(void* sim)
	{
		if (maestro && sim && fFlushSimulator)
			return fFlushSimulator(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to flush the simulator.");
		return 0;
	}

	int SaveStateToInternalDestructive(void* sim)
	{
		if (maestro && sim && fSaveStateToInternalDestructive)
			return fSaveStateToInternalDestructive(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to save the state to internal destructive storage.");
		return 0;
	}

	int RestoreInternalDestructiveSavedState(void* sim)
	{
		if (maestro && sim && fRestoreInternalDestructiveSavedState)
			return fRestoreInternalDestructiveSavedState(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to restore the state from internal destructive storage.");
		return 0;
	}

	int SaveState(void* sim)
	{
		if (maestro && sim && fSaveState)
			return fSaveState(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to save the state.");
		return 0;
	}

	int RestoreState(void* sim)
	{
		if (maestro && sim && fRestoreState)
			return fRestoreState(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to restore the state.");
		return 0;
	}

	int SetMultithreading(void* sim, int multithreading)
	{
		if (maestro && sim && fSetMultithreading)
			return fSetMultithreading(sim, multithreading);
		else
			throw std::runtime_error("MaestroLibrary: Unable to set multithreading.");
		return 0;
	}

	int GetMultithreading(void* sim)
	{
		if (maestro && sim && fGetMultithreading)
			return fGetMultithreading(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to get multithreading status.");
		return 0;
	}

	int IsQcsim(void* sim)
	{
		if (maestro && sim && fIsQcsim)
			return fIsQcsim(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to check if the simulator is a QCSIM.");
		return 0;
	}

	unsigned long long int MeasureNoCollapse(void* sim)
	{
		if (maestro && sim && fMeasureNoCollapse)
			return fMeasureNoCollapse(sim);
		else
			throw std::runtime_error("MaestroLibrary: Unable to measure without collapse.");
		return 0;
	}

	int ApplyX(void* sim, int qubit)
	{
		if (maestro && sim && fApplyX)
			return fApplyX(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply X gate.");
		return 0;
	}

	int ApplyY(void* sim, int qubit)
	{
		if (maestro && sim && fApplyY)
			return fApplyY(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Y gate.");
		return 0;
	}

	int ApplyZ(void* sim, int qubit)
	{
		if (maestro && sim && fApplyZ)
			return fApplyZ(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Z gate.");
		return 0;
	}

	int ApplyH(void* sim, int qubit)
	{
		if (maestro && sim && fApplyH)
			return fApplyH(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply H gate.");
		return 0;
	}

	int ApplyS(void* sim, int qubit)
	{
		if (maestro && sim && fApplyS)
			return fApplyS(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply S gate.");
		return 0;
	}

	int ApplySDG(void* sim, int qubit)
	{
		if (maestro && sim && fApplySDG)
			return fApplySDG(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply SDG gate.");
		return 0;
	}

	int ApplyT(void* sim, int qubit)
	{
		if (maestro && sim && fApplyT)
			return fApplyT(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply T gate.");
		return 0;
	}

	int ApplyTDG(void* sim, int qubit)
	{
		if (maestro && sim && fApplyTDG)
			return fApplyTDG(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply TDG gate.");
		return 0;
	}

	int ApplySX(void* sim, int qubit)
	{
		if (maestro && sim && fApplySX)
			return fApplySX(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply SX gate.");
		return 0;
	}

	int ApplySXDG(void* sim, int qubit)
	{
		if (maestro && sim && fApplySXDG)
			return fApplySXDG(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply SXDG gate.");
		return 0;
	}

	int ApplyK(void* sim, int qubit)
	{
		if (maestro && sim && fApplyK)
			return fApplyK(sim, qubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply K gate.");
		return 0;
	}

	int ApplyP(void* sim, int qubit, double theta)
	{
		if (maestro && sim && fApplyP)
			return fApplyP(sim, qubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply P gate.");
		return 0;
	}

	int ApplyRx(void* sim, int qubit, double theta)
	{
		if (maestro && sim && fApplyRx)
			return fApplyRx(sim, qubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Rx gate.");
		return 0;
	}

	int ApplyRy(void* sim, int qubit, double theta)
	{
		if (maestro && sim && fApplyRy)
			return fApplyRy(sim, qubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Ry gate.");
		return 0;
	}

	int ApplyRz(void* sim, int qubit, double theta)
	{
		if (maestro && sim && fApplyRz)
			return fApplyRz(sim, qubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Rz gate.");
		return 0;
	}

	int ApplyU(void* sim, int qubit, double theta, double phi, double lambda, double gamma)
	{
		if (maestro && sim && fApplyU)
			return fApplyU(sim, qubit, theta, phi, lambda, gamma);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply U gate.");
		return 0;
	}

	int ApplyCX(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCX)
			return fApplyCX(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CX gate.");
		return 0;
	}

	int ApplyCY(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCY)
			return fApplyCY(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CY gate.");
		return 0;
	}

	int ApplyCZ(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCZ)
			return fApplyCZ(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CZ gate.");
		return 0;
	}

	int ApplyCH(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCH)
			return fApplyCH(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CH gate.");
		return 0;
	}

	int ApplyCSX(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCSX)
			return fApplyCSX(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CSX gate.");
		return 0;
	}

	int ApplyCSXDG(void* sim, int controlQubit, int targetQubit)
	{
		if (maestro && sim && fApplyCSXDG)
			return fApplyCSXDG(sim, controlQubit, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CSXDG gate.");
		return 0;
	}

	int ApplyCP(void* sim, int controlQubit, int targetQubit, double theta)
	{
		if (maestro && sim && fApplyCP)
			return fApplyCP(sim, controlQubit, targetQubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CP gate.");
		return 0;
	}

	int ApplyCRx(void* sim, int controlQubit, int targetQubit, double theta)
	{
		if (maestro && sim && fApplyCRx)
			return fApplyCRx(sim, controlQubit, targetQubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CRx gate.");
		return 0;
	}

	int ApplyCRy(void* sim, int controlQubit, int targetQubit, double theta)
	{
		if (maestro && sim && fApplyCRy)
			return fApplyCRy(sim, controlQubit, targetQubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CRy gate.");
		return 0;
	}

	int ApplyCRz(void* sim, int controlQubit, int targetQubit, double theta)
	{
		if (maestro && sim && fApplyCRz)
			return fApplyCRz(sim, controlQubit, targetQubit, theta);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CRz gate.");
		return 0;
	}

	int ApplyCCX(void* sim, int controlQubit1, int controlQubit2, int targetQubit)
	{
		if (maestro && sim && fApplyCCX)
			return fApplyCCX(sim, controlQubit1, controlQubit2, targetQubit);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CCX gate.");
		return 0;
	}

	int ApplySwap(void* sim, int qubit1, int qubit2)
	{
		if (maestro && sim && fApplySwap)
			return fApplySwap(sim, qubit1, qubit2);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply Swap gate.");
		return 0;
	}

	int ApplyCSwap(void* sim, int controlQubit, int qubit1, int qubit2)
	{
		if (maestro && sim && fApplyCSwap)
			return fApplyCSwap(sim, controlQubit, qubit1, qubit2);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CSwap gate.");
		return 0;
	}

	int ApplyCU(void* sim, int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma)
	{
		if (maestro && sim && fApplyCU)
			return fApplyCU(sim, controlQubit, targetQubit, theta, phi, lambda, gamma);
		else
			throw std::runtime_error("MaestroLibrary: Unable to apply CU gate.");
		return 0;
	}

private:
	void* maestro = nullptr;

	void* (*fGetMaestroObject)();

	unsigned long int (*fCreateSimpleSimulator)(int);
	void (*fDestroySimpleSimulator)(unsigned long int);

	int (*fRemoveAllOptimizationSimulatorsAndAdd)(unsigned long int, int, int);
	int (*fAddOptimizationSimulator)(unsigned long int, int, int);

	char* (*fSimpleExecute)(unsigned long int, const char*, const char*);
	void (*fFreeResult)(char*);

	unsigned long int (*fCreateSimulator)(int, int);
	void* (*fGetSimulator)(unsigned long int);
	void (*fDestroySimulator)(unsigned long int);

	int (*fInitializeSimulator)(void*);
	int (*fResetSimulator)(void*);
	int (*fConfigureSimulator)(void*, const char*, const char*);
	char* (*fGetConfiguration)(void*, const char*);
	unsigned long int (*fAllocateQubits)(void*, unsigned long int);
	unsigned long int (*fGetNumberOfQubits)(void*);
	int (*fClearSimulator)(void*);
	unsigned long long int (*fMeasure)(void*, const unsigned long int*, unsigned long int);
	int (*fApplyReset)(void*, const unsigned long int*, unsigned long int);
	double (*fProbability)(void*, unsigned long long int);
	void (*fFreeDoubleVector)(double*);
	void (*fFreeULLIVector)(unsigned long long int*);
	double* (*fAmplitude)(void*, unsigned long long int);
	double* (*fAllProbabilities)(void*);
	double* (*fProbabilities)(void*, const unsigned long long int*, unsigned long int);
	unsigned long long int* (*fSampleCounts)(void*, const unsigned long long int*, unsigned long int, unsigned long int);
	int (*fGetSimulatorType)(void*);
	int (*fGetSimulationType)(void*);
	int (*fFlushSimulator)(void*);
	int (*fSaveStateToInternalDestructive)(void*);
	int (*fRestoreInternalDestructiveSavedState)(void*);
	int (*fSaveState)(void*);
	int (*fRestoreState)(void*);
	int (*fSetMultithreading)(void*, int);
	int (*fGetMultithreading)(void*);
	int (*fIsQcsim)(void*);
	unsigned long long int (*fMeasureNoCollapse)(void*);

	int (*fApplyX)(void*, int);
	int (*fApplyY)(void*, int);
	int (*fApplyZ)(void*, int);
	int (*fApplyH)(void*, int);
	int (*fApplyS)(void*, int);
	int (*fApplySDG)(void*, int);
	int (*fApplyT)(void*, int);
	int (*fApplyTDG)(void*, int);
	int (*fApplySX)(void*, int);
	int (*fApplySXDG)(void*, int);
	int (*fApplyK)(void*, int);
	int (*fApplyP)(void*, int, double);
	int (*fApplyRx)(void*, int, double);
	int (*fApplyRy)(void*, int, double);
	int (*fApplyRz)(void*, int, double);
	int (*fApplyU)(void*, int, double, double, double, double);
	int (*fApplyCX)(void*, int, int);
	int (*fApplyCY)(void*, int, int);
	int (*fApplyCZ)(void*, int, int);
	int (*fApplyCH)(void*, int, int);
	int (*fApplyCSX)(void*, int, int);
	int (*fApplyCSXDG)(void*, int, int);
	int (*fApplyCP)(void*, int, int, double);
	int (*fApplyCRx)(void*, int, int, double);
	int (*fApplyCRy)(void*, int, int, double);
	int (*fApplyCRz)(void*, int, int, double);
	int (*fApplyCCX)(void*, int, int, int);
	int (*fApplySwap)(void*, int, int);
	int (*fApplyCSwap)(void*, int, int, int);
	int (*fApplyCU)(void*, int, int, double, double, double, double);
};