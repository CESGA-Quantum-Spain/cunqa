#include <string>
#include <iostream>

#include "qpu.hpp"
#include "quantum_task/quantum_task.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

using namespace std::string_literals;

namespace {
using namespace cunqa;

Circuit json_to_circuit(const JSON& circuit_json) 
{
    Circuit circuit;
    for (auto const& instruction : circuit_json) {
        Instruction cunqa_instruction;
        auto instruction_type = INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        switch (instruction_type) {
            case InstructionType::ID:
            case InstructionType::X:
            case InstructionType::Y:
            case InstructionType::Z:
            case InstructionType::H:
            case InstructionType::S:
            case InstructionType::SX:
            case InstructionType::SY:
            case InstructionType::SZ:
            case InstructionType::SDG:
            case InstructionType::SXDG:
            case InstructionType::SYDG:
            case InstructionType::SZDG:
            case InstructionType::T:
            case InstructionType::TDG:
            case InstructionType::P0:
            case InstructionType::P1:
            case InstructionType::V:
            case InstructionType::VDG:
            case InstructionType::K:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitNoParam {
                        instruction.at("qubits").get<Qubit>()
                    }
                };
                break;
            }
            case InstructionType::RX:
            case InstructionType::RY:
            case InstructionType::RZ:
            case InstructionType::GLOBALP:
            case InstructionType::P:
            case InstructionType::U1:
            case InstructionType::ROTX:
            case InstructionType::ROTY:
            case InstructionType::ROTZ:
            case InstructionType::ROTINVX:
            case InstructionType::ROTINVY:
            case InstructionType::ROTINVZ:
            {
                auto param = instruction.at("params").get<double>();
                circuit.params.push_back(&param);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitOneParam {
                        instruction.at("qubits").get<Qubit>(), 
                        param
                    }
                };
                break;
            }
            case InstructionType::U2:
            case InstructionType::R:
            {
                auto params = instruction.at("params").get<std::array<double, 2>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitTwoParam {
                        instruction.at("qubits").get<Qubit>(), 
                        params
                    }
                };
                break;
            }
            case InstructionType::U3:
            {
                auto params = instruction.at("params").get<std::array<double, 3>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitThreeParam {
                        instruction.at("qubits").get<Qubit>(), 
                        params
                    }
                };
                break;
            }
            case InstructionType::U:
            {
                auto params = instruction.at("params").get<std::array<double, 4>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                circuit.params.push_back(&params[3]);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitFourParam {
                        instruction.at("qubits").get<Qubit>(), 
                        params
                    }
                };
                break;
            }
            case InstructionType::ECR:
            case InstructionType::SWAP:
            case InstructionType::ISWAP:
            case InstructionType::CX:
            case InstructionType::CY:
            case InstructionType::CZ:
            case InstructionType::CH:
            case InstructionType::CSX:
            case InstructionType::CSXDG:
            case InstructionType::CS:
            case InstructionType::CSDG:
            case InstructionType::CT:
            case InstructionType::DCX:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitNoParam {
                        instruction.at("qubits").get<std::array<Qubit, 2>>()
                    }
                };
                break;
            }
            case InstructionType::CRX:
            case InstructionType::CRY:
            case InstructionType::CRZ:
            case InstructionType::CP:
            case InstructionType::CU1:
            case InstructionType::RXX:
            case InstructionType::RYY:
            case InstructionType::RZZ:
            case InstructionType::RZX:
            {
                auto param = instruction.at("params").get<double>();
                circuit.params.push_back(&param);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitOneParam {
                        instruction.at("qubits").get<std::array<Qubit, 2>>(),
                        param
                    }
                };
                break;
            }
            case InstructionType::CU2:
            case InstructionType::XXMYY:
            case InstructionType::XXPYY:
            {
                auto params = instruction.at("params").get<std::array<double, 2>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitTwoParam {
                        instruction.at("qubits").get<std::array<Qubit, 2>>(),
                        params
                    }
                };
                break;
            }
            case InstructionType::CU3:
            {
                auto params = instruction.at("params").get<std::array<double, 3>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitThreeParam {
                        instruction.at("qubits").get<std::array<Qubit, 2>>(),
                        params
                    }
                };
                break;
            }
            case InstructionType::CU:
            {
                auto params = instruction.at("params").get<std::array<double, 4>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                circuit.params.push_back(&params[3]);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitFourParam{
                        instruction.at("qubits").get<std::array<Qubit, 2>>(),
                        params
                    }
                };
                break;
            }
            case InstructionType::CECR:
            case InstructionType::CSWAP:
            case InstructionType::CSQRTSWAP:
            case InstructionType::CCX:
            case InstructionType::CCY:
            case InstructionType::CCZ:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = ThreeQubitNoParam{
                        instruction.at("qubits").get<std::array<Qubit, 3>>()
                    }
                };
                break;
            }
            case InstructionType::MCX:
            case InstructionType::MCY:
            case InstructionType::MCZ:
            case InstructionType::MCH:
            case InstructionType::MCSX:
            case InstructionType::MCS:
            case InstructionType::MCT:
            case InstructionType::MCSWAP:
            case InstructionType::MCSQRTSWAP:
            case InstructionType::MX:
            case InstructionType::CMX:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = MultiNoParam{
                        instruction.at("qubits").get<std::vector<Qubit>>()
                    }
                };
                break;
            }
            case InstructionType::MCRX:
            case InstructionType::MCRY:
            case InstructionType::MCRZ:
            case InstructionType::MCRAXIS:
            case InstructionType::MCP:
            case InstructionType::MCU1:
            case InstructionType::MCU2:
            case InstructionType::MCU3:
            case InstructionType::MCU:
            case InstructionType::PHASEGADGET:
            case InstructionType::CPHASEGADGET:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                for(auto& param : params)
                    circuit.params.push_back(&param);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = MultiParam{
                        instruction.at("qubits").get<std::vector<int>>(), 
                        params
                    }
                };
                break;
            }
            case InstructionType::PAULISTR:
            case InstructionType::CPAULISTR:
            case InstructionType::MCPAULISTR:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = PauliNoParam{
                        instruction.at("qubits").get<std::vector<int>>(), 
                        instruction.at("paulistr").get<std::string>()
                    }
                };
                break;
            }
            case InstructionType::PAULIGADGET:
            case InstructionType::CPAULIGADGET:
            case InstructionType::MCPAULIGADGET:
            case InstructionType::NONUNITARYPAULIGADGET:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                for(auto& param : params)
                    circuit.params.push_back(&param);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = PauliParam{
                        instruction.at("qubits").get<std::vector<int>>(), 
                        parmas,
                        instruction.at("paulistr").get<std::string>()
                    }
                };
                break;
            }
            case InstructionType::MCMX:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = NumControlsNoParam{
                        instruction.at("qubits").get<std::vector<int>>(), 
                        instruction.at("num_controls").get<int>()
                    }
                };
                break;
            }
            case InstructionType::MCPHASEGADGET:
            {
                auto param = instruction.at("params").get<double>();
                circuit.params.push_back(&param);
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = NumControlsParam{
                        instruction.at("qubits").get<std::vector<int>>(), 
                        param
                    }
                };
                break;
            }
            case InstructionType::UNITARY:
            case InstructionType::SPARSEMATRIX:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = MatrixGate{
                        instruction.at("qubits").get<std::vector<Qubit>>(), 
                        Matrix() // TODO: Arreglar
                    }
                };
                break;
            }
            case InstructionType::DIAGONAL:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = DiagonalMatrixGate {
                        instruction.at("qubits").get<std::vector<Qubit>>(), 
                        DiagonalMatrix() // TODO: Arreglar
                    }
                };
                break;
            }
            case InstructionType::RANDOMUNITARY:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = RandomUnitary {
                        instruction.at("qubits").get<std::vector<int>>(), 
                        seed
                    }
                };
                break;
            }
            case InstructionType::FUSEDSWAP:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = FusedSwap {
                        instruction.at("qubits").get<std::vector<Qubit>>(),
                        instruction.at("block_size").get<int>()
                    }
                };
                break;
            }
            case InstructionType::MULTIPAULI:
            case InstructionType::MULTIPAULIROTATION:
            {
                double param = 0;
                if (instruction.contains("param"))
                    param = instruction.at("param");
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = MultiPauli{
                        instruction.at("qubits").get<std::vector<Qubit>>(), 
                        param,
                        instruction.at("pauli_id_list").get<std::vector<unsigned int>>()
                    }
                };
                break;
            }
            case InstructionType::AMPLITUDEDAMPINGNOISE:
            case InstructionType::BITFLIPNOISE:
            case InstructionType::DEPHASINGNOISE:
            case InstructionType::DEPOLARIZINGNOISE:
            case InstructionType::INDEPENDENTXZNOISE:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = OneQubitNoise{
                        instruction.at("qubits").get<Qubit>(), 
                        instruction.at("params").get<double>(),
                        seed
                    }
                };
                break;
            }
            case InstructionType::TWOQUBITDEPOLARIZINGNOISE:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = TwoQubitNoise{
                        instruction.at("qubits").get<std::array<Qubit, 2>>(), 
                        instruction.at("params").get<double>(),
                        seed
                    }
                };
                break;
            }
            case InstructionType::SEND:
            case InstructionType::RECV:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = ClassicalComm{
                        instruction.at("clbits").get<std::vector<Clbit>>(), 
                        instruction.at("qpus").get<std::vector<std::string>>()
                    }
                };
                break;
            }
            case InstructionType::QSEND:
            case InstructionType::QRECV:
            case InstructionType::EXPOSE:
            case InstructionType::UNEXPOSE:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = QuantumComm{
                        instruction.at("qubits").get<std::vector<Qubit>>(),
                        instruction.at("qpus").get<std::vector<std::string>>()
                    }
                };
                break;
            }
            case InstructionType::CIF:
            case InstructionType::ENDCIF:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = ClassicalIf{
                        instruction.at("clbits").get<std::vector<Clbit>>(), 
                    }
                };
                break;
            }
            case InstructionType::COPY:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = Copy{
                        instruction.at("l_clbits").get<std::vector<Clbit>>(),
                        instruction.at("r_clbits").get<std::vector<Clbit>>()
                    }
                };
                break;
            }
            case InstructionType::RESET:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = Reset{
                        instruction.at("qubits").get<std::vector<Qubit>>()
                    }
                };
                break;
            }
            case InstructionType::SAVE_STATE:
            {
                // TODO: Hacerlo
                break;
            }    
            case InstructionType::MEASURE:
            {
                cunqa_instruction = {
                    .type = instruction_type,
                    .payload = Measure{
                        instruction.at("qubits").get<Qubit>(),
                        instruction.at("clbits").get<Clbit>()
                    }
                };
                break;
            }
            default:
                LOGGER_ERROR("Instruction not suported!");
        } // End switch
        circuit.instructions.push_back(cunqa_instruction);
    }
    return circuit;
}

}

