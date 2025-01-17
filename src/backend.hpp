#include <iostream>
#include <fstream>
#include <memory>
#include "utils/constants.hpp"
#include "config/backend_config.hpp"
#include "simulators/simulator.hpp"

using json = nlohmann::json;
using namespace config;

template <SimType sim_type>
class Backend {
    std::unique_ptr<typename SimClass<sim_type>::type> simulator;
    BackendConfig<sim_type> backend_config;
public:

    Backend(config::BackendConfig<sim_type> backend_config) :
        simulator{std::make_unique<typename SimClass<sim_type>::type>()},
        backend_config{backend_config}
    { }

    json run(json circuit_json, const config::RunConfig& run_config) 
    {
        return simulator->execute(circuit_json, run_config);
    }

};