#pragma once

#include "simulators/circuit_executor.hpp"
#include "framework/json.hpp"
#include "framework/config.hpp"
#include "noise/noise_model.hpp"
#include "framework/circuit.hpp"
#include "controllers/controller_execute.hpp"
#include "framework/results/result.hpp"
#include "controllers/aer_controller.hpp"
#include <string>
#include "config/run_config.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std::literals;
using namespace AER;
using namespace config::run;

class AerSimulator {
public:

    // TODO: Añadir el modelo de ruido
    json execute(json circuit_json, const config::run::RunConfig& run_config) {

        Circuit circuit(circuit_json);
        Noise::NoiseModel noise_default;

        json run_config_json(run_config);
        std::cout << run_config_json.dump(4) << "\n";

        Config aer_default(run_config_json);


        std::vector<std::shared_ptr<Circuit>> circuits;
        circuits.push_back(std::make_shared<Circuit>(circuit));

        Result result = controller_execute<Controller>(circuits, noise_default, aer_default);

        return result.to_json();
    }

};