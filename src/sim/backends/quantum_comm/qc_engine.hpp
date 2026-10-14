#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sim/simulator.hpp"
#include "sim/run_config.hpp"
#include "dynamic_circuit/dynamic_circuit.hpp"
#include "utils/json.hpp"

#include "comm_model.hpp"

namespace cunqa {
namespace sim {

struct QuantumTask {
    std::string id;
    RunConfig config;
    std::unique_ptr<DynamicCircuit> circuit;

    QuantumTask(std::string id, RunConfig config, std::unique_ptr<DynamicCircuit> circuit)
        : id(std::move(id))
        , config(std::move(config))
        , circuit(std::move(circuit))
    { };
};

// Simulates, shot by shot and on a single simulator, the circuits that the vQPUs of the
// quantum-communications scheme send in one round.
//
// When every vQPU has a "timing" block, or a communication model is set, each shot follows
// the simulated time of a SimClock per vQPU. With a communication model, entangled pairs
// come from the EPR generator of each link, arrive after a random generation time and
// decohere in the communication qubits until they are consumed, and every qubit that waits
// decoheres as well. Without any of them, entanglement is ideal and instantaneous.
class QCEngine {
public:
    QCEngine(std::shared_ptr<Simulator> simulator, CommModel model);

    // Runs every shot and returns, for each task (in order), its result JSON: counts,
    // time_taken and, in simulated time, simulated_time and comm_stats.
    std::vector<JSON> run(std::vector<QuantumTask>& quantum_tasks);

private:
    std::shared_ptr<Simulator> simulator_;
    CommModel model_;
};

} // End of sim namespace
} // End of cunqa namespace
