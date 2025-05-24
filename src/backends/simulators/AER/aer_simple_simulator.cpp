

#include <string>
#include <optional>

#include "simulators/circuit_executor.hpp"
#include "framework/config.hpp"
#include "noise/noise_model.hpp"
#include "framework/circuit.hpp"
#include "controllers/controller_execute.hpp"
#include "framework/results/result.hpp"
#include "controllers/aer_controller.hpp"

#include "aer_helpers.hpp"
#include "aer_simple_simulator.hpp"

#include "logger.hpp"

using namespace AER;

namespace cunqa {
namespace sim {

AerSimpleSimulator::~AerSimpleSimulator() = default;

JSON AerSimpleSimulator::execute(const SimpleBackend& backend, const QuantumTask& quantum_task) const 
{
    try {
        //TODO: Maybe improve them to send several circuits at once
        auto aer_quantum_task = quantum_task_to_AER(quantum_task);
        JSON circuit_json = aer_quantum_task.circuit;

        LOGGER_DEBUG("Circuit {}.", circuit_json.dump(4));

        Circuit circuit(circuit_json);
        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        JSON run_config_json(aer_quantum_task.config);

        // TODO: See how to manage the seeds
        //run_config_json["seed_simulator"] = run_config.seed;
        Config aer_config(run_config_json);

        //LOGGER_DEBUG("circuit: {}.", backend.config.noise_model);
        Noise::NoiseModel noise_model(backend.config.noise_model);
        Result result = controller_execute<Controller>(circuits, noise_model, aer_config);

        return result.to_json();
    } catch (const std::exception& e) {
        // TODO: specify the circuit format in the docs.
        LOGGER_ERROR("Error executing the circuit in the AER simulator.\n\tTry checking the format of the circuit sent and/or of the noise model.");
        return {{"ERROR", std::string(e.what())}};
    }
    return {};
}

} // End of sim namespace
} // End of cunqa namespace