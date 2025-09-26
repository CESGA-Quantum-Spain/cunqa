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
    return "{\"id\": \"" + data.id + "\",\n\"config\": " + data.config.dump() + ",\n\"instructions\": " + data.circuit.dump() + "}\n"; 
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
        has_cc = ((quantum_task_json.contains("has_cc")) ? quantum_task_json.at("has_cc").get<bool>() : false);
        id = quantum_task_json.at("id");

        if (has_cc) {
            const char* STORE = std::getenv("STORE");
            std::string filepath = std::string(STORE) + "/.cunqa/communications.json";
            std::ifstream communications_file(filepath); 
            // TODO: Manage behaviour when file is not well opened

            JSON communications;
            communications_file >> communications;

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

    } else if (quantum_task_json.contains("params"))
        update_params_(quantum_task_json.at("params"), quantum_task_json.at("shots"));
}

    
void QuantumTask::update_params_(const std::vector<double> params, const int& new_shots)
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
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    instruction.at("params")[2] = params[counter + 2];
                    counter = counter + 3;
                    break;
                // Four parameter gates
                case cunqa::constants::CU:
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    instruction.at("params")[2] = params[counter + 2];
                    instruction.at("params")[3] = params[counter + 3];
                    counter = counter + 4;
                    break;
                default:
                    break;
            }
        }

        config.at("shots") = new_shots;
        
    } catch (const std::exception& e){
        LOGGER_ERROR("Error updating parameters. (check correct size).");
        throw std::runtime_error("Error updating parameters:" + std::string(e.what())); 
    }
}

} // End of cunqa namespace