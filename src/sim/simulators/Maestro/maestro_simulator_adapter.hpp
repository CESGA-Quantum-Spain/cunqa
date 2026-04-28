#pragma once

#include <vector>
#include <string_view>

#include "src/sim/simulator.hpp"

namespace cunqa {
namespace sim {

class MaestroSimulatorAdapter final : public Simulator 
{
public:
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return MAESTRO_BASIS_GATES;
    }

private:

static constexpr std::array<std::string_view, 32> MAESTRO_BASIS_GATES = {{
    "measure",
    "x", "y", "z", "h", "s", "sdg", "t", "tdg", "sx", "sxdg", "k",
    "p", "rx", "ry", "rz",
    "u",
    "cx", "cy", "cz", "ch", "csx", "csxdg", "swap",
    "cp", "crx", "cry", "crz",
    "ccx", "cswap",
    "cu",
    "reset",
}};

};


} // End of sim namespace
} // End of cunqa namespace