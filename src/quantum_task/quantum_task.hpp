#pragma once

#include <vector>
#include <variant>
#include <string>

#include "circuit.hpp"
#include "run_config.hpp"

namespace cunqa {

class QuantumTask {
public:
    std::string id;
    RunConfig config;
    Circuit circuit;

    QuantumTask() = default;

    QuantumTask(std::string id, RunConfig config, Circuit&& circuit)
        : id(std::move(id))
        , config(std::move(config))
        , circuit(std::move(circuit))
    { };

    inline void update_params(const std::vector<double>& params)
    {
        if (params.size() != circuit.params.size())
            throw std::runtime_error("Number of circuit parameters and of new parameters does not match.");

        for (std::size_t i = 0; i < params.size(); ++i)
            *circuit.params[i] = params[i];
    }
};

} // End of cunqa namespace