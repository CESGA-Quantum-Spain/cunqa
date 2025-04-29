#include <iostream>
#include <fstream>
#include <memory>
#include <optional>

#include "utils/json.hpp"
#include "utils/constants.hpp"
#include "config/backend_config.hpp"
#include "simulators/simulator.hpp"
#include "comm/qpu_comm.hpp"

using namespace config;


template <SimType sim_type>
class Backend {
private:
    std::unique_ptr<typename SimClass<sim_type>::type> simulator;
public:
    BackendConfig<sim_type> backend_config;

    Backend();
    Backend(BackendConfig<sim_type>& backend_config);

    inline cunqa::JSON run(cunqa::JSON& circuit_json);
    inline int apply_measure(std::array<int, 3>& qubits);
    inline void apply_gate(std::string& instruction_name, std::array<int, 3>& qubits, std::vector<double> param = {0.0});
    inline int get_shot_result();
    inline void restart_statevector();

};

template <SimType sim_type>
Backend<sim_type>::Backend() : backend_config{}, simulator{std::make_unique<typename SimClass<sim_type>::type>()}
{}

template <SimType sim_type>
Backend<sim_type>::Backend(BackendConfig<sim_type>& backend_config) : backend_config{backend_config}, simulator{std::make_unique<typename SimClass<sim_type>::type>()}
{
    cunqa::JSON backend_config_json = this->backend_config;
    this->simulator->configure_simulator(backend_config_json);
}

//Offloading wrappers
template <SimType sim_type>
inline cunqa::JSON Backend<sim_type>::run(cunqa::JSON& circuit_json)
{
    try {
        return this->simulator->execute(circuit_json, this->backend_config.noise_model, config::RunConfig(circuit_json.at("config")));
        //return SimClass<sim_type>::type::execute(circuit_json, this->backend_config.noise_model, config::RunConfig(circuit_json.at("config")));
        
    } catch (const std::exception& e) {
        LOGGER_ERROR("Error parsing the run configuration - {}", e.what());
        return {};
    }
} 

//Dynamic wrapper
template <SimType sim_type>
inline int Backend<sim_type>::apply_measure(std::array<int, 3>& qubits)
{
    return this->simulator->_apply_measure(qubits);
    //return SimClass<sim_type>::type::_apply_measure(qubits);
}

template <SimType sim_type>
inline void Backend<sim_type>::apply_gate(std::string& gate_name, std::array<int, 3>& qubits, std::vector<double> param)
{
    this->simulator->_apply_gate(gate_name, qubits, param);
    //SimClass<sim_type>::type::_apply_gate(gate_name, qubits, param);
}

template <SimType sim_type>
inline int Backend<sim_type>::get_shot_result()
{
    return this->simulator->_get_statevector_nonzero_position();
    //return SimClass<sim_type>::type::_get_statevector_nonzero_position();
}

template <SimType sim_type>
inline void Backend<sim_type>::restart_statevector()
{
    this->simulator->_reinitialize_statevector();
    //SimClass<sim_type>::type::_reinitialize_statevector();
}

