/**
 * @file interface.h
 * @version 1.0
 *
 * @section DESCRIPTION
 * C interface API for the maestro library.
 */

#pragma once

#ifndef _MAESTRO_INTERFACE_H_
#define _MAESTRO_INTERFACE_H_

extern "C" {

	void* GetMaestroObject();

	unsigned long int CreateSimpleSimulator(int nrQubits);
	void DestroySimpleSimulator(unsigned long int simHandle);

	int RemoveAllOptimizationSimulatorsAndAdd(unsigned long int simHandle, int simType, int simExecType);
	int AddOptimizationSimulator(unsigned long int simHandle, int simType, int simExecType);

	char* SimpleExecute(unsigned long int simpleSim, const char* jsonCircuit, const char* jsonConfig);
	void FreeResult(char* result);

	unsigned long int CreateSimulator(int simType, int simExecType);
	void* GetSimulator(unsigned long int simHandle);
	void DestroySimulator(unsigned long int simHandle);

	int InitializeSimulator(void* sim);
	int ResetSimulator(void* sim);
	int ConfigureSimulator(void* sim, const char* key, const char* value);
	char* GetConfiguration(void* sim, const char* key);
	unsigned long int AllocateQubits(void* sim, unsigned long int nrQubits);
	unsigned long int GetNumberOfQubits(void* sim);
	int ClearSimulator(void* sim);
	unsigned long long int Measure(void* sim, const unsigned long int* qubits, unsigned long int nrQubits);
	int ApplyReset(void* sim, const unsigned long int* qubits, unsigned long int nrQubits);
	double Probability(void* sim, unsigned long long int outcome);
	void FreeDoubleVector(double* vec);
	void FreeULLIVector(unsigned long long int* vec);
	double* Amplitude(void* sim, unsigned long long int outcome);
	double* AllProbabilities(void* sim);
	double* Probabilities(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits);
	unsigned long long int* SampleCounts(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits, unsigned long int shots);
	int GetSimulatorType(void* sim);
	int GetSimulationType(void* sim);
	int FlushSimulator(void* sim);
	int SaveStateToInternalDestructive(void* sim);
	int RestoreInternalDestructiveSavedState(void* sim);
	int SaveState(void* sim);
	int RestoreState(void* sim);
	int SetMultithreading(void* sim, int multithreading);
	int GetMultithreading(void* sim);
	int IsQcsim(void* sim);
	unsigned long long int MeasureNoCollapse(void* sim);

	int ApplyX(void* sim, int qubit);
	int ApplyY(void* sim, int qubit);
	int ApplyZ(void* sim, int qubit);
	int ApplyH(void* sim, int qubit);
	int ApplyS(void* sim, int qubit);
	int ApplySDG(void* sim, int qubit);
	int ApplyT(void* sim, int qubit);
	int ApplyTDG(void* sim, int qubit);
	int ApplySX(void* sim, int qubit);
	int ApplySXDG(void* sim, int qubit);
	int ApplyK(void* sim, int qubit);
	int ApplyP(void* sim, int qubit, double theta);
	int ApplyRx(void* sim, int qubit, double theta);
	int ApplyRy(void* sim, int qubit, double theta);
	int ApplyRz(void* sim, int qubit, double theta);
	int ApplyU(void* sim, int qubit, double theta, double phi, double lambda, double gamma);
	int ApplyCX(void* sim, int controlQubit, int targetQubit);
	int ApplyCY(void* sim, int controlQubit, int targetQubit);
	int ApplyCZ(void* sim, int controlQubit, int targetQubit);
	int ApplyCH(void* sim, int controlQubit, int targetQubit);
	int ApplyCSX(void* sim, int controlQubit, int targetQubit);
	int ApplyCSXDG(void* sim, int controlQubit, int targetQubit);
	int ApplyCP(void* sim, int controlQubit, int targetQubit, double theta);
	int ApplyCRx(void* sim, int controlQubit, int targetQubit, double theta);
	int ApplyCRy(void* sim, int controlQubit, int targetQubit, double theta);
	int ApplyCRz(void* sim, int controlQubit, int targetQubit, double theta);
	int ApplyCCX(void* sim, int controlQubit1, int controlQubit2, int targetQubit);
	int ApplySwap(void* sim, int qubit1, int qubit2);
	int ApplyCSwap(void* sim, int controlQubit, int qubit1, int qubit2);
	int ApplyCU(void* sim, int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma);
}

#endif

