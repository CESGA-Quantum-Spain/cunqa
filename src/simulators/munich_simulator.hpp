#pragma once

#include <nlohmann/json.hpp>
#include <chrono>
#include <optional>

#include "config/backend_config.hpp"
#include "CircuitSimulator.hpp"
#include "StochasticNoiseSimulator.hpp"
#include "ir/QuantumComputation.hpp"
#include "logger/logger.hpp"
#include "comm/qpu_comm.hpp"
#include "config/backend_config.hpp"
#include "utils/constants.hpp"
#include "simulator.hpp"

using json = nlohmann::json;


class MunichSimulator {
public:
    int munich_mpi_rank;
    MunichSimulator()
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &munich_mpi_rank);
        SPDLOG_LOGGER_DEBUG(logger, "munich_mpi_rank: {}", munich_mpi_rank);
    }
    
    void configure_simulator(json& backend_config)
    {
        SPDLOG_LOGGER_DEBUG(logger, "No configuration needed for MunichSimulator");
    }

    //Offloading execution
    json execute(json circuit_json, json& noise_model_json,  const config::RunConfig& run_config) 
    {
        try {
            SPDLOG_LOGGER_DEBUG(logger, "Noise JSON: {}", noise_model_json.dump(4));

            std::string circuit(circuit_json.at("instructions"));
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

    
    //Dynamic execution
    inline int _apply_measure(std::vector<int>& qubits)
    {
        SPDLOG_LOGGER_ERROR(logger, "Error. Dynamic execution is not available with Munich simulator. ");
        return -1;
    }
    
    inline void _apply_gate(std::string& gate_name, std::vector<int>& qubits, std::vector<double>& param)
    {
        SPDLOG_LOGGER_ERROR(logger, "Error. Dynamic execution is not available with Munich simulator. ");
    }

    inline int _get_statevector_nonzero_position()
    {
        SPDLOG_LOGGER_ERROR(logger, "Error. Dynamic execution is not available with Munich simulator. ");
        return -1;
    }

    inline void _reinitialize_statevector()
    {
        SPDLOG_LOGGER_ERROR(logger, "Error. Dynamic execution is not available with Munich simulator. ");
    }
};