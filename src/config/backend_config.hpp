#pragma once

#include <nlohmann/json.hpp>
#include <iostream>

#include "../utils/constants.hpp"
#include "../simulators/simulator.hpp"

using namespace std::literals;
using json = nlohmann::json;

namespace config {

template <SimType sim_type = SimType::Aer>
class BackendConfig {
public:
    std::string name;
    std::string version;
    std::string simulator;
    int n_qubits;
    std::string url;
    bool is_simulator;
    bool conditional;
    bool memory;
    int max_shots;
    std::string description;
    std::optional<std::string> coupling_map;
    std::string basis_gates;
    std::string custom_instructions;
    std::vector<std::string> gates;
    

    template<SimType T = sim_type,
             typename std::enable_if_t<is_same<T, SimType::Aer>::value, bool> = true>
    BackendConfig() : 
        name{"BasicAer"},
        version{"0.0.1"},
        simulator{"AerSimulator"},
        n_qubits{32},
        url{"https://github.com/Qiskit/qiskit-aer"},
        is_simulator{true},
        conditional{true},
        memory{true},
        max_shots{10000},
        description{"Usual AER simulator."}
    { }

    template<SimType T = sim_type,
             typename std::enable_if_t<is_same<T, SimType::Munich>::value, bool> = true>
    BackendConfig() :
        name{"BasicMunich"},
        version{"0.0.1"},
        simulator{"MunichSimulator"},
        n_qubits{32},
        url{"https://github.com/cda-tum/mqt-ddsim"},
        is_simulator{true},
        conditional{true},
        memory{true},
        max_shots{10000},
        description{"Usual Munich simulator."}
    { }

    template<SimType T = sim_type,
             typename std::enable_if_t<is_same<T, SimType::Aer>::value, bool> = true>
    BackendConfig(const json& config)
        : simulator{"AerSimulator"}
    {
        from_json(config, *this);
    }

    template<SimType T = sim_type,
             typename std::enable_if_t<is_same<T, SimType::Munich>::value, bool> = true>
    BackendConfig(const json& config) 
        : simulator{"MunichSimulator"}
    {
        from_json(config, *this);
    }
};

template <SimType sim_type>
void to_json(json& j, const BackendConfig<sim_type>& backend_conf)
{
    j = {   
            {"name", backend_conf.name}, 
            {"version", backend_conf.version},
            {"simulator", backend_conf.simulator},
            {"n_qubits", backend_conf.n_qubits}, 
            {"url", backend_conf.url},
            {"is_simulator", backend_conf.is_simulator},
            {"conditional", backend_conf.conditional}, 
            {"memory", backend_conf.memory},
            {"max_shots", backend_conf.max_shots},
            {"description", backend_conf.description},
            //{"coupling_map", backend_conf.coupling_map},
            {"basis_gates", backend_conf.basis_gates}, 
            {"custom_instructions", backend_conf.custom_instructions},
            //{"gates", backend_conf.gates}
        };
}

template <SimType sim_type>
void from_json(const json& j, BackendConfig<sim_type>& backend_conf) 
{
    j.at("name").get_to(backend_conf.name);
    j.at("version").get_to(backend_conf.version);
    j.at("simulator").get_to(backend_conf.simulator);
    j.at("n_qubits").get_to(backend_conf.n_qubits);
    j.at("url").get_to(backend_conf.url);
    j.at("is_simulator").get_to(backend_conf.is_simulator);
    j.at("conditional").get_to(backend_conf.conditional);
    j.at("memory").get_to(backend_conf.memory);
    j.at("max_shots").get_to(backend_conf.max_shots);
    j.at("description").get_to(backend_conf.description);
    //j.at("coupling_map").get_to(backend_conf.coupling_map);
    j.at("basis_gates").get_to(backend_conf.basis_gates);
    j.at("custom_instructions").get_to(backend_conf.custom_instructions);
    //j.at("gates").get_to(backend_conf.gates);
}; 

};

std::ostream& operator<<(std::ostream& os, const config::BackendConfig<SimType::Aer>& config) {
    /* os << "\nIPs: \n";
    for (const auto& net_bind : config.IPs) {
            os << net_bind.first << " ---> " << net_bind.second << "\n";
    }
    os << "\nPuerto: " << config.port
       << "\nHostname: " << config.hostname << "\n\n"; */
    return os;
}