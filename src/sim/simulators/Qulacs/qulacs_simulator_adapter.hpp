#pragma once

#include <vector>
#include <string_view>

#include "src/sim/simulator.hpp"

namespace cunqa {
namespace sim {

class QulacsSimulatorAdapter final : public Simulator 
{
public:
    
    std::span<const std::string_view> get_basis_gates() const noexcept override 
    {
        return QULACS_BASIS_GATES;
    }

private:

static constexpr std::array<std::string_view, 49> QULACS_BASIS_GATES = {{
    "measure",
    "id", "x", "y", "z", "h", "s", "sdg", "t", "tdg", "p0", "p1", "sx", "sxdg", "sy", "sydg", "multipauli",
    "u1", "rx", "ry", "rz", "rotinvx", "rotinvy", "rotinvz", "rotx", "roty", "rotz", "parametricrx", "parametricry", "parametricrz", "multipaulirotation",
    "u2",
    "u3",
    "cx", "cz", "swap", "fusedswap", "ecr",
    "cp",
    "amplitudedampingnoise", "bitflipnoise", "densematrix", "dephasingnoise", "depolarizingnoise", "diagonalmatrix", "independentxznoise", "randomunitary", "sparsematrix", "twoqubitdepolarizingnoise",
}};

};


} // End of sim namespace
} // End of cunqa namespace