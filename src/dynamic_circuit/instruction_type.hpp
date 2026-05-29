#pragma once

#include <array>
#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <complex>
#include <string_view>
#include <stdexcept>

namespace cunqa {

using DiagonalMatrix = std::vector<std::complex<double>>;
using Matrix = std::vector<std::vector<std::complex<double>>>;


enum class InstructionType {
        ID,
    X,
    Y,
    Z,
    H,
    S,
    SDG,
    T,
    TDG,
    SX,
    SY, 
    SZ,
    SXDG,
    SYDG,
    SZDG,
    P0,
    P1,
    V,
    VDG,
    K,
    HZ2,

    RX,
    RY,
    RZ,
    RAXIS,
    GLOBALP,
    P,
    U1,
    ROTX,
    ROTY,
    ROTZ,
    ROTINVX,
    ROTINVY,
    ROTINVZ,

    U2,
    R,

    U3,

    U,

    ID2,
    ECR,
    SWAP,
    ISWAP,
    SQRTSWAP,
    CX,
    CY,
    CZ,
    CH,
    CSX,
    CSXDG,
    CS,
    CSDG,
    CT,
    DCX,
    
    CRX,
    CRY,
    CRZ,
    CRAXIS,
    CP,
    CU1,
    RXX,
    RYY,
    RZZ,
    RXY,
    RZX,

    CU2,
    XXMYY,
    XXPYY,
    FS,

    CU3,

    CU,

    CECR,
    CSWAP,
    CSQRTSWAP,
    CCX,
    CCY,
    CCZ,

    MX,
    CMX,

    PHASEGADGET,
    CPHASEGADGET,

    MCX,
    MCY,
    MCZ,
    MCSX,
    MCS,
    MCT,
    MCH,
    MCSWAP,
    MCSQRTSWAP,

    MCRX,
    MCRY,
    MCRZ,
    MCRAXIS,
    MCP,
    MCU1,
    MCU2,
    MCU3,
    MCU,

    MCMX,

    MCPAULISTR,
    MCPAULIGADGET,
    MCPHASEGADGET,

    UNITARY,
    CUNITARY,
    SPARSEMATRIX,
    DIAGONAL,
    RANDOMUNITARY,
    FUSEDSWAP,
    MULTIPAULI,
    MULTIPAULIROTATION,

    PAULISTR,
    CPAULISTR,
    PAULIGADGET,
    NONUNITARYPAULIGADGET,
    CPAULIGADGET,

    AMPLITUDEDAMPINGNOISE,
    BITFLIPNOISE,
    DEPHASINGNOISE,
    DEPOLARIZINGNOISE,
    INDEPENDENTXZNOISE,
    TWOQUBITDEPOLARIZINGNOISE,

    SEND,
    RECV,
    GENENT,

