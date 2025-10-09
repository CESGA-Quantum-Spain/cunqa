/**
 * @file interface.cpp
 * @version 1.0
 *
 * @section DESCRIPTION
 * C interface implementation for the maestro library.
 */

#include "Interface.h"

#ifdef COMPOSER
#include "../../composer/composer/Estimators/ExecutionEstimator.h"
#endif

#include "../Simulators/Factory.h"

#include "Maestro.h"

#include "Json.h"

#include <boost/json/src.hpp>

#include <atomic>
#include <memory>

#include "../Utils/LogFile.h"

static std::atomic_bool isInitialized = false;
static std::unique_ptr<Maestro> maestroInstance = nullptr;

extern "C" void* GetMaestroObject()
{
	if (!isInitialized.exchange(true))
	{
#ifdef __linux__
		Simulators::SimulatorsFactory::InitGpuLibrary();
#endif

#ifdef COMPOSER
		Estimators::ExecutionEstimator<>::InitializeRegressors();
#endif

		maestroInstance = std::make_unique<Maestro>();
	}

	return (void*)maestroInstance.get();
}


extern "C" unsigned long int CreateSimpleSimulator(int nrQubits)
{
	if (!maestroInstance)
		return 0;

	return maestroInstance->CreateSimpleSimulator(nrQubits);
}

extern "C" void DestroySimpleSimulator(unsigned long int simHandle)
{
	if (!maestroInstance || simHandle == 0)
		return;

	maestroInstance->DestroySimpleSimulator(simHandle);
}

extern "C" int RemoveAllOptimizationSimulatorsAndAdd(unsigned long int simHandle, int simType, int simExecType)
{
	if (!maestroInstance || simHandle == 0)
		return 0;

	return maestroInstance->RemoveAllOptimizationSimulatorsAndAdd(simHandle, static_cast<Simulators::SimulatorType>(simType), static_cast<Simulators::SimulationType>(simExecType));
}

extern "C" int AddOptimizationSimulator(unsigned long int simHandle, int simType, int simExecType)
{
	if (!maestroInstance || simHandle == 0)
		return 0;

	return maestroInstance->AddOptimizationSimulator(simHandle, static_cast<Simulators::SimulatorType>(simType), static_cast<Simulators::SimulationType>(simExecType));
}

