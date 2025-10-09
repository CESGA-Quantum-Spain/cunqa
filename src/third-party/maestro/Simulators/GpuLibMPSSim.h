/**
 * @file GpuLibMPSSim.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Gpu MPS library class.
 *
 * Just a wrapped aroung c api functions, should not be used directly but with a adapter/bridge pattern to expose the same interface as the other ones.
 */

#pragma once

#ifndef _GPU_LIB_MPS_SIM_H_
#define _GPU_LIB_MPS_SIM_H_

#ifdef __linux__

#include "GpuLibrary.h"

namespace Simulators {

	class GpuLibMPSSim {
	public:
		explicit GpuLibMPSSim(const std::shared_ptr<GpuLibrary>& lib)
			: lib(lib)
		{
			if (lib) obj = lib->CreateMPS();
			else obj = nullptr;
		}

		GpuLibMPSSim(const std::shared_ptr<GpuLibrary>& lib, void* obj) : lib(lib), obj(obj)
		{
		}

		GpuLibMPSSim() = delete;
		GpuLibMPSSim(const GpuLibMPSSim&) = delete;
		GpuLibMPSSim& operator=(const GpuLibMPSSim&) = delete;
		GpuLibMPSSim(GpuLibMPSSim&&) = default;
		GpuLibMPSSim& operator=(GpuLibMPSSim&&) = default;

		~GpuLibMPSSim()
		{
			if (lib && obj)
				lib->DestroyMPS(obj);
		}

		bool Create(unsigned int nrQubits)
		{
			if (obj)
				return lib->MPSCreate(obj, nrQubits);

			return false;
		}

		bool Reset()
		{
			if (obj)
				return lib->MPSReset(obj);
			
			return false;
		}


		bool IsValid() const
		{
			if (obj)
				return lib->MPSIsValid(obj);

			return false;
		}

		bool IsCreated() const
		{
			if (obj)
				return lib->MPSIsCreated(obj);

			return false;
		}

		bool SetDataType(int useDoublePrecision)
		{
			if (obj)
				return lib->MPSSetDataType(obj, useDoublePrecision);

			return false;
		}

		bool IsDoublePrecision() const
		{
			if (obj)
				return lib->MPSIsDoublePrecision(obj);

			return false;
		}

		bool SetCutoff(double val)
		{
			if (obj)
				return lib->MPSSetCutoff(obj, val);

			return false;
		}

		double GetCutoff() const
		{
			if (obj)
				return lib->MPSGetCutoff(obj);

			return 0.;
		}

		bool SetGesvdJ(int val)
		{
			if (obj)
				return lib->MPSSetGesvdJ(obj, val);

			return false;
		}

		bool GetGesvdJ() const
		{
			if (obj)
				return lib->MPSGetGesvdJ(obj);

			return false;
		}

		bool SetMaxExtent(long int val)
		{
			if (obj)
				return lib->MPSSetMaxExtent(obj, val);

			return false;
		}

		long int GetMaxExtent() const
		{
			if (obj)
				return lib->MPSGetMaxExtent(obj);

			return 0;
		}

		int GetNrQubits() const
		{
			if (obj)
				return lib->MPSGetNrQubits(obj);

			return 0;
		}

		bool Amplitude(long int numFixedValues, long int* fixedValues, double* real, double* imaginary) const
		{
			if (obj)
				return lib->MPSAmplitude(obj, numFixedValues, fixedValues, real, imaginary);

			return false;
		}

		double Probability0(unsigned int qubit) const
		{
			if (obj)
				return lib->MPSProbability0(obj, qubit);

			return 0.;
		}

		bool Measure(unsigned int qubit)
		{
			if (obj)
				return lib->MPSMeasure(obj, qubit);

			return 0;
		}

		bool MeasureQubits(long int numQubits, unsigned int* qubits, int* result)
		{
			if (obj)
				return lib->MPSMeasureQubits(obj, numQubits, qubits, result);

			return false;
		}

		std::unordered_map<std::vector<bool>, int64_t>* GetMapForSample() const
		{
			if (lib)
				return lib->MPSGetMapForSample();

			return nullptr;
		}

