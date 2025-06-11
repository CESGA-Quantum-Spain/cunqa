#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include "quantum_task.hpp"
#include "utils/json.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

namespace cunqa {

std::string to_string(const QuantumTask& data)
{
    if (data.circuit.empty())
        return "";
    return data.config.dump(4) + data.circuit.dump(4); 
}

void QuantumTask::update_circuit(const std::string& quantum_task) 
{
    auto quantum_task_json = JSON::parse(quantum_task);
    std::vector<std::string> no_communications = {};

    if (quantum_task_json.contains("instructions") && quantum_task_json.contains("config")) {
        circuit = quantum_task_json.at("instructions").get<std::vector<JSON>>();
        config = quantum_task_json.at("config").get<JSON>();
        sending_to = (quantum_task_json.contains("sending_to") ? quantum_task_json.at("sending_to").get<std::vector<std::string>>() : no_communications);
        is_dynamic = ((quantum_task_json.contains("is_dynamic")) ? quantum_task_json.at("is_dynamic").get<bool>() : false);
        is_distributed = ((quantum_task_json.contains("is_distributed")) ? quantum_task_json.at("is_distributed").get<bool>() : false);

        LOGGER_DEBUG("After quantum task keys");

        if (is_distributed) {
            LOGGER_DEBUG("is_distributed");
            const char* STORE = std::getenv("STORE");
            std::string filepath = std::string(STORE) + "/.cunqa/communications.json";
            std::ifstream communications_file(filepath); 
            // TODO: Manage behaviour when file is not well opened

            JSON communications;
            communications_file >> communications;
            LOGGER_DEBUG("qpus json well read");

            for (auto& instruction : circuit) {
                std::string name = instruction.at("name").get<std::string>();
                if (instruction.contains("qpus")) {
                    std::vector<std::string> qpuid = instruction.at("qpus").get<std::vector<std::string>>();  
                    JSON qpu_communications = communications.at(qpuid[0]).get<JSON>();
                    std::string communications_endpoint = qpu_communications.at("communications_endpoint").get<std::string>();
                    instruction.at("qpus") = {communications_endpoint};
                }
            }
            std::vector<std::string> aux_connects_with = sending_to;
            int counter = 0;
            for (auto& qpuid : aux_connects_with) {
                JSON qpu = communications.at(qpuid).get<JSON>();
                sending_to[counter] = qpu.at("communications_endpoint").get<std::string>();
                counter++;
            }
        }

    } else if (quantum_task_json.contains("params")) {
        update_params_(quantum_task_json.at("params"));
    } else
        throw std::runtime_error("Incorrect format of the circuit.");
}

    
void QuantumTask::update_params_(const std::vector<double> params)
{
    if (circuit.empty()) 
        throw std::runtime_error("Circuit not sent before updating parameters.");

    try{
        int counter = 0;
        
        for (auto& instruction : circuit){

            std::string name = instruction.at("name");
            
            
            // TODO: Look at the instructions constants and how to work with them
            switch(cunqa::constants::INSTRUCTIONS_MAP.at(name)){
                case cunqa::constants::MEASURE:
                case cunqa::constants::ID:
                case cunqa::constants::X:
                case cunqa::constants::Y:
                case cunqa::constants::Z:
                case cunqa::constants::H:
                case cunqa::constants::CX:
                case cunqa::constants::CY:
                case cunqa::constants::CZ:
                    break;
                case cunqa::constants::RX:
                case cunqa::constants::RY:
                case cunqa::constants::RZ:
                    instruction.at("params")[0] = params[counter];
                    counter = counter + 1;
                    break; 
            }
        }
        
    } catch (const std::exception& e){
        LOGGER_ERROR("Error updating parameters. (check correct size).");
        throw std::runtime_error("Error updating parameters:" + std::string(e.what())); 
    }
}

} // End of cunqa namespace