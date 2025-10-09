/**
 * @file GpuLibStateVectorSim.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Gpu StateVector library class.
 *
 * Just a wrapped aroung c api functions, should not be used directly but with a adapter/bridge pattern to expose the same interface as the other ones.
 */

#pragma once

#ifndef _GPU_LIB_STATEVECTOR_SIM
#define _GPU_LIB_STATEVECTOR_SIM 1

#ifdef __linux__

#include "GpuLibrary.h"

namespace Simulators {

	class GpuLibStateVectorSim {
	public:
		explicit GpuLibStateVectorSim(const std::shared_ptr<GpuLibrary>& lib)
			: lib(lib)
		{
			if (lib) obj = lib->CreateStateVector();
			else obj = nullptr;
		}

		GpuLibStateVectorSim(const std::shared_ptr<GpuLibrary>& lib, void* obj) : lib(lib), obj(obj)
		{
		}

		GpuLibStateVectorSim() = delete;
		GpuLibStateVectorSim(const GpuLibStateVectorSim&) = delete;
		GpuLibStateVectorSim& operator=(const GpuLibStateVectorSim&) = delete;
		GpuLibStateVectorSim(GpuLibStateVectorSim&&) = default;
		GpuLibStateVectorSim& operator=(GpuLibStateVectorSim&&) = default;
		
		~GpuLibStateVectorSim()
		{
			if (lib && obj)
				lib->DestroyStateVector(obj);
		}

		bool Create(unsigned int nrQubits)
		{
			if (obj)
				return lib->Create(obj, nrQubits);

			return false;
		}

		bool CreateWithState(unsigned int nrQubits, const double* state)
		{
			if (obj)
				return lib->CreateWithState(obj, nrQubits, state);

			return false;
		}

		bool Reset()
		{
			if (obj)
				return lib->Reset(obj);

			return false;
		}

		bool SetDataType(bool useDoublePrecision)
		{
			if (obj)
				return lib->SetDataType(obj, useDoublePrecision ? 1 : 0);
			return false;
		}

		bool IsDoublePrecision() const
		{
			if (obj)
				return lib->IsDoublePrecision(obj);
			return false;
		}

		bool MeasureQubitCollapse(int qubitIndex)
		{
			if (obj)
				return lib->MeasureQubitCollapse(obj, qubitIndex);

			return false;
		}

		bool MeasureQubitNoCollapse(int qubitIndex)
		{
			if (obj)
				return lib->MeasureQubitNoCollapse(obj, qubitIndex);

			return false;
		}

		bool MeasureQubitsCollapse(int* qubits, int* bitstring, int bitstringLen)
		{
			if (obj)
				return lib->MeasureQubitsCollapse(obj, qubits, bitstring, bitstringLen);
			
			return false;
		}

		bool MeasureQubitsNoCollapse(int* qubits, int* bitstring, int bitstringLen)
		{
			if (obj)
				return lib->MeasureQubitsNoCollapse(obj, qubits, bitstring, bitstringLen);

			return false;
		}

		unsigned long long MeasureAllQubitsCollapse()
		{
			if (obj)
				return lib->MeasureAllQubitsCollapse(obj);
			
			return static_cast<unsigned long long>(-1);
		}

		unsigned long long MeasureAllQubitsNoCollapse()
		{
			if (obj)
				return lib->MeasureAllQubitsNoCollapse(obj);

			return static_cast<unsigned long long>(-1);
		}

		bool SaveState()
		{
			if(obj)
				return lib->SaveState(obj);

			return false;
		}

		bool SaveStateToHost()
		{
			if (obj)
				return lib->SaveStateToHost(obj);

			return false;
		}

		bool SaveStateDestructive()
		{
			if (obj)
				return lib->SaveStateDestructive(obj);

			return false;
		}

		bool RestoreStateFreeSaved()
		{
			if (obj)
				return lib->RestoreStateFreeSaved(obj);

			return false;
		}

		bool RestoreStateNoFreeSaved()
		{
			if (obj)
				return lib->RestoreStateNoFreeSaved(obj);

			return false;
		}

		void FreeSavedState()
		{
			if (obj)
				lib->FreeSavedState(obj);
		}

		std::unique_ptr<GpuLibStateVectorSim> Clone()
		{
			if (obj)
				return std::make_unique<GpuLibStateVectorSim>(lib, lib->Clone(obj));

			return nullptr;
		}

		bool Sample(unsigned int nSamples, long int* samples, unsigned int nBits, int* bits)
		{
			if (obj)
				return lib->Sample(obj, nSamples, samples, nBits, bits);
			return false;
		}

		bool SampleAll(unsigned int nSamples, long int* samples)
		{
			if (obj)
				return lib->SampleAll(obj, nSamples, samples);

			return false;
		}

