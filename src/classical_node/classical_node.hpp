#pragma once

#include <string>
#include <memory>
#include <vector>
#include <complex>
#include "zmq.hpp"

#include "simulators/simulator.hpp"
#include "logger/logger.hpp"
#include "classical_node_utils.hpp"
#include "communication_component.hpp"

#include "logger/logger.hpp"


using json = nlohmann::json;
using CunqaInstructions = std::vector<json>;


template <SimType sim_type>
class QPUClassicalNode 
{
public:

    Backend<sim_type> backend;
    CommunicationComponent<sim_type> comm_component;
    json endpoint_info = {};
    json result = {};
    
    QPUClassicalNode(std::string& comm_type, config::QPUConfig<sim_type> qpu_config);

    inline void send_circ_to_execute(json& kernel);
    inline void send_instructions_to_execute(json& kernel);
    inline void clean_result();

};

template <SimType sim_type>
QPUClassicalNode<sim_type>::QPUClassicalNode(std::string& comm_type, config::QPUConfig<sim_type> qpu_config) : backend(qpu_config.backend_config), comm_component(comm_type)
{
    SPDLOG_LOGGER_DEBUG(logger, "Classical Node correctly instanciated.");

    std::string endpoint_type;
    std::string endpoint_value;
    if (this->comm_component.comm_type == "mpi") {
        endpoint_type = "mpi";
        endpoint_value = std::to_string(this->comm_component.mpi_rank);
    } else if (this->comm_component.comm_type == "zmq") {
        endpoint_type = "zmq";
        endpoint_value = this->comm_component.zmq_endpoint.value();
    } else {
        endpoint_type = "none";
        endpoint_value = "";
    }

    this->endpoint_info = {
        {endpoint_type, endpoint_value}
    };
}

template <SimType sim_type>
inline void QPUClassicalNode<sim_type>::send_circ_to_execute(json& kernel)
{
    this->result = this->backend.run(kernel);
    SPDLOG_LOGGER_DEBUG(logger, "Result is in the classical node.");
}

template <SimType sim_type>
inline void QPUClassicalNode<sim_type>::send_instructions_to_execute(json& kernel)
{
    CunqaInstructions instructions = kernel.at("instructions");
    config::RunConfig run_config(kernel.at("config"));
    json run_config_json(run_config);
    int shots = run_config_json.at("shots");
    std::string instruction_name;
    std::vector<int> qubits;
    int measurement;
    std::array<std::string, 2> comm_endp;
    std::vector<double> param;
    std::unordered_map<int, int> counts;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < shots; i++) {
        for (auto& instruction : instructions) {
            instruction_name = instruction.at("name");
            qubits = instruction.at("qubits").get<std::vector<int>>();
            switch (CUNQA::INSTRUCTIONS_MAP[instruction_name])
            {
                case measure:
                    measurement = this->backend.apply_measure(qubits);
                    break;
                case CUNQA::ID:
                case CUNQA::X:
                case CUNQA::Y:
                case CUNQA::Z:
                case CUNQA::H:
                case CUNQA::CX:
                case CUNQA::CY:
                case CUNQA::CZ:
                case CUNQA::ECR:
                case CUNQA::C_IF_H:
                case CUNQA::C_IF_X:
                case CUNQA::C_IF_Y:
                case CUNQA::C_IF_Z:
                case CUNQA::C_IF_CX:
                case CUNQA::C_IF_CY:
                case CUNQA::C_IF_CZ:
                case CUNQA::C_IF_ECR:
                    this->backend.apply_gate(instruction_name, qubits);
                    break;
                case CUNQA::RX:
                case CUNQA::RY:
                case CUNQA::RZ:
                case CUNQA::C_IF_RX:
                case CUNQA::C_IF_RY:
                case CUNQA::C_IF_RZ:
                    param =  instruction.at("params").get<std::vector<double>>();
                    this->backend.apply_gate(instruction_name, qubits, param);
                    break;
                case CUNQA::D_C_IF_H:
                case CUNQA::D_C_IF_X:
                case CUNQA::D_C_IF_Y:
                case CUNQA::D_C_IF_Z:
                case CUNQA::D_C_IF_CX:
                case CUNQA::D_C_IF_CY:
                case CUNQA::D_C_IF_CZ:
                case CUNQA::D_C_IF_ECR:
                    comm_endp = instruction.at("qpus").get<std::array<std::string, 2>>();
                    //SPDLOG_LOGGER_DEBUG(logger, "Communication endpoints: {}, {}", comm_endp[0], comm_endp[1]);
                    if (this->comm_component.is_sender_qpu(comm_endp[0])) {
                        measurement = this->backend.apply_measure(qubits);
                        this->comm_component._send(measurement, comm_endp[1]);
                        //SPDLOG_LOGGER_DEBUG(logger, "Measurement sent to {}", comm_endp[1]);
                    } else if (this->comm_component.is_receiver_qpu(comm_endp[1])) {
                        measurement = this->comm_component._recv(comm_endp[0]);
                        //SPDLOG_LOGGER_DEBUG(logger, "Measurement received from {}", comm_endp[0]);
                        if (measurement == 1) {
                            this->backend.apply_gate(CUNQA::CORRESPONDENCE_D_GATE_MAP[instruction_name], qubits);
                        }
                    } else {
                        //SPDLOG_LOGGER_ERROR(logger, "This QPU has no ID {} nor {}", comm_endp[0], comm_endp[1]); 
                        throw std::runtime_error("Error with the QPU IDs.");
                    }
                    break;
                case CUNQA::D_C_IF_RX:
                case CUNQA::D_C_IF_RY:
                case CUNQA::D_C_IF_RZ:
                    comm_endp = instruction.at("qpus").get<std::array<std::string, 2>>();
                    //SPDLOG_LOGGER_DEBUG(logger, "Communication endpoint: {}, {}", comm_endp[0], comm_endp[1]);
                    param = instruction.at("params").get<std::vector<double>>();
                    if (this->comm_component.is_sender_qpu(comm_endp[0])) {
                        measurement = this->backend.apply_measure(qubits);
                        this->comm_component._send(measurement, comm_endp[1]);
                    } else if (this->comm_component.is_receiver_qpu(comm_endp[1])) {
                        measurement = this->comm_component._recv(comm_endp[0]);
                        if (measurement == 1) {
                            this->backend.apply_gate(CUNQA::CORRESPONDENCE_D_GATE_MAP[instruction_name], qubits, param);
                        }
                    } else {
                        SPDLOG_LOGGER_ERROR(logger, "This QPU has no ID {} nor {}", comm_endp[0], comm_endp[1]); 
                        throw std::runtime_error("Error with the QPU IDs.");
                    }
                    break;  
                default:
                    std::cout << "Error. Invalid gate name" << "\n";
                    break;
            }
        }
        int position = this->backend.get_shot_result();
        counts[position]++;
        this->backend.restart_statevector();
    }

    auto stop_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time);
    double total_time = duration.count();

    this->result = {
        {"counts", counts},
        {"time_taken", total_time}
    }; 

    counts.clear();

}

template <SimType sim_type>
inline void QPUClassicalNode<sim_type>::clean_result()
{
    this->result.clear();
}

