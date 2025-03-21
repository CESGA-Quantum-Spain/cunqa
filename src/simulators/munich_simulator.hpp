#pragma once

#include "CircuitSimulator.hpp"
#include "StochasticNoiseSimulator.hpp"
#include "ir/QuantumComputation.hpp"
#include "logger/logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

class MunichSimulator {
public:
    static json execute(json circuit_json, json noise_model_json, const config::RunConfig& run_config) 
    {
        try {
            noise_model_json = noise_model_json;
            SPDLOG_LOGGER_DEBUG(logger, "Noise JSON: {}", noise_model_json.dump(4));

            std::string circuit(circuit_json["instructions"]);
            SPDLOG_LOGGER_DEBUG(logger, "Circuit JSON: {}", circuit);
            auto mqt_circuit = std::make_unique<qc::QuantumComputation>(std::move(qc::QuantumComputation::fromQASM(circuit)));

            json result_json;
            float time_taken;
            SPDLOG_LOGGER_DEBUG(logger, "Noise JSON: {}", noise_model_json.dump(4));

            if (!noise_model_json.empty()){
                const ApproximationInfo approx_info{noise_model_json["step_fidelity"], noise_model_json["approx_steps"], ApproximationInfo::FidelityDriven};
                StochasticNoiseSimulator sim(std::move(mqt_circuit), approx_info, run_config.seed, "APD", noise_model_json["noise_prob"],
                                                noise_model_json["noise_prob_t1"], noise_model_json["noise_prob_multi"]);
                auto start = std::chrono::high_resolution_clock::now();
                auto result = sim.simulate(run_config.shots);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> duration = end - start;
                time_taken = duration.count();
                !result.empty() ? result_json = json(result) : throw std::runtime_error("QASM format is not correct.");
            } else {
                CircuitSimulator sim(std::move(mqt_circuit));
                auto start = std::chrono::high_resolution_clock::now();
                auto result = sim.simulate(run_config.shots);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> duration = end - start;
                time_taken = duration.count();
                !result.empty() ? result_json = json(result) : throw std::runtime_error("QASM format is not correct.");
            }        

            SPDLOG_LOGGER_DEBUG(logger, "Results: {}", result_json.dump(4));
            return json({{"counts", result_json}, {"time_taken", time_taken}});
        } catch (const std::exception& e) {
            // TODO: specify the circuit format in the docs.
            SPDLOG_LOGGER_ERROR(logger, "Error executing the circuit in the Munich simulator.\nTry checking the format of the circuit sent and/or of the noise model.");
            return {{"ERROR", "\"" + std::string(e.what()) + "\""}};
        }
        return {};
    }
};