extern "C" char* SimpleExecute(unsigned long int simpleSim, const char* jsonCircuit, const char* jsonConfig)
{
	if (simpleSim == 0 || !jsonCircuit || !jsonConfig || !maestroInstance)
		return nullptr;

	auto network = maestroInstance->GetSimpleSimulator(simpleSim);

	// step 1: Parse the JSON circuit and configuration strings
	// convert the JSON circuit into a Circuit object

	// I'm unsure here on how it deals with the classical registers, more precisely
	// with stuff like "other_measure_name" and "meas" (see below)
	// since in the example it seems to just use the cbit number

	// This is the json format:
	// {"instructions": 
	// [{"name": "h", "qubits": [0], "params": []},
	// {"name": "cx", "qubits": [0, 1], "params": []}, 
	// {"name": "rx", "qubits": [0], "params": [0.39528385768119634]}, 
	// {"name": "measure", "qubits": [0], "memory": [0]}], 
	// 
	// "num_qubits": 2, "num_clbits": 4, 
	// "quantum_registers": {"q": [0, 1]}, 
	// "classical_registers": {"c": [0, 1], "other_measure_name": [2], "meas": [3]}}

	Json::JsonParserMaestro<> jsonParser;

	auto circuit = jsonParser.ParseCircuit(jsonCircuit);
	
	// check if the circuit has measurements only at the end

	// get the number of shots from the configuration
	size_t nrShots = 1; // default value

	const auto configJson = Json::JsonParserMaestro<>::ParseString(jsonConfig);

	if (configJson.is_object())
	{
		const auto configObject = configJson.as_object();
		// get whatever else is needed from the configuration
		// maybe simulator type, allowed simulator types, bond dimension limit, etc.

		// execute the circuit in the network object
		if (configObject.contains("shots") && configObject.at("shots").is_number())
		{
			auto number = configObject.at("shots");
			nrShots = number.is_int64() ? (size_t)number.as_int64() : (size_t)number.as_uint64();
		}
	}

	bool configured = false;

	const std::string maxBondDim = Json::JsonParserMaestro<>::GetConfigString("matrix_product_state_max_bond_dimension", configJson);
	if (!maxBondDim.empty())
	{
		configured = true;
		if (network->GetSimulator())
			network->GetSimulator()->Clear();
		network->Configure("matrix_product_state_max_bond_dimension", maxBondDim.c_str());
	}

	const std::string singularValueThreshold = Json::JsonParserMaestro<>::GetConfigString("matrix_product_state_truncation_threshold", configJson);
	if (!singularValueThreshold.empty())
	{
		configured = true;
		if (network->GetSimulator())
			network->GetSimulator()->Clear();
		network->Configure("matrix_product_state_truncation_threshold", singularValueThreshold.c_str());
	}
	
	const std::string mpsSample = Json::JsonParserMaestro<>::GetConfigString("mps_sample_measure_algorithm", configJson);
	if (!mpsSample.empty())
	{
		configured = true;
		if (network->GetSimulator())
			network->GetSimulator()->Clear();
		network->Configure("mps_sample_measure_algorithm", mpsSample.c_str());
	}

	if (configured || !network->GetSimulator())
		network->CreateSimulator();

	// TODO: get from config the allowed simulators types and so on, if set

	auto results = network->RepeatedExecuteOnHost(circuit, 0, nrShots);

	// convert the results into a JSON string
	// allocate memory for the result string and copy the JSON result into it
	// return the result string

	// TODO: Implement it!

	boost::json::object response;
	boost::json::object jsonResult;

	for (const auto& result : results)
	{
		boost::json::string bits;
		for (const auto bit : result.first)
			bits.append(bit ? "1" : "0");

		jsonResult[bits] = result.second;
	}
	response["counts"] = jsonResult;
	const std::string responseStr = boost::json::serialize(response);
	const size_t responseSize = responseStr.length();
	char* result = new char[responseSize + 1];

	//strncpy_s(result, responseSize, responseStr.c_str(), responseSize);
	const char* responseData = responseStr.c_str();
	std::copy(responseData, responseData + responseSize, result);

	result[responseSize] = 0; // ensure null-termination

	return result;
}

extern "C" void FreeResult(char* result)
{
	if (result)
		delete[] result;
}

extern "C" unsigned long int CreateSimulator(int simType, int simExecType)
{
	if (!maestroInstance)
		return 0;

	return maestroInstance->CreateSimulator(static_cast<Simulators::SimulatorType>(simType), static_cast<Simulators::SimulationType>(simExecType));
}

extern "C" void* GetSimulator(unsigned long int simHandle)
{
	if (!maestroInstance || simHandle == 0)
		return nullptr;
	return maestroInstance->GetSimulator(simHandle);
}

extern "C" void DestroySimulator(unsigned long int simHandle)
{
	if (!maestroInstance || simHandle == 0)
		return;
	maestroInstance->DestroySimulator(simHandle);
}

extern "C" int ApplyX(void* sim, int qubit)
{
	if (!sim)
		return 0;

	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyX(qubit);

	return 1;
}

extern "C" int  ApplyY(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyY(qubit);

	return 1;
}

extern "C" int  ApplyZ(void* sim, int qubit)
{
	if (!sim)
		return 0;

	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyZ(qubit);
	return 1;
}

extern "C" int  ApplyH(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyH(qubit);

	return 1;
}

extern "C" int  ApplyS(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyS(qubit);

	return 1;
}

extern "C" int  ApplySDG(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplySDG(qubit);

	return 1;
}

extern "C" int  ApplyT(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyT(qubit);

	return 1;
}

extern "C" int  ApplyTDG(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyTDG(qubit);

	return 1;
}