		bool Amplitude(long long int state, double* real, double* imaginary) const
		{
			if (obj)
				return lib->Amplitude(obj, state, real, imaginary);

			return false;
		}

		double Probability(int* qubits, int* mask, int len) const
		{
			if (obj)
				return lib->Probability(obj, qubits, mask, len);
			return 0.0;
		}

		double BasisStateProbability(long long int state) const
		{
			if (obj)
				return lib->BasisStateProbability(obj, state);
			return 0.0;
		}

		bool AllProbabilities(double* probabilities) const
		{
			if (obj)
				return lib->AllProbabilities(obj, probabilities);
			return false;
		}

		double ExpectationValue(const std::string& pauliString) const
		{
			if (obj)
				return lib->ExpectationValue(obj, pauliString.c_str(), pauliString.length());
			
			return 0.0;
		}

		bool ApplyX(int qubit)
		{
			if (obj)
				return lib->ApplyX(obj, qubit);

			return false;
		}

		bool ApplyY(int qubit)
		{
			if (obj)
				return lib->ApplyY(obj, qubit);

			return false;
		}

		bool ApplyZ(int qubit)
		{
			if (obj)
				return lib->ApplyZ(obj, qubit);

			return false;
		}

		bool ApplyH(int qubit)
		{
			if (obj)
				return lib->ApplyH(obj, qubit);

			return false;
		}

		bool ApplyS(int qubit)
		{
			if (obj)
				return lib->ApplyS(obj, qubit);

			return false;
		}

		bool ApplySDG(int qubit)
		{
			if (obj)
				return lib->ApplySDG(obj, qubit);

			return false;
		}

		bool ApplyT(int qubit)
		{
			if (obj)
				return lib->ApplyT(obj, qubit);

			return false;
		}

		bool ApplyTDG(int qubit)
		{
			if (obj)
				return lib->ApplyTDG(obj, qubit);

			return false;
		}

		bool ApplySX(int qubit)
		{
			if (obj)
				return lib->ApplySX(obj, qubit);

			return false;
		}

		bool ApplySXDG(int qubit)
		{
			if (obj)
				return lib->ApplySXDG(obj, qubit);

			return false;
		}

		bool ApplyK(int qubit)
		{
			if (obj)
				return lib->ApplyK(obj, qubit);

			return false;
		}

		bool ApplyP(int qubit, double theta)
		{
			if (obj)
				return lib->ApplyP(obj, qubit, theta);

			return false;
		}

		bool ApplyRx(int qubit, double theta)
		{
			if (obj)
				return lib->ApplyRx(obj, qubit, theta) == 1;

			return false;
		}

		bool ApplyRy(int qubit, double theta)
		{
			if (obj)
				return lib->ApplyRy(obj, qubit, theta);

			return false;
		}

		bool ApplyRz(int qubit, double theta)
		{
			if (obj)
				return lib->ApplyRz(obj, qubit, theta);

			return false;
		}

		bool ApplyU(int qubit, double theta, double phi, double lambda, double gamma)
		{
			if (obj)
				return lib->ApplyU(obj, qubit, theta, phi, lambda, gamma);

			return false;
		}

		bool ApplyCX(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCX(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCY(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCY(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCZ(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCZ(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCH(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCH(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCSX(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCSX(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCSXDG(int controlQubit, int targetQubit)
		{
			if (obj)
				return lib->ApplyCSXDG(obj, controlQubit, targetQubit);

			return false;
		}

		bool ApplyCP(int controlQubit, int targetQubit, double theta)
		{
			if (obj)
				return lib->ApplyCP(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRx(int controlQubit, int targetQubit, double theta)
		{
			if (obj)
				return lib->ApplyCRx(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRy(int controlQubit, int targetQubit, double theta)
		{
			if (obj)
				return lib->ApplyCRy(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCRz(int controlQubit, int targetQubit, double theta)
		{
			if (obj)
				return lib->ApplyCRz(obj, controlQubit, targetQubit, theta);

			return false;
		}

		bool ApplyCCX(int controlQubit1, int controlQubit2, int targetQubit)
		{
			if (obj)
				return lib->ApplyCCX(obj, controlQubit1, controlQubit2, targetQubit);

			return false;
		}

		bool ApplySwap(int qubit1, int qubit2)
		{
			if (obj)
				return lib->ApplySwap(obj, qubit1, qubit2);

			return false;
		}

		bool ApplyCSwap(int controlQubit, int qubit1, int qubit2)
		{
			if (obj)
				return lib->ApplyCSwap(obj, controlQubit, qubit1, qubit2);

			return false;
		}

		bool ApplyCU(int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma)
		{
			if (obj)
				return lib->ApplyCU(obj, controlQubit, targetQubit, theta, phi, lambda, gamma);

			return false;
		}

	private:
		std::shared_ptr<GpuLibrary> lib;
		void* obj;
	};
}

#endif

#endif

