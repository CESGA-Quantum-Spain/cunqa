#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include "quantum_task.hpp"
#include "utils/json.hpp"

#include "logger.hpp"


namespace cunqa {
using namespace cunqa::constants;

QuantumTask::QuantumTask(const std::string& quantum_task) { update_circuit(quantum_task); }

void QuantumTask::update_circuit(const std::string& quantum_task) 
{
    auto quantum_task_json = quantum_task == "" ? JSON() : JSON::parse(quantum_task);
    std::vector<std::string> no_communications = {};

    if (quantum_task_json.contains("instructions") && quantum_task_json.contains("config")) { // Usual circuit with config
        id = quantum_task_json.at("id");

        circuit = quantum_task_json.at("instructions").get<std::vector<JSON>>();

        config = quantum_task_json.at("config").get<JSON>();

        sending_to = (quantum_task_json.contains("sending_to") ? quantum_task_json.at("sending_to").get<std::vector<std::string>>() : no_communications);

        is_dynamic = ((quantum_task_json.contains("is_dynamic")) ? quantum_task_json.at("is_dynamic").get<bool>() : false);

    } else if (quantum_task_json.contains("params"))
        update_params_(quantum_task_json.at("params"), quantum_task_json.at("shots"));
}

    
void QuantumTask::update_params_(const std::vector<double> params, const int shots)
{
    if (circuit.empty()) 
        throw std::runtime_error("Circuit not sent before updating parameters.");

    try{
        int counter = 0;
        
        for (auto& instruction : circuit){
            std::string name = instruction.at("name");
            switch(INSTRUCTIONS_MAP.at(name)){
                // One parameter gates 
                case RX:
                case RY:
                case RZ:
                case P:
                case U1:
                case CRX:
                case CRY:
                case CRZ:
                case CP:
                case CU1:
                case RXX:
                case RYY:
                case RZZ:
                case RZX:
                    instruction.at("params")[0] = params[counter];
                    counter = counter + 1;
                    break; 
                // Two parameter gates 
                case U2:
                case R:
                case CU2:
                case CR:
                case MCU2:
                case MCR:
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    counter = counter + 2;
                    break;
                // Three parameter gates 
                case U3:
                case CU3:
                case MCU3:
                    instruction.at("params")[0] = params[counter];
                    instruction.at("params")[1] = params[counter + 1];
                    instruction.at("params")[2] = params[counter + 2];
                    counter = counter + 3;
                    break;
                // Four parameter gates 
                case U:
                case CU:
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

        config["shots"] = shots;

    } catch (const std::exception& e){
        LOGGER_ERROR("Error updating parameters. (check correct size).");
        throw std::runtime_error("Error updating parameters:" + std::string(e.what())); 
    }
}

std::string to_string(const QuantumTask& data)
{
    if (data.circuit.empty())
        return "";
    
    JSON instructions = data.circuit; // Warning. Strange convertion from std::vector<JSON> to JSON
    std::string circ_str = "{\"id\": \"" + data.id + "\",\n\"config\": " + data.config.dump() + ",\n\"instructions\":" + instructions.dump() + ",\n\"sending_to\":[";

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

StructuredQuantumTask from_quantum_task_to_structuredqtask(const QuantumTask& quantum_task)
{
    StructuredQuantumTask structured_qtask = {
        .id = quantum_task.id,
        .n_qubits = quantum_task.config.at("num_qubits").get<int>(),
        .n_clbits = quantum_task.config.at("num_clbits").get<int>(),
        .instructions = from_json_instructions_to_cunqainstructions(quantum_task.circuit)
    };

    return structured_qtask;
}

} // End of cunqa namespace