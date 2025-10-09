/**
 * @file GpuLibrary.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Gpu library class.
 *
 * Allows loading the gpu library dynamically and exposes the c api functions.
 */

#pragma once

#ifndef _GPU_LIBRARY_H
#define _GPU_LIBRARY_H

#ifdef __linux__

#include "../Utils/Library.h"

#include <vector>
#include <unordered_map>
#include <stdint.h>

namespace Simulators {

	// use it as a singleton
	class GpuLibrary : public Utils::Library
	{
	public:
		GpuLibrary(const GpuLibrary&) = delete;
		GpuLibrary& operator=(const GpuLibrary&) = delete;
		
		GpuLibrary(GpuLibrary&&) = default;
		GpuLibrary& operator=(GpuLibrary&&) = default;
		
		GpuLibrary() noexcept
		{
		}

		virtual ~GpuLibrary()
		{
			if (LibraryHandle)
				FreeLib();
		}

		bool Init(const char* libName) noexcept override
		{
			if (Utils::Library::Init(libName))
			{
				InitLib = (void* (*)())GetFunction("InitLib");
				CheckFunction((void*)InitLib, __LINE__);
				if (InitLib)
				{
					LibraryHandle = InitLib();
					if (LibraryHandle)
					{
						FreeLib = (void(*)())GetFunction("FreeLib");
						CheckFunction((void*)FreeLib, __LINE__);

						// state vector api functions

						fCreateStateVector = (void* (*)(void*))GetFunction("CreateStateVector");
						CheckFunction((void*)fCreateStateVector, __LINE__);
						fDestroyStateVector = (void (*)(void*))GetFunction("DestroyStateVector");
						CheckFunction((void*)fDestroyStateVector, __LINE__);

						fCreate = (int (*)(void*, unsigned int))GetFunction("Create");
						CheckFunction((void*)fCreate, __LINE__);
						fCreateWithState = (int (*)(void*, unsigned int, const double*))GetFunction("CreateWithState");
						CheckFunction((void*)fCreateWithState, __LINE__);
						fReset = (int (*)(void*))GetFunction("Reset");
						CheckFunction((void*)fReset, __LINE__);

						fSetDataType = (int (*)(void*, int))GetFunction("SetDataType");
						CheckFunction((void*)fSetDataType, __LINE__);
						fIsDoublePrecision = (int (*)(void*))GetFunction("IsDoublePrecision");
						CheckFunction((void*)fIsDoublePrecision, __LINE__);

						fMeasureQubitCollapse = (int (*)(void*, int))GetFunction("MeasureQubitCollapse");
						CheckFunction((void*)fMeasureQubitCollapse, __LINE__);
						fMeasureQubitNoCollapse = (int (*)(void*, int))GetFunction("MeasureQubitNoCollapse");
						CheckFunction((void*)fMeasureQubitNoCollapse, __LINE__);
						fMeasureQubitsCollapse = (int (*)(void*, int*, int*, int))GetFunction("MeasureQubitsCollapse");
						CheckFunction((void*)fMeasureQubitsCollapse, __LINE__);
						fMeasureQubitsNoCollapse = (int (*)(void*, int*, int*, int))GetFunction("MeasureQubitsNoCollapse");
						CheckFunction((void*)fMeasureQubitsNoCollapse, __LINE__);
						fMeasureAllQubitsCollapse = (unsigned long long (*)(void*))GetFunction("MeasureAllQubitsCollapse");
						CheckFunction((void*)fMeasureAllQubitsCollapse, __LINE__);
						fMeasureAllQubitsNoCollapse = (unsigned long long (*)(void*))GetFunction("MeasureAllQubitsNoCollapse");
						CheckFunction((void*)fMeasureAllQubitsNoCollapse, __LINE__);

						fSaveState = (int (*)(void*))GetFunction("SaveState");
						CheckFunction((void*)fSaveState, __LINE__);
						fSaveStateToHost = (int (*)(void*))GetFunction("SaveStateToHost");
						CheckFunction((void*)fSaveStateToHost, __LINE__);
						fSaveStateDestructive = (int (*)(void*))GetFunction("SaveStateDestructive");
						CheckFunction((void*)fSaveStateDestructive, __LINE__);
						fRestoreStateFreeSaved = (int (*)(void*))GetFunction("RestoreStateFreeSaved");
						CheckFunction((void*)fRestoreStateFreeSaved, __LINE__);
						fRestoreStateNoFreeSaved = (int (*)(void*))GetFunction("RestoreStateNoFreeSaved");
						CheckFunction((void*)fRestoreStateNoFreeSaved, __LINE__);
						fFreeSavedState = (void (*)(void*))GetFunction("FreeSavedState");
						CheckFunction((void*)fFreeSavedState, __LINE__);
						fClone = (void* (*)(void*))GetFunction("Clone");
						CheckFunction((void*)fClone, __LINE__);

						fSample = (int (*)(void*, unsigned int, long int*, unsigned int, int*))GetFunction("Sample");
						CheckFunction((void*)fSample, __LINE__);
						fSampleAll = (int (*)(void*, unsigned int, long int*))GetFunction("SampleAll");
						CheckFunction((void*)fSampleAll, __LINE__);
						fAmplitude = (int (*)(void*, long long int, double*, double*))GetFunction("Amplitude");
						CheckFunction((void*)fAmplitude, __LINE__);
						fProbability = (double (*)(void*, int*, int*, int))GetFunction("Probability");
						CheckFunction((void*)fProbability, __LINE__);
						fBasisStateProbability = (double (*)(void*, long long int))GetFunction("BasisStateProbability");
						CheckFunction((void*)fBasisStateProbability, __LINE__);
						fAllProbabilities = (int (*)(void* obj, double* probabilities))GetFunction("AllProbabilities");
						CheckFunction((void*)fAllProbabilities, __LINE__);
						fExpectationValue = (double (*)(void*, const char*, int))GetFunction("ExpectationValue");
						CheckFunction((void*)fExpectationValue, __LINE__);

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

						// mps api functions

						fCreateMPS = (void* (*)(void*))GetFunction("CreateMPS");
						CheckFunction((void*)fCreateMPS, __LINE__);
						fDestroyMPS = (void (*)(void*))GetFunction("DestroyMPS");
						CheckFunction((void*)fDestroyMPS, __LINE__);

						fMPSCreate = (int (*)(void*, unsigned int))GetFunction("MPSCreate");
						CheckFunction((void*)fMPSCreate, __LINE__);
						fMPSReset = (int (*)(void*))GetFunction("MPSReset");
						CheckFunction((void*)fMPSReset, __LINE__);

						fMPSIsValid = (int (*)(void*))GetFunction("MPSIsValid");
						CheckFunction((void*)fMPSIsValid, __LINE__);
						fMPSIsCreated = (int (*)(void*))GetFunction("MPSIsCreated");
						CheckFunction((void*)fMPSIsCreated, __LINE__);

						fMPSSetDataType = (int (*)(void*, int))GetFunction("MPSSetDataType");
						CheckFunction((void*)fMPSSetDataType, __LINE__);
						fMPSIsDoublePrecision = (int (*)(void*))GetFunction("MPSIsDoublePrecision");
						CheckFunction((void*)fMPSIsDoublePrecision, __LINE__);
						fMPSSetCutoff = (int (*)(void*, double))GetFunction("MPSSetCutoff");
						CheckFunction((void*)fMPSSetCutoff, __LINE__);
						fMPSGetCutoff = (double (*)(void*))GetFunction("MPSGetCutoff");
						CheckFunction((void*)fMPSGetCutoff, __LINE__);
						fMPSSetGesvdJ = (int (*)(void*, int))GetFunction("MPSSetGesvdJ"); 
						CheckFunction((void*)fMPSSetGesvdJ, __LINE__);
						fMPSGetGesvdJ = (int (*)(void*))GetFunction("MPSGetGesvdJ");
						CheckFunction((void*)fMPSGetGesvdJ, __LINE__);
						fMPSSetMaxExtent = (int (*)(void*, long int))GetFunction("MPSSetMaxExtent");
						CheckFunction((void*)fMPSSetMaxExtent, __LINE__);
						fMPSGetMaxExtent = (long int (*)(void*))GetFunction("MPSGetMaxExtent");
						CheckFunction((void*)fMPSGetMaxExtent, __LINE__);
						fMPSGetNrQubits = (int (*)(void*))GetFunction("MPSGetNrQubits");
						CheckFunction((void*)fMPSGetNrQubits, __LINE__);
						fMPSAmplitude = (int (*)(void*, long int, long int*, double*, double*))GetFunction("MPSAmplitude");
						CheckFunction((void*)fMPSAmplitude, __LINE__);
						fMPSProbability0 = (double (*)(void*, unsigned int))GetFunction("MPSProbability0");
						CheckFunction((void*)fMPSProbability0, __LINE__);
						fMPSMeasure = (int (*)(void*, unsigned int))GetFunction("MPSMeasure");
						CheckFunction((void*)fMPSMeasure, __LINE__);
						fMPSMeasureQubits = (int (*)(void*, long int, unsigned int*, int*))GetFunction("MPSMeasureQubits");
						CheckFunction((void*)fMPSMeasureQubits, __LINE__);

						fMPSGetMapForSample = (void* (*)())GetFunction("MPSGetMapForSample");
						CheckFunction((void*)fMPSGetMapForSample, __LINE__);
						fMPSFreeMapForSample = (int (*)(void*))GetFunction("MPSFreeMapForSample");
						CheckFunction((void*)fMPSFreeMapForSample, __LINE__);
						fMPSSample = (int (*)(void*, long int, long int, unsigned int*, void*))GetFunction("MPSSample");
						CheckFunction((void*)fMPSSample, __LINE__);

						fMPSSaveState = (int (*)(void*))GetFunction("MPSSaveState");
						CheckFunction((void*)fMPSSaveState, __LINE__);
						fMPSRestoreState = (int (*)(void*))GetFunction("MPSRestoreState");
						CheckFunction((void*)fMPSRestoreState, __LINE__);
						fMPSCleanSavedState = (int (*)(void*))GetFunction("MPSCleanSavedState");
						CheckFunction((void*)fMPSCleanSavedState, __LINE__);
						fMPSClone = (void* (*)(void*))GetFunction("MPSClone");
						CheckFunction((void*)fMPSClone, __LINE__);

						fMPSExpectationValue = (double (*)(void*, const char*, int))GetFunction("MPSExpectationValue");
						CheckFunction((void*)fMPSExpectationValue, __LINE__);

						fMPSApplyX = (int (*)(void*, unsigned int))GetFunction("MPSApplyX");
						CheckFunction((void*)fMPSApplyX, __LINE__);
						fMPSApplyY = (int (*)(void*, unsigned int))GetFunction("MPSApplyY");
						CheckFunction((void*)fMPSApplyY, __LINE__);
						fMPSApplyZ = (int (*)(void*, unsigned int))GetFunction("MPSApplyZ");
						CheckFunction((void*)fMPSApplyZ, __LINE__);
						fMPSApplyH = (int (*)(void*, unsigned int))GetFunction("MPSApplyH");
						CheckFunction((void*)fMPSApplyH, __LINE__);
						fMPSApplyS = (int (*)(void*, unsigned int))GetFunction("MPSApplyS");
						CheckFunction((void*)fMPSApplyS, __LINE__);
						fMPSApplySDG = (int (*)(void*, unsigned int))GetFunction("MPSApplySDG");
						CheckFunction((void*)fMPSApplySDG, __LINE__);
						fMPSApplyT = (int (*)(void*, unsigned int))GetFunction("MPSApplyT");
						CheckFunction((void*)fMPSApplyT, __LINE__);
						fMPSApplyTDG = (int (*)(void*, unsigned int))GetFunction("MPSApplyTDG");
						CheckFunction((void*)fMPSApplyTDG, __LINE__);
						fMPSApplySX = (int (*)(void*, unsigned int))GetFunction("MPSApplySX");
						CheckFunction((void*)fMPSApplySX, __LINE__);
						fMPSApplySXDG = (int (*)(void*, unsigned int))GetFunction("MPSApplySXDG");
						CheckFunction((void*)fMPSApplySXDG, __LINE__);
						fMPSApplyK = (int (*)(void*, unsigned int))GetFunction("MPSApplyK");
						CheckFunction((void*)fMPSApplyK, __LINE__);
						fMPSApplyP = (int (*)(void*, unsigned int, double))GetFunction("MPSApplyP");
						CheckFunction((void*)fMPSApplyP, __LINE__);
						fMPSApplyRx = (int (*)(void*, unsigned int, double))GetFunction("MPSApplyRx");
						CheckFunction((void*)fMPSApplyRx, __LINE__);
						fMPSApplyRy = (int (*)(void*, unsigned int, double))GetFunction("MPSApplyRy");
						CheckFunction((void*)fMPSApplyRy, __LINE__);
						fMPSApplyRz = (int (*)(void*, unsigned int, double))GetFunction("MPSApplyRz");
						CheckFunction((void*)fMPSApplyRz, __LINE__);
						fMPSApplyU = (int (*)(void*, unsigned int, double, double, double, double))GetFunction("MPSApplyU");
						CheckFunction((void*)fMPSApplyU, __LINE__);
						fMPSApplySwap = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplySwap");
						CheckFunction((void*)fMPSApplySwap, __LINE__);
						fMPSApplyCX = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCX");
						CheckFunction((void*)fMPSApplyCX, __LINE__);
						fMPSApplyCY = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCY");
						CheckFunction((void*)fMPSApplyCY, __LINE__);
						fMPSApplyCZ = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCZ");
						CheckFunction((void*)fMPSApplyCZ, __LINE__);
						fMPSApplyCH = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCH");
						CheckFunction((void*)fMPSApplyCH, __LINE__);
						fMPSApplyCSX = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCSX");
						CheckFunction((void*)fMPSApplyCSX, __LINE__);
						fMPSApplyCSXDG = (int (*)(void*, unsigned int, unsigned int))GetFunction("MPSApplyCSXDG");
						CheckFunction((void*)fMPSApplyCSXDG, __LINE__);
						fMPSApplyCP = (int (*)(void*, unsigned int, unsigned int, double))GetFunction("MPSApplyCP");
						CheckFunction((void*)fMPSApplyCP, __LINE__);
						fMPSApplyCRx = (int (*)(void*, unsigned int, unsigned int, double))GetFunction("MPSApplyCRx");
						CheckFunction((void*)fMPSApplyCRx, __LINE__);
						fMPSApplyCRy = (int (*)(void*, unsigned int, unsigned int, double))GetFunction("MPSApplyCRy");
						CheckFunction((void*)fMPSApplyCRy, __LINE__);
						fMPSApplyCRz = (int (*)(void*, unsigned int, unsigned int, double))GetFunction("MPSApplyCRz");
						CheckFunction((void*)fMPSApplyCRz, __LINE__);
						fMPSApplyCU = (int (*)(void*, unsigned int, unsigned int, double, double, double, double))GetFunction("MPSApplyCU");
						CheckFunction((void*)fMPSApplyCU, __LINE__);

						return true;
					}
					else std::cout << "GpuLibrary: Unable to initialize gpu library" << std::endl;
				}
				else std::cout << "GpuLibrary: Unable to get initialization function for gpu library" << std::endl;
			}
			else std::cout << "GpuLibrary: Unable to load gpu library" << std::endl;

			return false;
		}

