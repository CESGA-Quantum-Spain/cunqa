#pragma once

#include <vector>
#include <string_view>

#include "src/sim/simulator.hpp"

namespace cunqa {
namespace sim {

class CunqaSimulatorAdapter final : public Simulator 
{
public:
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return CUNQA_BASIS_GATES;
    }

private:

static constexpr std::array<std::string_view, 18> CUNQA_BASIS_GATES = {
  "measure", 
  "id", "x", "y", "z", "h", "sx",
  "rx", "ry", "rz", 
  "swap", "cx", "cy", "cz", "ecr", 
  "crx", "cry", "crz",
};

};


} // End of sim namespace
} // End of cunqa namespace