extern "C" int  ApplySX(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplySx(qubit);

	return 1;
}

extern "C" int  ApplySXDG(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplySxDAG(qubit);

	return 1;
}

extern "C" int  ApplyK(void* sim, int qubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyK(qubit);

	return 1;
}

extern "C" int  ApplyP(void* sim, int qubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyP(qubit, theta);

	return 1;
}

extern "C" int  ApplyRx(void* sim, int qubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyRx(qubit, theta);

	return 1;
}

extern "C" int  ApplyRy(void* sim, int qubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyRy(qubit, theta);

	return 1;
}

extern "C" int  ApplyRz(void* sim, int qubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyRz(qubit, theta);

	return 1;
}

extern "C" int  ApplyU(void* sim, int qubit, double theta, double phi, double lambda, double gamma)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyU(qubit, theta, phi, lambda, gamma);

	return 1;
}

extern "C" int  ApplyCX(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCX(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCY(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCY(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCZ(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCZ(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCH(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCH(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCSX(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCSx(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCSXDG(void* sim, int controlQubit, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCSxDAG(controlQubit, targetQubit);

	return 1;
}

extern "C" int  ApplyCP(void* sim, int controlQubit, int targetQubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCP(controlQubit, targetQubit, theta);

	return 1;
}

extern "C" int  ApplyCRx(void* sim, int controlQubit, int targetQubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCRx(controlQubit, targetQubit, theta);

	return 1;
}

extern "C" int  ApplyCRy(void* sim, int controlQubit, int targetQubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCRy(controlQubit, targetQubit, theta);

	return 1;
}

extern "C" int  ApplyCRz(void* sim, int controlQubit, int targetQubit, double theta)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCRz(controlQubit, targetQubit, theta);

	return 1;
}

extern "C" int  ApplyCCX(void* sim, int controlQubit1, int controlQubit2, int targetQubit)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCCX(controlQubit1, controlQubit2, targetQubit);

	return 1;
}

extern "C" int  ApplySwap(void* sim, int qubit1, int qubit2)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplySwap(qubit1, qubit2);

	return 1;
}

extern "C" int  ApplyCSwap(void* sim, int controlQubit, int qubit1, int qubit2)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCSwap(controlQubit, qubit1, qubit2);

	return 1;
}

extern "C" int  ApplyCU(void* sim, int controlQubit, int targetQubit, double theta, double phi, double lambda, double gamma)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->ApplyCU(controlQubit, targetQubit, theta, phi, lambda, gamma);

	return 1;
}

extern "C" int InitializeSimulator(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->Initialize();
	return 1;
}

extern "C" int ResetSimulator(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->Reset();
	return 1;
}

extern "C" int ConfigureSimulator(void* sim, const char* key, const char* value)
{
	if (!sim || !key || !value)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->Configure(key, value);
	return 1;
}

extern "C" char* GetConfiguration(void* sim, const char* key)
{
	if (!sim || !key)
		return nullptr;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	std::string value = simulator->GetConfiguration(key);
	if (value.empty())
		return nullptr;
	// allocate memory for the result string and copy the configuration value into it
	const size_t valueSize = value.length();
	char* result = new char[valueSize + 1];
	std::copy(value.c_str(), value.c_str() + valueSize, result);
	result[valueSize] = 0; // ensure null-termination
	return result;
}

extern "C" unsigned long int AllocateQubits(void* sim, unsigned long int nrQubits)
{
	if (!sim || nrQubits == 0)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	const size_t res = simulator->AllocateQubits(nrQubits);

	return static_cast<unsigned long int>(res);
}

extern "C" unsigned long int GetNumberOfQubits(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	const size_t res = simulator->GetNumberOfQubits();
	return static_cast<unsigned long int>(res);
}

extern "C" int ClearSimulator(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->Clear();
	return 1;
}

extern "C" unsigned long long int Measure(void* sim, const unsigned long int* qubits, unsigned long int nrQubits)
{
	if (!sim || !qubits || nrQubits == 0)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	Types::qubits_vector qubitVector(qubits, qubits + nrQubits);
	const size_t res = simulator->Measure(qubitVector);
	return static_cast<unsigned long long int>(res);
}

extern "C" int ApplyReset(void* sim, const unsigned long int* qubits, unsigned long int nrQubits)
{
	if (!sim || !qubits || nrQubits == 0)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	Types::qubits_vector qubitVector(qubits, qubits + nrQubits);
	simulator->ApplyReset(qubitVector);
	return 1;
}

extern "C" double Probability(void* sim, unsigned long long int outcome)
{
	if (!sim)
		return 0.0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	const double res = simulator->Probability(outcome);
	return res;
}

extern "C" void FreeDoubleVector(double* vec)
{
	if (vec)
		delete[] vec;
}

extern "C" void FreeULLIVector(unsigned long long int* vec)
{
	if (vec)
		delete[] vec;
}

extern "C" double* Amplitude(void* sim, unsigned long long int outcome)
{
	if (!sim)
		return nullptr;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	const std::complex<double> amp = simulator->Amplitude(outcome);

	double* result = new double[2];
	result[0] = amp.real();
	result[1] = amp.imag();
	return result;
}

extern "C" double* AllProbabilities(void* sim)
{
	if (!sim)
		return nullptr;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	const auto probabilities = simulator->AllProbabilities();
	
	double* result = new double[probabilities.size()];
	std::copy(probabilities.begin(), probabilities.end(), result);
	return result;
}

extern "C" double* Probabilities(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits)
{
	if (!sim || !qubits || nrQubits == 0)
		return nullptr;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	Types::qubits_vector qubitVector(qubits, qubits + nrQubits);
	const auto probabilities = simulator->Probabilities(qubitVector);

	double* result = new double[probabilities.size()];
	std::copy(probabilities.begin(), probabilities.end(), result);
	return result;
}

extern "C" unsigned long long int* SampleCounts(void* sim, const unsigned long long int* qubits, unsigned long int nrQubits, unsigned long int shots)
{
	if (!sim || !qubits || nrQubits == 0 || shots == 0)
		return nullptr;
	
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	Types::qubits_vector qubitVector(qubits, qubits + nrQubits);
	const auto counts = simulator->SampleCounts(qubitVector, shots);

	unsigned long long int* result = new unsigned long long int[counts.size() * 2];
	size_t index = 0;
	for (const auto& count : counts)
	{
		result[index] = count.first; // outcome
		++index;
		result[index] = count.second; // count
		++index;
	}
	return result;
}

extern "C" int GetSimulatorType(void* sim)
{
	if (!sim)
		return -1;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	return static_cast<int>(simulator->GetType());
}

extern "C" int GetSimulationType(void* sim)
{
	if (!sim)
		return -1;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	return static_cast<int>(simulator->GetSimulationType());
}

extern "C" int FlushSimulator(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->Flush();
	return 1;
}

extern "C" int SaveStateToInternalDestructive(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->SaveStateToInternalDestructive();
	return 1;
}

extern "C" int RestoreInternalDestructiveSavedState(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->RestoreInternalDestructiveSavedState();
	return 1;
}

extern "C" int SaveState(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->SaveState();
	return 1;
}

extern "C" int RestoreState(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->RestoreState();
	return 1;
}

extern "C" int SetMultithreading(void* sim, int multithreading)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	simulator->SetMultithreading(multithreading != 0);
	return 1;
}

extern "C" int GetMultithreading(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	return simulator->GetMultithreading() ? 1 : 0;
}

extern "C" int IsQcsim(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	return simulator->IsQcsim() ? 1 : 0;
}

extern "C" unsigned long long int MeasureNoCollapse(void* sim)
{
	if (!sim)
		return 0;
	auto simulator = static_cast<Simulators::ISimulator*>(sim);
	return static_cast<unsigned long long int>(simulator->MeasureNoCollapse());
}