		static void CheckFunction(void* func, int line)
		{
			if (!func)
			{
				std::cout << "GpuLibrary: Unable to load function, line #: " << std::to_string(line);
				const char* dlsym_error = dlerror();
				if (dlsym_error)
					std::cout << ", error: " << dlsym_error;
				
				std::cout << std::endl;
			}
		}

		bool IsValid() const
		{
			return LibraryHandle != nullptr;
		}

		// statevector functions

		void* CreateStateVector()
		{
			if (LibraryHandle)
				return fCreateStateVector(LibraryHandle);
			else
				throw std::runtime_error("GpuLibrary: Unable to create state vector");
		}

		void DestroyStateVector(void* obj)
		{
			if (LibraryHandle)
				fDestroyStateVector(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to destroy state vector");
		}

		bool Create(void* obj, unsigned int nrQubits)
		{
			if (LibraryHandle)
				return fCreate(obj, nrQubits) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to create state vector state");

			return false;
		}

		bool CreateWithState(void* obj, unsigned int nrQubits, const double* state)
		{
			if (LibraryHandle)
				return fCreateWithState(obj, nrQubits, state) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to create state vector state with a state");

			return false;
		}

		bool Reset(void* obj)
		{
			if (LibraryHandle)
				return fReset(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to reset state vector");
			
			return false;
		}

		bool SetDataType(void* obj, int dataType)
		{
			if (LibraryHandle)
				return fSetDataType(obj, dataType) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to set data type");

			return false;
		}

		bool IsDoublePrecision(void* obj) const
		{
			if (LibraryHandle)
				return fIsDoublePrecision(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to check if double precision");

			return false;
		}

		bool MeasureQubitCollapse(void* obj, int qubitIndex)
		{
			if (LibraryHandle)
				return fMeasureQubitCollapse(obj, qubitIndex) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubit with collapse");

			return false;
		}

		bool MeasureQubitNoCollapse(void* obj, int qubitIndex)
		{
			if (LibraryHandle)
				return fMeasureQubitNoCollapse(obj, qubitIndex) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubit no collapse");

			return false;
		}

		bool MeasureQubitsCollapse(void* obj, int* qubits, int* bitstring, int bitstringLen)
		{
			if (LibraryHandle)
				return fMeasureQubitsCollapse(obj, qubits, bitstring, bitstringLen) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubits with collapse");

			return false;
		}

		bool MeasureQubitsNoCollapse(void* obj, int* qubits, int* bitstring, int bitstringLen)
		{
			if (LibraryHandle)
				return fMeasureQubitsNoCollapse(obj, qubits, bitstring, bitstringLen) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubits with no collapse");

			return false;
		}

		unsigned long long MeasureAllQubitsCollapse(void* obj)
		{
			if (LibraryHandle)
				return fMeasureAllQubitsCollapse(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to measure all qubits with collapse");

			return 0;
		}

		unsigned long long MeasureAllQubitsNoCollapse(void* obj)
		{
			if (LibraryHandle)
				return fMeasureAllQubitsNoCollapse(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to measure all qubits with no collapse");

			return 0;
		}

		bool SaveState(void* obj)
		{
			if (LibraryHandle)
				return fSaveState(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to save state");

			return false;
		}

		bool SaveStateToHost(void* obj)
		{
			if (LibraryHandle)
				return fSaveStateToHost(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to save state to host");

			return false;
		}

		bool SaveStateDestructive(void* obj)
		{
			if (LibraryHandle)
				return fSaveStateDestructive(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to save state destructively");

			return false;
		}

		bool RestoreStateFreeSaved(void* obj)
		{
			if (LibraryHandle)
				return fRestoreStateFreeSaved(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to restore state free saved");

			return false;
		}

		bool RestoreStateNoFreeSaved(void* obj)
		{
			if (LibraryHandle)
				return fRestoreStateNoFreeSaved(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to restore state no free saved");

			return false;
		}

		void FreeSavedState(void* obj)
		{
			if (LibraryHandle)
				fFreeSavedState(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to free saved state");
		}

		void* Clone(void* obj) const
		{
			if (LibraryHandle)
				return fClone(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to clone state vector");

			return nullptr;
		}

		bool Sample(void* obj, unsigned int nSamples, long int* samples, unsigned int nBits, int* bits)
		{
			if (LibraryHandle)
				return fSample(obj, nSamples, samples, nBits, bits) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to sample state vector");

			return false;
		}

		bool SampleAll(void* obj, unsigned int nSamples, long int* samples)
		{
			if (LibraryHandle)
				return fSampleAll(obj, nSamples, samples) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to sample state vector");

			return false;
		}

		bool Amplitude(void* obj, long long int state, double* real, double* imaginary) const
		{
			if (LibraryHandle)
				return fAmplitude(obj, state, real, imaginary) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to get amplitude");

			return false;
		}

		double Probability(void* obj, int* qubits, int* mask, int len) const
		{
			if (LibraryHandle)
				return fProbability(obj, qubits, mask, len);
			else
				throw std::runtime_error("GpuLibrary: Unable to get probability");

			return 0;
		}

		double BasisStateProbability(void* obj, long long int state) const
		{
			if (LibraryHandle)
				return fBasisStateProbability(obj, state);
			else
				throw std::runtime_error("GpuLibrary: Unable to get basis state probability");

			return 0;
		}

		bool AllProbabilities(void* obj, double* probabilities) const
		{
			if (LibraryHandle)
				return fAllProbabilities(obj, probabilities) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to get all probabilities");

			return false;
		}

		double ExpectationValue(void* obj, const char* pauliString, int len) const
		{
			if (LibraryHandle)
				return fExpectationValue(obj, pauliString, len);
			else
				throw std::runtime_error("GpuLibrary: Unable to get expectation value");

			return 0;
		}

		bool ApplyX(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyX(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply X gate");

			return false;
		}

		bool ApplyY(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyY(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Y gate");

			return false;
		}

		bool ApplyZ(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyZ(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Z gate");

			return false;
		}

		bool ApplyH(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyH(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply H gate");

			return false;
		}

		bool ApplyS(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyS(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply S gate");

			return false;
		}

		bool ApplySDG(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplySDG(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply SDG gate");

			return false;
		}

		bool ApplyT(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyT(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply T gate");

			return false;
		}

		bool ApplyTDG(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyTDG(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply TDG gate");

			return false;
		}

		bool ApplySX(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplySX(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply SX gate");

			return false;
		}

		bool ApplySXDG(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplySXDG(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply SXDG gate");

			return false;
		}

		bool ApplyK(void* obj, int qubit)
		{
			if (LibraryHandle)
				return fApplyK(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply K gate");

			return false;
		}

		bool ApplyP(void* obj, int qubit, double theta)
		{
			if (LibraryHandle)
				return fApplyP(obj, qubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply P gate");

			return false;
		}

		bool ApplyRx(void* obj, int qubit, double theta)
		{
			if (LibraryHandle)
				return fApplyRx(obj, qubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Rx gate");

			return false;
		}

		bool ApplyRy(void* obj, int qubit, double theta)
		{
			if (LibraryHandle)
				return fApplyRy(obj, qubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Ry gate");

			return false;
		}

		bool ApplyRz(void* obj, int qubit, double theta)
		{
			if (LibraryHandle)
				return fApplyRz(obj, qubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Rz gate");

			return false;
		}

		bool ApplyU(void* obj, int qubit, double theta, double phi, double lambda, double gamma)
		{
			if (LibraryHandle)
				return fApplyU(obj, qubit, theta, phi, lambda, gamma) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply U gate");

			return false;
		}

		bool ApplyCX(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCX(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CX gate");

			return false;
		}

		bool ApplyCY(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCY(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CY gate");

			return false;
		}

		bool ApplyCZ(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCZ(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CZ gate");

			return false;
		}

		bool ApplyCH(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCH(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CH gate");

			return false;
		}

		bool ApplyCSX(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCSX(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CSX gate");

			return false;
		}

		bool ApplyCSXDG(void* obj, int controlQubit, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCSXDG(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CSXDG gate");

			return false;
		}

		bool ApplyCP(void* obj, int controlQubit, int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fApplyCP(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CP gate");

			return false;
		}

		bool ApplyCRx(void* obj, int controlQubit, int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fApplyCRx(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CRx gate");

			return false;
		}

		bool ApplyCRy(void* obj, int controlQubit, int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fApplyCRy(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CRy gate");

			return false;
		}

		bool ApplyCRz(void* obj, int controlQubit, int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fApplyCRz(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CRz gate");

			return false;
		}

		bool ApplyCCX(void* obj, int controlQubit1, int controlQubit2, int targetQubit)
		{
			if (LibraryHandle)
				return fApplyCCX(obj, controlQubit1, controlQubit2, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CCX gate");

			return false;
		}

		bool ApplySwap(void* obj, int qubit1, int qubit2)
		{
			if (LibraryHandle)
				return fApplySwap(obj, qubit1, qubit2) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Swap gate");

			return false;
		}

		bool ApplyCSwap(void* obj, int controlQubit, int qubit1, int qubit2)
		{
			if (LibraryHandle)
				return fApplyCSwap(obj, controlQubit, qubit1, qubit2) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CSwap gate");

			return false;
		}

		bool ApplyCU(void* obj, int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma)
		{
			if (LibraryHandle)
				return fApplyCU(obj, controlQubit, targetQubit, theta, phi, lambda, gamma) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply CU gate");

			return false;
		}

		// mps functions

		void* CreateMPS()
		{
			if (LibraryHandle)
				return fCreateMPS(LibraryHandle);
			else
				throw std::runtime_error("GpuLibrary: Unable to create mps");
		}

		void DestroyMPS(void* obj)
		{
			if (LibraryHandle)
				fDestroyMPS(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to destroy mps");
		}

		bool MPSCreate(void* obj, unsigned int nrQubits)
		{
			if (LibraryHandle)
				return fMPSCreate(obj, nrQubits) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to create mps with the specified number of qubits");

			return false;
		}

		bool MPSReset(void* obj)
		{
			if (LibraryHandle)
				return fMPSReset(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to reset mps");

			return false;
		}

		bool MPSIsValid(void* obj) const
		{
			if (LibraryHandle)
				return fMPSIsValid(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to check if mps is valid");

			return false;
		}

		bool MPSIsCreated(void* obj) const
		{
			if (LibraryHandle)
				return fMPSIsCreated(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to check if mps is created");

			return false;
		}

		bool MPSSetDataType(void* obj, int useDoublePrecision)
		{
			if (LibraryHandle)
				return fMPSSetDataType(obj, useDoublePrecision) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to set precision for mps");

			return false;
		}

		bool MPSIsDoublePrecision(void* obj) const
		{
			if (LibraryHandle)
				return fMPSIsDoublePrecision(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to get precision for mps");

			return false;
		}

		bool MPSSetCutoff(void* obj, double val)
		{
			if (LibraryHandle)
				return fMPSSetCutoff(obj, val) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to set cutoff for mps");

			return false;
		}

		double MPSGetCutoff(void* obj) const
		{
			if (LibraryHandle)
				return fMPSGetCutoff(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to get cutoff for mps");
		}

		bool MPSSetGesvdJ(void* obj, int val)
		{
			if (LibraryHandle)
				return fMPSSetGesvdJ(obj, val) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to set GesvdJ for mps");

			return false;
		}

		bool MPSGetGesvdJ(void* obj) const
		{
			if (LibraryHandle)
				return fMPSGetGesvdJ(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to get GesvdJ for mps");

			return false;
		}

		bool MPSSetMaxExtent(void* obj, long int val)
		{
			if (LibraryHandle)
				return fMPSSetMaxExtent(obj, val) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to set max extent for mps");

			return false;
		}

		long int MPSGetMaxExtent(void* obj)
		{
			if (LibraryHandle)
				return fMPSGetMaxExtent(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to get max extent for mps");

			return 0;
		}

		int MPSGetNrQubits(void* obj)
		{
			if (LibraryHandle)
				return fMPSGetNrQubits(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to get nr qubits for mps");

			return 0;
		}

		bool MPSAmplitude(void* obj, long int numFixedValues, long int* fixedValues, double* real, double* imaginary)
		{
			if (LibraryHandle)
				return fMPSAmplitude(obj, numFixedValues, fixedValues, real, imaginary) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to get mps amplitude");

			return false;
		}

		double MPSProbability0(void* obj, unsigned int qubit)
		{
			if (LibraryHandle)
				return fMPSProbability0(obj, qubit);
			else
				throw std::runtime_error("GpuLibrary: Unable to get probability for 0 for mps");

			return 0.0;
		}

		bool MPSMeasure(void* obj, unsigned int qubit)
		{
			if (LibraryHandle)
				return fMPSMeasure(obj, qubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubit on mps");

			return false;
		}

		bool MPSMeasureQubits(void* obj, long int numQubits, unsigned int* qubits, int* result)
		{
			if (LibraryHandle)
				return fMPSMeasureQubits(obj, numQubits, qubits, result) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to measure qubits on mps");

			return false;
		}

		std::unordered_map<std::vector<bool>, int64_t>* MPSGetMapForSample()
		{
			if (LibraryHandle)
				return (std::unordered_map<std::vector<bool>, int64_t>*)fMPSGetMapForSample();
			else
				throw std::runtime_error("GpuLibrary: Unable to get map for sample for mps");

			return nullptr;
		}

		bool MPSFreeMapForSample(std::unordered_map<std::vector<bool>, int64_t>* map)
		{
			if (LibraryHandle)
				return fMPSFreeMapForSample((void*)map) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to free map for sample for mps");

			return false;
		}

		bool MPSSample(void* obj, long int numShots, long int numQubits, unsigned int* qubits, void* resultMap)
		{
			if (LibraryHandle)
				return fMPSSample(obj, numShots, numQubits, qubits, resultMap) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to sample mps");

			return false;
		}

		bool MPSSaveState(void* obj)
		{
			if (LibraryHandle)
				return fMPSSaveState(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to save mps state");

			return false;
		}

		bool MPSRestoreState(void* obj)
		{
			if (LibraryHandle)
				return fMPSRestoreState(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to restore mps state");

			return false;
		}

		bool MPSCleanSavedState(void* obj)
		{
			if (LibraryHandle)
				return fMPSCleanSavedState(obj) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to clean mps saved state");

			return false;
		}

		void* MPSClone(void* obj)
		{
			if (LibraryHandle)
				return fMPSClone(obj);
			else
				throw std::runtime_error("GpuLibrary: Unable to clone mps");

			return nullptr;
		}

		double MPSExpectationValue(void* obj, const char* pauliString, int len) const
		{
			if (LibraryHandle)
				return fMPSExpectationValue(obj, pauliString, len);
			else
				throw std::runtime_error("GpuLibrary: Unable to get mps expectation value");

			return 0;
		}

		bool MPSApplyX(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyX(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply X gate on mps");

			return false;
		}

		bool MPSApplyY(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyY(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Y gate on mps");

			return false;
		}

		bool MPSApplyZ(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyZ(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply Z gate on mps");

			return false;
		}

		bool MPSApplyH(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyH(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply H gate on mps");

			return false;
		}

		bool MPSApplyS(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyS(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply S gate on mps");

			return false;
		}

		bool MPSApplySDG(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplySDG(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply sdg gate on mps");

			return false;
		}

		bool MPSApplyT(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyT(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply t gate on mps");

			return false;
		}

		bool MPSApplyTDG(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyTDG(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to appl tdg gate on mps");

			return false;
		}

		bool MPSApplySX(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplySX(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply sx gate on mps");

			return false;
		}

		bool MPSApplySXDG(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplySXDG(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply sxdg gate on mps");

			return false;
		}

		bool MPSApplyK(void* obj, unsigned int siteA)
		{
			if (LibraryHandle)
				return fMPSApplyK(obj, siteA) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply k gate on mps");

			return false;
		}

		bool MPSApplyP(void* obj, unsigned int siteA, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyP(obj, siteA, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply p gate on mps");
			return false;
		}

		bool MPSApplyRx(void* obj, unsigned int siteA, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyRx(obj, siteA, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply rx gate on mps");

			return false;
		}

		bool MPSApplyRy(void* obj, unsigned int siteA, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyRy(obj, siteA, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply ry gate on mps");

			return false;
		}

		bool MPSApplyRz(void* obj, unsigned int siteA, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyRz(obj, siteA, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply rz gate on mps");

			return false;
		}

		bool MPSApplyU(void* obj, unsigned int siteA, double theta, double phi, double lambda, double gamma)
		{
			if (LibraryHandle)
				return fMPSApplyU(obj, siteA, theta, phi, lambda, gamma) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply u gate on mps");

			return false;
		}

		bool MPSApplySwap(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplySwap(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply swap gate on mps");

			return false;
		}

		bool MPSApplyCX(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCX(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cx gate on mps");

			return false;
		}

		bool MPSApplyCY(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCY(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cy gate on mps");

			return false;
		}

		bool MPSApplyCZ(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCZ(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cz gate on mps");

			return false;
		}

		bool MPSApplyCH(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCH(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply ch gate on mps");

			return false;
		}

		bool MPSApplyCSX(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCSX(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply csx gate on mps");
		}

		bool MPSApplyCSXDG(void* obj, unsigned int controlQubit, unsigned int targetQubit)
		{
			if (LibraryHandle)
				return fMPSApplyCSXDG(obj, controlQubit, targetQubit) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply csxdg gate on mps");

			return false;
		}

		bool MPSApplyCP(void* obj, unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyCP(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cp gate on mps");

			return false;
		}

		bool MPSApplyCRx(void* obj, unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyCRx(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply crx gate on mps");

			return false;
		}

		bool MPSApplyCRy(void* obj, unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyCRy(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cry gate on mps");

			return false;
		}

		bool MPSApplyCRz(void* obj, unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (LibraryHandle)
				return fMPSApplyCRz(obj, controlQubit, targetQubit, theta) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply crz gate on mps");

			return false;
		}

		bool MPSApplyCU(void* obj, unsigned int controlQubit, unsigned int targetQubit, double theta, double phi, double lambda, double gamma)
		{
			if (LibraryHandle)
				return fMPSApplyCU(obj, controlQubit, targetQubit, theta, phi, lambda, gamma) == 1;
			else
				throw std::runtime_error("GpuLibrary: Unable to apply cu gate on mps");

			return false;
		}

	private:
		void* LibraryHandle = nullptr;

		void* (*InitLib)();
		void (*FreeLib)();

		void* (*fCreateStateVector)(void*);
		void (*fDestroyStateVector)(void*);

		// statevector functions
		int (*fSetDataType)(void*, int);
		int (*fIsDoublePrecision)(void*);
		int (*fCreate)(void*, unsigned int);
		int (*fReset)(void*);
		int (*fCreateWithState)(void*, unsigned int, const double*);
		int (*fMeasureQubitCollapse)(void*, int);
		int (*fMeasureQubitNoCollapse)(void*, int);
		int (*fMeasureQubitsCollapse)(void*, int*, int*, int);
		int (*fMeasureQubitsNoCollapse)(void*, int*, int*, int);
		unsigned long long (*fMeasureAllQubitsCollapse)(void*);
		unsigned long long (*fMeasureAllQubitsNoCollapse)(void*);

		int (*fSaveState)(void*);
		int (*fSaveStateToHost)(void*);
		int (*fSaveStateDestructive)(void*);
		int (*fRestoreStateFreeSaved)(void*);
		int (*fRestoreStateNoFreeSaved)(void*);
		void (*fFreeSavedState)(void* obj);
		void* (*fClone)(void*);
		int (*fSample)(void*, unsigned int, long int*, unsigned int, int*);
		int (*fSampleAll)(void*, unsigned int, long int*);
		int (*fAmplitude)(void*, long long int, double*, double*);
		double (*fProbability)(void*, int*, int*, int);
		double (*fBasisStateProbability)(void*, long long int);
		int (*fAllProbabilities)(void*, double*);
		double (*fExpectationValue)(void*, const char*, int);

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

		// mps functions
		void* (*fCreateMPS)(void*);
		void (*fDestroyMPS)(void*);

		int (*fMPSCreate)(void*, unsigned int);
		int (*fMPSReset)(void*);

		int (*fMPSIsValid)(void*);
		int (*fMPSIsCreated)(void*);

		int (*fMPSSetDataType)(void*, int);
		int (*fMPSIsDoublePrecision)(void*);
		int (*fMPSSetCutoff)(void*, double);
		double (*fMPSGetCutoff)(void*);
		int (*fMPSSetGesvdJ)(void*, int);
		int (*fMPSGetGesvdJ)(void*);
		int (*fMPSSetMaxExtent)(void*, long int);
		long int (*fMPSGetMaxExtent)(void*);
		int (*fMPSGetNrQubits)(void*);
		int (*fMPSAmplitude)(void*, long int, long int*, double*, double*);
		double (*fMPSProbability0)(void*, unsigned int);
		int (*fMPSMeasure)(void*, unsigned int);
		int (*fMPSMeasureQubits)(void*, long int, unsigned int*, int*);
		
		void* (*fMPSGetMapForSample)();
		int (*fMPSFreeMapForSample)(void*);
		int (*fMPSSample)(void*, long int, long int, unsigned int*, void*);

		int (*fMPSSaveState)(void*);
		int (*fMPSRestoreState)(void*);
		int (*fMPSCleanSavedState)(void*);
		void* (*fMPSClone)(void*);

		double (*fMPSExpectationValue)(void*, const char*, int);

		int (*fMPSApplyX)(void*, unsigned int);
		int (*fMPSApplyY)(void*, unsigned int);
		int (*fMPSApplyZ)(void*, unsigned int);
		int (*fMPSApplyH)(void*, unsigned int);
		int (*fMPSApplyS)(void*, unsigned int);
		int (*fMPSApplySDG)(void*, unsigned int);
		int (*fMPSApplyT)(void*, unsigned int);
		int (*fMPSApplyTDG)(void*, unsigned int);
		int (*fMPSApplySX)(void*, unsigned int);
		int (*fMPSApplySXDG)(void*, unsigned int);
		int (*fMPSApplyK)(void*, unsigned int);
		int (*fMPSApplyP)(void*, unsigned int, double);
		int (*fMPSApplyRx)(void*, unsigned int, double);
		int (*fMPSApplyRy)(void*, unsigned int, double);
		int (*fMPSApplyRz)(void*, unsigned int, double);
		int (*fMPSApplyU)(void*, unsigned int, double, double, double, double);
		int (*fMPSApplySwap)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCX)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCY)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCZ)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCH)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCSX)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCSXDG)(void*, unsigned int, unsigned int);
		int (*fMPSApplyCP)(void*, unsigned int, unsigned int, double);
		int (*fMPSApplyCRx)(void*, unsigned int, unsigned int, double);
		int (*fMPSApplyCRy)(void*, unsigned int, unsigned int, double);
		int (*fMPSApplyCRz)(void*, unsigned int, unsigned int, double);
		int (*fMPSApplyCU)(void*, unsigned int, unsigned int, double, double, double, double);
	};
}

#endif
#endif

