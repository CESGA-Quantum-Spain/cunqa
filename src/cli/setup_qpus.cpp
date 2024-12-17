//#include "qpu.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include "qpu.hpp"
#include "simulators/simulator.hpp"
#include "config/qpu_config.hpp"
#include "comm/server.hpp"
#include "comm/client.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{

    std::cout << "argc: " << argc << "argv: " << argv[1] << " " << argv[2] << "\n";
    json qpu_config_json;
    if (argc == 4) 
    {
        std::ifstream f(argv[3]);
        qpu_config_json = json::parse(f);
    } 
    else  if (argc < 2)
        std::cerr << "Error, not a QPU configuration was given\n";

    if(auto search = sim_names.find(argv[2]); search != sim_names.end()) 
    {
    //if (sim_names.contains(argv[2]))
        if (search->second == SimType::Aer) 
        {
            config::QPUConfig<SimType::Aer> qpu_config{qpu_config_json, argv[1]};
            QPU<SimType::Aer> qpu(qpu_config);
            qpu.turn_ON();
        } 
        else if (search->second == SimType::Munich)
        {
            config::QPUConfig<SimType::Munich> qpu_config{qpu_config_json, argv[1]};
            QPU<SimType::Munich> qpu(qpu_config);
            qpu.turn_ON();
        }  
    }
    

    else
        std::cerr << "No such a simulator as " << argv[2] <<" is implemented\n";
}