namespace cunqa {

QPU::QPU(std::unique_ptr<sim::Backend> backend, const std::string& mode, 
         const std::string& name, const std::string& family) :
    backend{std::move(backend)},
    server{std::make_unique<comm::Server>(mode)},
    name_{name},
    family_{family}
{ }

void QPU::turn_ON() 
{
    std::thread listen([this](){this->recv_data_();});
    std::thread compute([this](){this->compute_result_();});

    JSON qpu_config = *this;
    write_on_file(qpu_config, QPUS_FILEPATH, name_);

    listen.join();
    compute.join();
}

void QPU::compute_result_()
{    
    QuantumTask task;
    while (true) 
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_condition_.wait(lock, [this] { return !message_queue_.empty(); });

        while (!message_queue_.empty()) 
        {
            try {
                auto quantum_task_json = JSON::parse(message_queue_.front());
                message_queue_.pop();
                lock.unlock();

                auto id = quantum_task_json.at("id").get<std::string>();
                RunConfig run_config(quantum_task_json.at("config"));

                if (auto it = quantum_task_json.find("instructions"); it != quantum_task_json.end()) {
                    auto& instructions = *it;
                    task = QuantumTask(id, run_config, json_to_circuit(instructions));
                }
                else if (auto it = quantum_task_json.find("params"); it != quantum_task_json.end())
                    task.update_params(it->get<std::vector<double>>());
                
                auto result = backend->execute(std::move(task));
                server->send_result(result.dump());
            } catch(const comm::ServerException& e) {
                LOGGER_ERROR("There has happened an error sending the result, probably the client has had an error.");
                LOGGER_ERROR("Message of the error: {}", e.what());
            } catch(const std::exception& e) {
                LOGGER_ERROR("There has happened an error executing or sending the result, the server keeps on iterating.");
                LOGGER_ERROR("Message of the error: {}", e.what());
                server->send_result("{\"ERROR\":\""s + std::string(e.what()) + "\"}"s);
            }
            lock.lock();
        }
    }
}

void QPU::recv_data_() 
{   
    while (true) {
        try {
            auto message = server->recv_data();
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                message_queue_.push(message);
            }
            queue_condition_.notify_one();
        } catch (const std::exception& e) {
            LOGGER_INFO("There has happened an error receiving the circuit, the server keeps on iterating.");
            LOGGER_ERROR("Official message of the error: {}", e.what());
            throw;
        }
    }
}


} // End of cunqa namespace

