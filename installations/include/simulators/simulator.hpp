#pragma once

#include <unordered_map>
#include "aer_simulator.hpp"
#include "munich_simulator.hpp"

// Define Sim Types as a closed list
enum class SimType {
    Aer,
    Munich
};

const std::unordered_map<std::string, SimType> sim_names = {
    {"Aer", SimType::Aer},
    {"Munich", SimType::Munich}
};

template<SimType T, SimType U>
struct is_same : std::false_type {};
 
template<SimType T>
struct is_same<T, T> : std::true_type {};

// Depending on the SimType selected, a Simulator class is selected
template <SimType = SimType::Aer>
struct SimClass {
    using type = AerSimulator;
};

template <>
struct SimClass<SimType::Aer> {
    using type = AerSimulator;
};

template <>
struct SimClass<SimType::Munich> {
    using type = MunichSimulator;
};

/* 
template <SimType sim_type>
class Simulator {
    std::unique_ptr<typename SimClass<sim_type>::type> simulator;

public:

    Simulator() : simulator{std::make_unique<typename SimClass<sim_type>::type>()} { }

    inline json execute(json circuit_json, const config::RunConfig& run_config) {
        return simulator->execute(circuit_json, run_config);
    }

}; */