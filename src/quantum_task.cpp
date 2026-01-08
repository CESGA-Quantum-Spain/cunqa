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
    std::string circ_str = "{\"id\": \"" + data.id + "\",\n\"config\": " + data.config.dump() + ",\n\"instructions\": " + data.circuit.dump() + ",\n\"sending_to\":[";

    bool first_target = true;
    for (const auto& target : data.sending_to) {
        if (!first_target) {
            circ_str += ", ";
        }
        first_target = false;
        circ_str += "\"" + target + "\"";
    }
    circ_str += "],\n\"is_dynamic\":";
    circ_str += data.is_dynamic ? "true}\n" : "false}\n";

    return circ_str;
}

QuantumTask::QuantumTask(const std::string& quantum_task) { update_circuit(quantum_task); }

void QuantumTask::update_circuit(const std::string& quantum_task) 
{
    auto quantum_task_json = quantum_task == "" ? JSON() : JSON::parse(quantum_task);
    std::vector<std::string> no_communications = {};

    if (quantum_task_json.contains("instructions") && quantum_task_json.contains("config")) {
        circuit = quantum_task_json.at("instructions").get<std::vector<JSON>>();
        config = quantum_task_json.at("config").get<JSON>();
        sending_to = (quantum_task_json.contains("sending_to") ? quantum_task_json.at("sending_to").get<std::vector<std::string>>() : no_communications);
        is_dynamic = ((quantum_task_json.contains("is_dynamic")) ? quantum_task_json.at("is_dynamic").get<bool>() : false);
        id = quantum_task_json.at("id");
    } else if (quantum_task_json.contains("params"))
        update_params_(quantum_task_json.at("params"));
}

    
void QuantumTask::update_params_(const std::vector<double> params)
{
    if (circuit.empty()) 
        throw std::runtime_error("Circuit not sent before updating parameters.");

    try{
        int counter = 0;
        
        for (auto& instruction : circuit){
            std::string name = instruction.at("name");
            switch(cunqa::constants::INSTRUCTIONS_MAP.at(name)){
                // One parameter gates 
                case cunqa::constants::RX:
                case cunqa::constants::RY:
                case cunqa::constants::RZ:
                    instruction.at("params")[0] = params[counter];
                    counter = counter + 1;
                    break; 
                // Two parameter gates 
                case cunqa::constants::R:
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    counter = counter + 2;
                    break;
                // Three parameter gates 
                case cunqa::constants::U:
                case cunqa::constants::CU:
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    instruction.at("params")[2] = params[counter + 2];
                    counter = counter + 3;
                    break;
                default:
                    break;
            }
        }

    } catch (const std::exception& e){
        LOGGER_ERROR("Error updating parameters. (check correct size).");
        throw std::runtime_error("Error updating parameters:" + std::string(e.what())); 
    }
}

} // End of cunqa namespace