		bool FreeMapForSample(std::unordered_map<std::vector<bool>, int64_t>* map) const
		{
			if (lib)
				return lib->MPSFreeMapForSample(map);

			return false;
		}

		bool Sample(long int numShots, long int numQubits, unsigned int* qubits, void* resultMap)
		{
			if (obj)
				return lib->MPSSample(obj, numShots, numQubits, qubits, resultMap);

			return false;
		}

		bool SaveState()
		{
			if (obj)
				return lib->MPSSaveState(obj);

			return false;
		}

		bool RestoreState()
		{
			if (obj)
				return lib->MPSRestoreState(obj);

			return false;
		}

		bool CleanSavedState()
		{
			if (obj)
				return lib->MPSCleanSavedState(obj);

			return false;
		}

		std::unique_ptr<GpuLibMPSSim> Clone() const
		{
			if (obj)
				return std::make_unique<GpuLibMPSSim>(lib, lib->MPSClone(obj));

			return nullptr;
		}

		double ExpectationValue(const std::string& pauliString) const
		{
			if (obj)
				return lib->MPSExpectationValue(obj, pauliString.c_str(), pauliString.length());

			return 0.0;
		}

		bool ApplyX(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyX(obj, siteA);

			return false;
		}

		bool ApplyY(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyY(obj, siteA);

			return false;
		}

		bool ApplyZ(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyZ(obj, siteA);

			return false;
		}

		bool ApplyH(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyH(obj, siteA);

			return false;
		}

		bool ApplyS(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyS(obj, siteA);

			return false;
		}

		bool ApplySDG(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplySDG(obj, siteA);

			return false;
		}

		bool ApplyT(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyT(obj, siteA);

			return false;
		}

		bool ApplyTDG(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyTDG(obj, siteA);

			return false;
		}

		bool ApplySX(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplySX(obj, siteA);

			return false;
		}

		bool ApplySXDG(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplySXDG(obj, siteA);

			return false;
		}

		bool ApplyK(unsigned int siteA)
		{
			if (obj)
				return lib->MPSApplyK(obj, siteA);

			return false;
		}

		bool ApplyP(unsigned int siteA, double theta)
		{
			if (obj)
				return lib->MPSApplyP(obj, siteA, theta);

			return false;
		}

		bool ApplyRx(unsigned int siteA, double theta)
		{
			if (obj)
				return lib->MPSApplyRx(obj, siteA, theta);

			return false;
		}

		bool ApplyRy(unsigned int siteA, double theta)
		{
			if (obj)
				return lib->MPSApplyRy(obj, siteA, theta);

			return false;
		}

		bool ApplyRz(unsigned int siteA, double theta)
		{
			if (obj)
				return lib->MPSApplyRz(obj, siteA, theta);

			return false;
		}

		bool ApplyU(unsigned int siteA, double theta, double phi, double lambda, double gamma)
		{
			if (obj)
				return lib->MPSApplyU(obj, siteA, theta, phi, lambda, gamma);

			return false;
		}

		bool ApplySwap(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplySwap(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCX(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCX(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCY(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCY(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCZ(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCZ(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCH(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCH(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCSX(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCSX(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCSXDG(unsigned int controlQubit, unsigned int targetQubit)
		{
			if (obj)
				return lib->MPSApplyCSXDG(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCP(unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (obj)
				return lib->MPSApplyCP(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRx(unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (obj)
				return lib->MPSApplyCRx(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRy(unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (obj)
				return lib->MPSApplyCRy(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRz(unsigned int controlQubit, unsigned int targetQubit, double theta)
		{
			if (obj)
				return lib->MPSApplyCRz(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCU(unsigned int controlQubit, unsigned int targetQubit, double theta, double phi, double lambda, double gamma)
		{
			if (obj)
				return lib->MPSApplyCU(obj, controlQubit, targetQubit, theta, phi, lambda, gamma);

			return false;
		}

	private:
		std::shared_ptr<GpuLibrary> lib;
		void* obj;
	};

} // namespace Simulators

#endif // __linux__

#endif // _GPU_LIB_MPS_SIM_H_