    CIF,
    ENDCIF,
    COPY,
    RESET,
    SAVE_STATE,
    MEASURE
};

struct InstructionTypeEntry {
    std::string_view name;
    InstructionType type;
};

inline constexpr std::array INSTRUCTION_TYPE_ENTRIES{
    // ONE QUBIT NO PARAM
    InstructionTypeEntry{"id", InstructionType::ID},
    InstructionTypeEntry{"x", InstructionType::X},
    InstructionTypeEntry{"y", InstructionType::Y},
    InstructionTypeEntry{"z", InstructionType::Z},
    InstructionTypeEntry{"h", InstructionType::H},
    InstructionTypeEntry{"s", InstructionType::S},
    InstructionTypeEntry{"sdg", InstructionType::SDG},
    InstructionTypeEntry{"t", InstructionType::T},
    InstructionTypeEntry{"tdg", InstructionType::TDG},
    InstructionTypeEntry{"sx", InstructionType::SX},
    InstructionTypeEntry{"sy", InstructionType::SY},
    InstructionTypeEntry{"sz", InstructionType::SZ},
    InstructionTypeEntry{"sxdg", InstructionType::SXDG},
    InstructionTypeEntry{"sydg", InstructionType::SYDG},
    InstructionTypeEntry{"szdg", InstructionType::SZDG},
    InstructionTypeEntry{"p0", InstructionType::P0},
    InstructionTypeEntry{"p1", InstructionType::P1},
    InstructionTypeEntry{"v", InstructionType::V},
    InstructionTypeEntry{"vdg", InstructionType::VDG},
    InstructionTypeEntry{"k", InstructionType::K},
    InstructionTypeEntry{"hz2", InstructionType::HZ2},

    // ONE QUBIT ONE PARAM
    InstructionTypeEntry{"rx", InstructionType::RX},
    InstructionTypeEntry{"ry", InstructionType::RY},
    InstructionTypeEntry{"rz", InstructionType::RZ},
    InstructionTypeEntry{"raxis", InstructionType::RAXIS},
    InstructionTypeEntry{"gp", InstructionType::GLOBALP},
    InstructionTypeEntry{"p", InstructionType::P},
    InstructionTypeEntry{"u1", InstructionType::U1},
    InstructionTypeEntry{"rotx", InstructionType::ROTX},
    InstructionTypeEntry{"roty", InstructionType::ROTY},
    InstructionTypeEntry{"rotz", InstructionType::ROTZ},
    InstructionTypeEntry{"rotinvx", InstructionType::ROTINVX},
    InstructionTypeEntry{"rotinvy", InstructionType::ROTINVY},
    InstructionTypeEntry{"rotinvz", InstructionType::ROTINVZ},

    // ONE QUBIT TWO PARAM
    InstructionTypeEntry{"u2", InstructionType::U2},
    InstructionTypeEntry{"r", InstructionType::R},

    // ONE QUBIT THREE PARAM
    InstructionTypeEntry{"u3", InstructionType::U3},

    // ONE QUBIT FOUR PARAM
    InstructionTypeEntry{"u", InstructionType::U},

    // TWO QUBIT NO PARAM
    InstructionTypeEntry{"id2", InstructionType::ID2},
    InstructionTypeEntry{"ecr", InstructionType::ECR},
    InstructionTypeEntry{"swap", InstructionType::SWAP},
    InstructionTypeEntry{"iswap", InstructionType::ISWAP},
    InstructionTypeEntry{"sqrtswap", InstructionType::SQRTSWAP},
    InstructionTypeEntry{"cx", InstructionType::CX},
    InstructionTypeEntry{"cy", InstructionType::CY},
    InstructionTypeEntry{"cz", InstructionType::CZ},
    InstructionTypeEntry{"ch", InstructionType::CH},
    InstructionTypeEntry{"csx", InstructionType::CSX},
    InstructionTypeEntry{"csxdg", InstructionType::CSXDG},
    InstructionTypeEntry{"cs", InstructionType::CS},
    InstructionTypeEntry{"csdg", InstructionType::CSDG},
    InstructionTypeEntry{"ct", InstructionType::CT},
    InstructionTypeEntry{"dcx", InstructionType::DCX},

    // TWO QUBIT ONE PARAM
    InstructionTypeEntry{"crx", InstructionType::CRX},
    InstructionTypeEntry{"cry", InstructionType::CRY},
    InstructionTypeEntry{"crz", InstructionType::CRZ},
    InstructionTypeEntry{"craxis", InstructionType::CRAXIS},
    InstructionTypeEntry{"cp", InstructionType::CP},
    InstructionTypeEntry{"cu1", InstructionType::CU1},
    InstructionTypeEntry{"rxx", InstructionType::RXX},
    InstructionTypeEntry{"ryy", InstructionType::RYY},
    InstructionTypeEntry{"rzz", InstructionType::RZZ},
    InstructionTypeEntry{"rxy", InstructionType::RXY},
    InstructionTypeEntry{"rzx", InstructionType::RZX},

    // TWO QUBIT TWO PARAM
    InstructionTypeEntry{"cu2", InstructionType::CU2},
    InstructionTypeEntry{"xxmyy", InstructionType::XXMYY},
    InstructionTypeEntry{"xxpyy", InstructionType::XXPYY},
    InstructionTypeEntry{"fs", InstructionType::FS},

    // TWO QUBIT THREE PARAM
    InstructionTypeEntry{"cu3", InstructionType::CU3},

    // TWO QUBIT FOUR PARAM
    InstructionTypeEntry{"cu", InstructionType::CU},

    // THREE QUBIT NO PARAM
    InstructionTypeEntry{"cecr", InstructionType::CECR},
    InstructionTypeEntry{"cswap", InstructionType::CSWAP},
    InstructionTypeEntry{"csqrtswap", InstructionType::CSQRTSWAP},
    InstructionTypeEntry{"ccx", InstructionType::CCX},
    InstructionTypeEntry{"ccy", InstructionType::CCY},
    InstructionTypeEntry{"ccz", InstructionType::CCZ},

    // MULTI-QUBIT NO PARAM
    InstructionTypeEntry{"mx", InstructionType::MX},
    InstructionTypeEntry{"cmx", InstructionType::CMX},

    // PHASE GADGETS
    InstructionTypeEntry{"phasegadget", InstructionType::PHASEGADGET},
    InstructionTypeEntry{"cphasegadget", InstructionType::CPHASEGADGET},

    // MULTICONTROLLED NO PARAM
    InstructionTypeEntry{"mcx", InstructionType::MCX},
    InstructionTypeEntry{"mcy", InstructionType::MCY},
    InstructionTypeEntry{"mcz", InstructionType::MCZ},
    InstructionTypeEntry{"mcsx", InstructionType::MCSX},
    InstructionTypeEntry{"mcs", InstructionType::MCS},
    InstructionTypeEntry{"mct", InstructionType::MCT},
    InstructionTypeEntry{"mch", InstructionType::MCH},
    InstructionTypeEntry{"mcswap", InstructionType::MCSWAP},
    InstructionTypeEntry{"mcsqrtswap", InstructionType::MCSQRTSWAP},

    // MULTICONTROLLED WITH PARAM
    InstructionTypeEntry{"mcrx", InstructionType::MCRX},
    InstructionTypeEntry{"mcry", InstructionType::MCRY},
    InstructionTypeEntry{"mcrz", InstructionType::MCRZ},
    InstructionTypeEntry{"mcraxis", InstructionType::MCRAXIS},
    InstructionTypeEntry{"mcp", InstructionType::MCP},
    InstructionTypeEntry{"mcu1", InstructionType::MCU1},
    InstructionTypeEntry{"mcu2", InstructionType::MCU2},
    InstructionTypeEntry{"mcu3", InstructionType::MCU3},
    InstructionTypeEntry{"mcu", InstructionType::MCU},

    // MULTICONTROLLED SPECIAL
    InstructionTypeEntry{"mcmx", InstructionType::MCMX},
    InstructionTypeEntry{"mcpaulistr", InstructionType::MCPAULISTR},
    InstructionTypeEntry{"mcpauligadget", InstructionType::MCPAULIGADGET},
    InstructionTypeEntry{"mcphasegadget", InstructionType::MCPHASEGADGET},

    // SPECIAL UNITARY GATES
    InstructionTypeEntry{"unitary", InstructionType::UNITARY},
    InstructionTypeEntry{"cunitary", InstructionType::CUNITARY},
    InstructionTypeEntry{"sparsematrix", InstructionType::SPARSEMATRIX},
    InstructionTypeEntry{"diagonal", InstructionType::DIAGONAL},
    InstructionTypeEntry{"randomunitary", InstructionType::RANDOMUNITARY},
    InstructionTypeEntry{"fusedswap", InstructionType::FUSEDSWAP},
    InstructionTypeEntry{"multipauli", InstructionType::MULTIPAULI},
    InstructionTypeEntry{"multipaulirotation", InstructionType::MULTIPAULIROTATION},

    // PAULI RELATED GATES
    InstructionTypeEntry{"paulistr", InstructionType::PAULISTR},
    InstructionTypeEntry{"cpaulistr", InstructionType::CPAULISTR},
    InstructionTypeEntry{"pauligadget", InstructionType::PAULIGADGET},
    InstructionTypeEntry{"nonunitarypauligadget", InstructionType::NONUNITARYPAULIGADGET},
    InstructionTypeEntry{"cpauligadget", InstructionType::CPAULIGADGET},

    // NOISE RELATED GATES
    InstructionTypeEntry{"amplitudedampingnoise", InstructionType::AMPLITUDEDAMPINGNOISE},
    InstructionTypeEntry{"bitflipnoise", InstructionType::BITFLIPNOISE},
    InstructionTypeEntry{"dephasingnoise", InstructionType::DEPHASINGNOISE},
    InstructionTypeEntry{"depolarizingnoise", InstructionType::DEPOLARIZINGNOISE},
    InstructionTypeEntry{"independentxznoise", InstructionType::INDEPENDENTXZNOISE},
    InstructionTypeEntry{"twoqubitdepolarizingnoise", InstructionType::TWOQUBITDEPOLARIZINGNOISE},

    // CLASSICAL COMMUNICATION DIRECTIVES
    InstructionTypeEntry{"send", InstructionType::SEND},
    InstructionTypeEntry{"recv", InstructionType::RECV},

    // QUANTUM COMMUNICATION DIRECTIVES
    InstructionTypeEntry{"gen_ent", InstructionType::GENENT},

    // NON-UNITARY OPERATIONS
    InstructionTypeEntry{"cif", InstructionType::CIF},
    InstructionTypeEntry{"endcif", InstructionType::ENDCIF},
    InstructionTypeEntry{"copy", InstructionType::COPY},
    InstructionTypeEntry{"reset", InstructionType::RESET},
    InstructionTypeEntry{"save_state", InstructionType::SAVE_STATE},
    InstructionTypeEntry{"measure", InstructionType::MEASURE},
};

inline InstructionType instruction_type_from_name(std::string_view name)
{
    for (const auto& entry : INSTRUCTION_TYPE_ENTRIES) {
        if (entry.name == name)
            return entry.type;
    }
    throw std::invalid_argument{"Unknown instruction type: " + std::string{name}};
}

inline std::string_view instruction_type_name(InstructionType type)
{
    for (const auto& entry : INSTRUCTION_TYPE_ENTRIES) {
        if (entry.type == type)
            return entry.name;
    }
    throw std::invalid_argument{"Unknown InstructionType value."};
}

} // namespace cunqa