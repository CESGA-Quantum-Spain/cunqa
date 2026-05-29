#pragma once

#include <vector>

#include "circuit.hpp"

#include "parameters.hpp"
#include "instruction.hpp"

#include "utils/json.hpp"

namespace cunqa {

struct DynamicCircuit : public Circuit{
    std::vector<Instruction> instructions;
    Parameters params;

    explicit DynamicCircuit(const JSON& instructions_json);

    void update_params(const std::vector<double>& new_params) override
    {
        params.update_params(new_params);
    }
};

} // End of cunqa namespace
