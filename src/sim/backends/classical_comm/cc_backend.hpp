#pragma once

#include "sim/backend.hpp"
#include "cc_executor.hpp"

namespace cunqa {
namespace sim {

class CCBackend final : public Backend {
public:
    CCBackend(std::unique_ptr<Simulator> simulator, const JSON& backend_json)
    {
        name = "CCBackend";
        num_qubits = {6, 0};
        description = "Backend with classical communications.";

        load_common_fields(backend_json, simulator.get());

        executor_.set_simulator(std::move(simulator));
    }

    inline JSON execute(const std::string& quantum_task_str) override
    {
        auto quantum_task = JSON::parse(quantum_task_str);

        if (quantum_task.contains("id")) {
            auto id = quantum_task.at("id").get<std::string>();
            quantum_task.erase("id");
        }

        return executor_.execute(quantum_task);
    }

    JSON to_json() const override
    {
        return base_json();
    }

private:
    CCExecutor executor_;
};

} // End of sim namespace
} // End of cunqa namespace
