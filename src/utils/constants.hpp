#pragma once

#include <string_view>
#include <unordered_map>

// NETWORK INTERFACES NAMES
constexpr std::string_view INFINIBAND = "ib0";
constexpr std::string_view VLAN120 = "VLAN120";
constexpr std::string_view VLAN117 = "VLAN117";


enum GATES {
    MEASURE,
    ID,
    X,
    Y,
    Z,
    H,
    RX,
    RY,
    RZ,
    CX,
    CY,
    CZ
};

std::unordered_map<std::string, int> GATE_NAMES = {
    // MEASURE
    {"measure", MEASURE},

    // ONE GATE NO PARAM
    {"id", ID},
    {"x", X},
    {"y", Y},
    {"z", Z},
    {"h", H},

    // ONE GATE PARAM
    {"rx", RX},
    {"ry", RY},
    {"rz", RZ},

    // TWO GATE NO PARAM
    {"cx", CX},
    {"cy", CY},
    {"cz", CZ},
};

std::unordered_map<int, std::string> INVERTED_GATE_NAMES = {
    {MEASURE, "measure"},
    {ID, "id"},
    {X, "x"},
    {Y, "y"},
    {Z, "z"},
    {H, "h"},
    {RX, "rx"},
    {RY, "ry"},
    {RZ, "rz"},
    {CX, "cx"},
    {CY, "cy"},
    {CZ, "cz"},
};