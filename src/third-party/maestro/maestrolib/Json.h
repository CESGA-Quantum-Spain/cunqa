/**
 * @file Json.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The Json Parser class.
 *
 * A json parser class that parses a json input - in the CUNQA format.
 */

#pragma once

#ifndef _JSON_H_
#define _JSON_H_

#include <iostream>
#include <exception>
#include <fstream>
#include <boost/json.hpp>

#include "../Circuit/Factory.h"

namespace Json {

	template<typename Time = Types::time_type> class JsonParserMaestro {
	public:
		/**
		 * @brief Parses a string containing json.
		 *
		 * Parses a string containing json.
		 *
         * @param str[in] The string to parse.
		 * @return A json value containing the parsed json.
		 */
		static boost::json::value ParseString(const char* str)
		{
			boost::system::error_code ec;
			std::string strs(str);
			std::stringstream ss(strs);
			return Read(ss, ec);
		}


		std::shared_ptr<Circuits::Circuit<Time>> ParseCircuit(const char* str) const
		{
			const boost::json::value circuitJson = ParseString(str);

			if (!circuitJson.is_array())
				return nullptr;

			std::shared_ptr<Circuits::Circuit<Time>> circuit;

			const auto circuitArray = circuitJson.as_array();
			circuit = ParseCircuitArray(circuitArray);

			return circuit;
		}


		static std::string GetConfigString(const std::string& config, const boost::json::value& jsonConfig)
		{
			if (jsonConfig.is_object())
			{
				const auto jsonObject = jsonConfig.as_object();

				if (jsonObject.contains(config))
				{
					const auto configValue = jsonObject.at(config);
					if (configValue.is_string())
						return configValue.as_string().c_str();
					else if (configValue.is_number())
					{
						if (configValue.is_int64())
							return std::to_string(configValue.as_int64());
						else if (configValue.is_uint64())
							return std::to_string(configValue.as_uint64());
						else if (configValue.is_double())
							return std::to_string(configValue.as_double());
					}
					else if (configValue.is_bool())
					{
						return configValue.as_bool() ? "true" : "false";
					}
				}
			}

			return "";
		}

	private:
		/**
		 * @brief Parses a circuit array
		 *
		 * Parses a circuit json array.
		 *
		 * @param circuitArray The json array to parse.
		 * @return A shared pointer to the parsed circuit.
		 * @sa Circuits::Circuit
		 */
		std::shared_ptr<Circuits::Circuit<Time>> ParseCircuitArray(const boost::json::array& circuitArray) const
		{
			const auto circuit = std::make_shared<Circuits::Circuit<Time>>();

			for (auto operationJson : circuitArray)
			{
				if (!operationJson.is_object())
					throw std::runtime_error("Circuit operation must be an object.");

				auto operationObject = operationJson.as_object();
				if (!operationObject.contains(nameString))
					throw std::runtime_error("Circuit operation does not have a type.");
				else if (!operationObject.at(nameString).is_string())
					throw std::runtime_error("Circuit operation type must be a string.");

				const boost::json::string type = operationObject.at(nameString).as_string();

				if (type != "id")
					circuit->AddOperation(ParseOperation(type, operationObject));
			}

			return circuit;
		}

		/**
		 * @brief Parses an operation.
		 *
		 * Parses a circuit operation.
		 *
		 * @param type The type of operation to parse.
		 * @param obj The json object to parse.
		 * @return A shared pointer to the parsed operation.
		 * @sa Circuits::IOperation
		 */
		std::shared_ptr<Circuits::IOperation<Time>> ParseOperation(const boost::json::string& type, boost::json::object& obj) const
		{
			std::shared_ptr<Circuits::IOperation<Time>> operation;

			if (type == measurementString)
				operation = ParseMeasurement(obj); 
			else
				operation = ParseGate(type, obj);

			return operation;
		}

		/**
		 * @brief Parses a measurement
		 *
		 * Parses a measurement operation.
		 *
		 * @param obj The json object to parse.
		 * @return A shared pointer to the parsed measurement.
		 * @sa Circuits::MeasurementOperation
		 */
		std::shared_ptr<Circuits::MeasurementOperation<Time>> ParseMeasurement(boost::json::object& obj) const
		{
			const auto qubits = ParseQubits(obj);
			auto cbits = ParseCbits(obj);
			if (cbits.empty()) cbits = std::vector<size_t>(qubits.begin(), qubits.end());

			if (qubits.size() != cbits.size())
				throw std::runtime_error("Number of qubits and cbits must be the same.");

			std::vector<std::pair<Types::qubit_t, size_t>> qs;

			for (size_t i = 0; i < qubits.size(); i++)
				qs.push_back(std::make_pair(qubits[i], cbits[i]));

			const auto operation = std::static_pointer_cast<Circuits::MeasurementOperation<Time>>(Circuits::CircuitFactory<Time>::CreateMeasurement(qs));

			return operation;
		}

		/**
		 * @brief Parses a gate
		 *
		 * Parses a gate operation.
		 *
		 * @param obj The json object to parse.
		 * @return A shared pointer to the parsed gate.
		 * @sa Circuits::IGateOperation
		 */
		std::shared_ptr<Circuits::IOperation<Time>> ParseGate(const boost::json::string& type, boost::json::object& obj) const
		{
			const std::string gateName = type.c_str();
			
			const auto qubits = ParseQubits(obj);
			if (qubits.empty())
				throw std::runtime_error("No qubits specified.");
			else if (qubits.size() > 3)
				throw std::runtime_error("Number of qubits must be 1, 2, or 3.");

			Circuits::QuantumGateType gateType;

			if (gatesMap.find(gateName) == gatesMap.end())
				throw std::runtime_error("Gate type not supported.");

			gateType = gatesMap.at(gateName);

			// parse gate parameters
			const auto parameters = ParseParameters(obj);

			const auto operation = Circuits::CircuitFactory<Time>::CreateGate(gateType, qubits[0], qubits.size() > 1 ? qubits[1] : 0, qubits.size() > 2 ? qubits[2] : 0,
				parameters.size() > 0 ? parameters[0] : 0., parameters.size() > 1 ? parameters[1] : 0., parameters.size() > 2 ? parameters[2] : 0., parameters.size() > 3 ? parameters[3] : 0.);

			// check if the number of qubits is correct for the gate

			if (operation->GetNumQubits() != qubits.size())
				throw std::runtime_error("The specified number of qubits does not match the number of qubits required by the gate type.");

			// TODO: check parameters?

			// check if it's a classically controlled gate
			if (obj.contains("conditional_reg"))
			{
				auto cond = obj.at("conditional_reg");
				unsigned long long int cbit = 0;
				if (cond.is_uint64())
					cbit = cond.as_uint64();
				else if (cond.is_int64())
					cbit = static_cast<unsigned long long int>(cond.as_int64());
				else if (cond.is_array())
				{
					auto arr = cond.as_array();
					if (arr.size() != 1 || !arr[0].is_number())
						throw std::runtime_error("Conditional register must be a single integer.");

					if (arr[0].is_uint64())
						cbit = arr[0].as_uint64();
					else if (arr[0].is_int64())
						cbit = static_cast<unsigned long long int>(arr[0].as_int64());
					else
						throw std::runtime_error("Conditional register must be an integer or an array of one integer.");
				}
				else
					throw std::runtime_error("Conditional register must be an integer or an array of one integer.");

				return Circuits::CircuitFactory<Time>::CreateSimpleConditionalGate(operation, cbit);
			}

			return std::static_pointer_cast<Circuits::IOperation<Time>>(operation);
		}

		/**
		 * @brief Parses parameters
		 *
		 * Parses quantum gate parameters from a json object.
		 *
		 * @param obj The json object to parse.
		 * @return A vector containing the parsed parameters.
		 */
		std::vector<double> ParseParameters(boost::json::object& obj) const
		{
			std::vector<double> parameters;

			if (obj.contains(paramsString) && !obj.at(paramsString).is_array())
				throw std::runtime_error("Parameters must be an array.");

			auto paramsJson = (obj.contains(paramsString) ? obj.at(paramsString).as_array() : boost::json::array());
			for (auto param : paramsJson)
			{
				if (!param.is_number())
					throw std::runtime_error("Parameter must be a number.");
				else if (param.is_double())
					parameters.push_back(param.as_double());
				else if (param.is_int64())
					parameters.push_back(static_cast<double>(param.as_int64()));
				else
					parameters.push_back(static_cast<double>(param.as_uint64()));
			}

			return parameters;
		}

		/**
		 * @brief Parses qubits
		 *
		 * Parses qubits from a json object.
		 *
		 * @param obj The json object to parse.
		 * @return A vector containing the parsed qubits.
		 */
		std::vector<Types::qubit_t> ParseQubits(const boost::json::object& obj) const
		{
			std::vector<Types::qubit_t> qubits;

			if (!obj.contains(qubitsString))
				throw std::runtime_error("No Qubits specified.");
			else if (!obj.at(qubitsString).is_array())
				throw std::runtime_error("Qubits must be an array.");
			else if (obj.at(qubitsString).as_array().size() == 0)
				throw std::runtime_error("Qubits array must not be empty.");

			auto qubitsJson = obj.at(qubitsString).as_array();
			for (auto qubit : qubitsJson)
			{
				if (!qubit.is_int64() && !qubit.is_uint64())
					throw std::runtime_error("Number of qubits must be an integer.");
				else if (qubit.is_int64())
					qubits.push_back(qubit.as_int64());
				else
					qubits.push_back(qubit.as_uint64());
			}

			return qubits;
		}

		/**
		 * @brief Parses cbits
		 *
		 * Parses classical bits from a json object.
		 *
		 * @param obj The json object to parse.
		 * @param mustBeSpecified Whether or not the cbits must be specified in the json.
		 * @return A vector containing the parsed cbits.
		 */
		std::vector<size_t> ParseCbits(const boost::json::object& obj, bool mustBeSpecified = false) const
		{
			std::vector<size_t> cbits;

			if (mustBeSpecified)
			{
				if (!obj.contains(memoryString))
					throw std::runtime_error("No Cbits specified.");
				else if (!obj.at(memoryString).is_array())
					throw std::runtime_error("Cbits must be an array.");
				else if (obj.at(memoryString).as_array().size() == 0)
					throw std::runtime_error("Cbits array must not be empty.");
			}
			else if (obj.contains(memoryString) && !obj.at(memoryString).is_array())
				throw std::runtime_error("Cbits must be an array.");

			auto cbitsJson = (obj.contains(memoryString) ? obj.at(memoryString).as_array() : boost::json::array());
			for (auto cbit : cbitsJson)
			{
				if (!cbit.is_int64() && !cbit.is_uint64())
					throw std::runtime_error("Number of cbits must be an integer.");
				else if (cbit.is_int64())
					cbits.push_back(cbit.as_int64());
				else
					cbits.push_back(cbit.as_uint64());
			}

			return cbits;
		}

		/**
		 * @brief Parses a string containing json.
		 *
		 * Parses a string containing json.
		 *
		 * @param str[in] The string to parse.
		 * @param ec[out] The error code.
		 * @return A json value containing the parsed json.
		 */
		static boost::json::value ParseString(const std::string& str, boost::system::error_code& ec)
		{
			std::stringstream ss(str);
			return Read(ss, ec);
		}

		/**
		 * @brief Parses a json input stream.
		 *
		 * Parses a json input stream.
		 *
		 * @param is The stream to read from.
		 * @param ec[out] The error code.
		 * @return A json value containing the parsed json.
		 */
		static boost::json::value Parse(std::istream& is, boost::system::error_code& ec)
		{
			return Read(is, ec);
		}

		/**
		 * @brief Reads a json stream and parses it.
		 *
		 * Reads a json stream and parses it.
		 *
		 * @param is The stream to read from.
		 * @param ec[out] The error code.
		 * @return A json value containing the parsed json.
		 */
		static boost::json::value Read(std::istream& is, boost::system::error_code& ec)
		{
			boost::json::stream_parser p;
			std::string line;
			while (std::getline(is, line))
			{
				p.write(line, ec);
				if (ec)
					return nullptr;
			}
			p.finish(ec);
			if (ec)
				return nullptr;

			return p.release();
		}


		const std::string circuitString = "instructions";
		const std::string nameString = "name";
		const std::string measurementString = "measure";

		const std::string qubitsString = "qubits";
		const std::string paramsString = "params";
		const std::string memoryString = "clbits";

		const std::unordered_map<std::string, Circuits::QuantumGateType> gatesMap = {
			{ "p", Circuits::QuantumGateType::kPhaseGateType },
			{ "x", Circuits::QuantumGateType::kXGateType },
			{ "y", Circuits::QuantumGateType::kYGateType },
			{ "z", Circuits::QuantumGateType::kZGateType },
			{ "h", Circuits::QuantumGateType::kHadamardGateType },
			{ "s", Circuits::QuantumGateType::kSGateType },
			{ "sdg", Circuits::QuantumGateType::kSdgGateType },
			{ "t", Circuits::QuantumGateType::kTGateType },
			{ "tdg", Circuits::QuantumGateType::kTdgGateType },
			{ "sx", Circuits::QuantumGateType::kSxGateType },
			{ "sxdg", Circuits::QuantumGateType::kSxDagGateType },
			{ "k", Circuits::QuantumGateType::kKGateType },
			{ "rx", Circuits::QuantumGateType::kRxGateType },
			{ "ry", Circuits::QuantumGateType::kRyGateType },
			{ "rz", Circuits::QuantumGateType::kRzGateType },
			{ "u", Circuits::QuantumGateType::kUGateType },
			{ "u1", Circuits::QuantumGateType::kUGateType },
			{ "u2", Circuits::QuantumGateType::kUGateType },
			{ "u3", Circuits::QuantumGateType::kUGateType },
			{ "swap", Circuits::QuantumGateType::kSwapGateType },
			{ "cx", Circuits::QuantumGateType::kCXGateType },
			{ "cy", Circuits::QuantumGateType::kCYGateType },
			{ "cz", Circuits::QuantumGateType::kCZGateType },
			{ "cp", Circuits::QuantumGateType::kCPGateType },
			{ "crx", Circuits::QuantumGateType::kCRxGateType },
			{ "cry", Circuits::QuantumGateType::kCRyGateType },
			{ "crz", Circuits::QuantumGateType::kCRzGateType },
			{ "ch", Circuits::QuantumGateType::kCHGateType },
			{ "csx", Circuits::QuantumGateType::kCSxGateType },
			{ "csxdg", Circuits::QuantumGateType::kCSxDagGateType },
			{ "cu", Circuits::QuantumGateType::kCUGateType },
			{ "cswap", Circuits::QuantumGateType::kCSwapGateType },
			{ "ccx", Circuits::QuantumGateType::kCCXGateType } }; /**< A map between gate names and gate types */
	};

}

#endif // !_JSON_H_
