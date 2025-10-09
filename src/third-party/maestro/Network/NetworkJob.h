/**
 * @file NetworkJob.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * A network job class.
 */

#pragma once

#ifndef  _NETWORK_JOB_H
#define _NETWORK_JOB_H

#include "../Utils/ThreadsPool.h"
#include "../Types.h"

namespace Network {

	template<typename Time = Types::time_type> class ExecuteJob
	{
	public:
		using ExecuteResults = typename Circuits::Circuit<Time>::ExecuteResults;

		ExecuteJob() = delete;

		explicit ExecuteJob(const std::shared_ptr<Circuits::Circuit<Time>>& c, ExecuteResults& r, size_t cnt, size_t nq, size_t nc, size_t ncr, Simulators::SimulatorType t, Simulators::SimulationType m, std::mutex& mut)
			: dcirc(c), res(r), curCnt(cnt), nrQubits(nq), nrCbits(nc), nrResultCbits(ncr), simType(t), method(m), resultsMutex(mut)
		{
		}

		void DoWork()
		{
			if (curCnt == 0) return;

			Circuits::OperationState state;
			state.AllocateBits(nrCbits);

			const bool hasMeasurementsOnlyAtEnd = !dcirc->HasOpsAfterMeasurements();
			const bool optimiseMultipleShots = optimiseMultipleShotsExecution;
			const bool specialOptimizationForStatevector = optimiseMultipleShots && method == Simulators::SimulationType::kStatevector && hasMeasurementsOnlyAtEnd;
			const bool specialOptimizationForMPS = optimiseMultipleShots && method == Simulators::SimulationType::kMatrixProductState && hasMeasurementsOnlyAtEnd;

			if (!optSim)
			{
				optSim = Simulators::SimulatorsFactory::CreateSimulator(simType, method);
				if (!optSim)
					return;

				if (!maxBondDim.empty()) optSim->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
				if (!singularValueThreshold.empty()) optSim->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
				if (!mpsSample.empty()) optSim->Configure("mps_sample_measure_algorithm", mpsSample.c_str());

				optSim->AllocateQubits(nrQubits);
				optSim->Initialize();

				if (optimiseMultipleShots)
				{
					executedGates = dcirc->ExecuteNonMeasurements(optSim, state);

					if (!specialOptimizationForStatevector && !specialOptimizationForMPS)
						optSim->SaveState();
				}
			}


			std::shared_ptr<Circuits::MeasurementOperation<Time>> measurementsOp;

			const std::vector<bool> executed = std::move(executedGates);

			if (optimiseMultipleShots && hasMeasurementsOnlyAtEnd)
			{
				measurementsOp = dcirc->GetLastMeasurements(executed, optSim->GetType() == Simulators::SimulatorType::kQiskitAer);

				const auto& qbits = measurementsOp->GetQubits();
				if (qbits.empty())
				{
					auto bits = state.GetAllBits();
					bits.resize(nrResultCbits, false);

					const std::lock_guard lock(resultsMutex);
					res[bits] += curCnt;

					return;
				}
			}

			ExecuteResults localRes;

			if (optimiseMultipleShots && (specialOptimizationForStatevector || hasMeasurementsOnlyAtEnd))
			{
				const auto& qbits = measurementsOp->GetQubits();

				const auto sampleres = optSim->SampleCounts(qbits, curCnt);

				for (const auto& [mstate, cnt] : sampleres)
				{
					measurementsOp->SetStateFromSample(mstate, state);

					auto bits = state.GetAllBits();
					bits.resize(nrResultCbits, false);

					localRes[bits] += cnt;

					state.Reset();
				}

				const std::lock_guard lock(resultsMutex);
				for (const auto& r : localRes)
					res[r.first] += r.second;

				return;
			}

			const auto curCnt1 = curCnt > 0 ? curCnt - 1 : 0;
			for (size_t i = 0; i < curCnt; ++i)
			{
				if (optimiseMultipleShots)
				{
					optSim->RestoreState();
					dcirc->ExecuteMeasurements(optSim, state, executed);
				}
				else
				{
					dcirc->Execute(optSim, state);
					if (i < curCnt1) optSim->Reset();
				}

				auto bits = state.GetAllBits();
				bits.resize(nrResultCbits, false);

				++localRes[bits];

				state.Reset();
			}

			const std::lock_guard lock(resultsMutex);
			for (const auto& r : localRes)
				res[r.first] += r.second;
		}

		void DoWorkNoLock()
		{
			if (curCnt == 0) return;

			Circuits::OperationState state;
			state.AllocateBits(nrCbits);

			const bool hasMeasurementsOnlyAtEnd = !dcirc->HasOpsAfterMeasurements();
			const bool optimiseMultipleShots = optimiseMultipleShotsExecution;
			const bool specialOptimizationForStatevector = optimiseMultipleShots && method == Simulators::SimulationType::kStatevector && hasMeasurementsOnlyAtEnd;
			const bool specialOptimizationForMPS = optimiseMultipleShots && method == Simulators::SimulationType::kMatrixProductState && hasMeasurementsOnlyAtEnd;

			if (optSim)
			{
				optSim->SetMultithreading(true);

				if (optSim->GetNumberOfQubits() != nrQubits)
				{
					optSim->Clear();

					if (!maxBondDim.empty()) optSim->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
					if (!singularValueThreshold.empty()) optSim->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
					if (!mpsSample.empty()) optSim->Configure("mps_sample_measure_algorithm", mpsSample.c_str());

					optSim->AllocateQubits(nrQubits);
					optSim->Initialize();
				
					if (optimiseMultipleShots)
					{
						executedGates = dcirc->ExecuteNonMeasurements(optSim, state);

						if (!specialOptimizationForStatevector && !specialOptimizationForMPS)
							optSim->SaveState();
					}
				}
			}
			else
			{
				optSim = Simulators::SimulatorsFactory::CreateSimulator(simType, method);
				if (!optSim)
					return;

				optSim->SetMultithreading(true);

				if (!maxBondDim.empty()) optSim->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
				if (!singularValueThreshold.empty()) optSim->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
				if (!mpsSample.empty()) optSim->Configure("mps_sample_measure_algorithm", mpsSample.c_str());

				optSim->AllocateQubits(nrQubits);
				optSim->Initialize();

				if (optimiseMultipleShots)
				{
					executedGates = dcirc->ExecuteNonMeasurements(optSim, state);

					if (!specialOptimizationForStatevector && !specialOptimizationForMPS)
						optSim->SaveState();
				}
			}

			std::shared_ptr<Circuits::MeasurementOperation<Time>> measurementsOp;

			const std::vector<bool> executed = std::move(executedGates);

			if (optimiseMultipleShots && hasMeasurementsOnlyAtEnd)
			{
				measurementsOp = dcirc->GetLastMeasurements(executed, optSim->GetType() == Simulators::SimulatorType::kQiskitAer);

				const auto& qbits = measurementsOp->GetQubits();
				if (qbits.empty())
				{
					auto bits = state.GetAllBits();
					bits.resize(nrResultCbits, false);

					res[bits] += curCnt;

					return;
				}
			}

			if (optimiseMultipleShots && (specialOptimizationForStatevector || hasMeasurementsOnlyAtEnd))
			{
				const auto& qbits = measurementsOp->GetQubits();

				const auto sampleres = optSim->SampleCounts(qbits, curCnt);

				for (const auto& [mstate, cnt] : sampleres)
				{
					measurementsOp->SetStateFromSample(mstate, state);

					auto bits = state.GetAllBits();
					bits.resize(nrResultCbits, false);

					res[bits] += cnt;

					state.Reset();
				}

				return;
			}

			const auto curCnt1 = curCnt > 0 ? curCnt - 1 : 0;
			for (size_t i = 0; i < curCnt; ++i)
			{
				if (optimiseMultipleShots)
				{
					optSim->RestoreState();
					dcirc->ExecuteMeasurements(optSim, state, executed);
				}
				else
				{
					dcirc->Execute(optSim, state);
					if (i < curCnt1) optSim->Reset(); // leave the simulator state for the last iteration
				}

				auto bits = state.GetAllBits();
				bits.resize(nrResultCbits, false);

				++res[bits];

				state.Reset();
			}
		}

		static bool IsOptimisableForMultipleShots(Simulators::SimulatorType t, size_t curCnt)
		{
			return curCnt > 1;
		}

		size_t GetJobCount() const
		{
			return curCnt;
		}

		const std::shared_ptr<Circuits::Circuit<Time>> dcirc;
		ExecuteResults& res;
		const size_t curCnt;
		const size_t nrQubits;
		const size_t nrCbits;
		const size_t nrResultCbits;

		const Simulators::SimulatorType simType;
		const Simulators::SimulationType method;
		std::mutex& resultsMutex;

		bool optimiseMultipleShotsExecution = true;
		std::shared_ptr<Simulators::ISimulator> optSim;
		std::vector<bool> executedGates;

		// only fill them if passing null simulator
		std::string maxBondDim;
		std::string singularValueThreshold;
		std::string mpsSample;
	};

}

#endif // ! _NETWORK_JOB_H
