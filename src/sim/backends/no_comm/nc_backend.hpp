#pragma once

#include "sim/backend.hpp"
#include "nc_executor.hpp"

#include "logger.hpp"

namespace cunqa {
namespace sim {

class NCBackend final : public Backend {
public:
    JSON noise_model;

    NCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json)
    {
        name = "NCBackend";
        num_qubits = {15, 0};
        description = "Simple backend with no communications.";

        load_common_fields(backend_json, simulator.get());

        if (backend_json.contains("noise_model")) {
            noise_model = backend_json.at("noise_model");
            const std::string noise_properties_path = noise_model.at("noise_properties_path");

            auto noise_properties = read_file(noise_properties_path);
            for (const auto& [key, value] : noise_model.items()) {
                if (key != "noise_properties_path")
                    noise_properties[key] = value;
            }

            simulator->set_noise_model(noise_properties);
        }

        executor_.set_simulator(std::move(simulator));
    }

    JSON execute(const std::string& quantum_task_str) override
    {
        auto quantum_task = JSON::parse(quantum_task_str);

        std::string id;
        if (quantum_task.contains("id")) {
            id = quantum_task.at("id").get<std::string>();
            quantum_task.erase("id");
        }

        auto result = executor_.execute(quantum_task);
        if (!id.empty())
            result["id"] = id;

        return result;
    }

    JSON to_json() const override
    {
        auto j = base_json();
        j["noise_model"] = noise_model;
        return j;
    }

private:
    NCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace
