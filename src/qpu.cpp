#include <string>
#include <iostream>

#include "qpu.hpp"
#include "quantum_task.hpp"
#include "utils/constants.hpp"

#include "logger.hpp"

using namespace std::string_literals;

namespace {
using namespace cunqa;

Circuit json_to_circuit(const JSON& circuit_json) 
{
    Circuit circuit;
    for (auto const& instruction : circuit_json) {
        LOGGER_DEBUG("Instruccion: {}", instruction.dump());
        Instruction cunqa_instruction;
        auto instruction_tag = INSTRUCTIONS_MAP.at(instruction.at("name").get<std::string>());
        switch (instruction_tag) {
            case InstructionTag::ID:
            case InstructionTag::X:
            case InstructionTag::Y:
            case InstructionTag::Z:
            case InstructionTag::H:
            case InstructionTag::S:
            case InstructionTag::SX:
            case InstructionTag::SY:
            case InstructionTag::SZ:
            case InstructionTag::SDG:
            case InstructionTag::SXDG:
            case InstructionTag::SYDG:
            case InstructionTag::SZDG:
            case InstructionTag::T:
            case InstructionTag::TDG:
            case InstructionTag::P0:
            case InstructionTag::P1:
            case InstructionTag::V:
            case InstructionTag::VDG:
            case InstructionTag::K:
            {
                cunqa_instruction = OneQubitNoParam{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>()
                };
                break;
            }
            case InstructionTag::RX:
            case InstructionTag::RY:
            case InstructionTag::RZ:
            case InstructionTag::GLOBALP:
            case InstructionTag::P:
            case InstructionTag::U1:
            case InstructionTag::ROTX:
            case InstructionTag::ROTY:
            case InstructionTag::ROTZ:
            case InstructionTag::ROTINVX:
            case InstructionTag::ROTINVY:
            case InstructionTag::ROTINVZ:
            {
                auto param = instruction.at("params").get<double>();
                circuit.params.push_back(&param);
                cunqa_instruction = OneQubitOneParam{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(), 
                    param
                };
                break;
            }
            case InstructionTag::U2:
            case InstructionTag::R:
            {
                auto params = instruction.at("params").get<std::array<double, 2>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                cunqa_instruction = OneQubitTwoParam{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(), 
                    params
                };
                break;
            }
            case InstructionTag::U3:
            {
                auto params = instruction.at("params").get<std::array<double, 3>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                cunqa_instruction = OneQubitThreeParam{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(), 
                    params
                };
                break;
            }
            case InstructionTag::U:
            {
                auto params = instruction.at("params").get<std::array<double, 4>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                circuit.params.push_back(&params[3]);
                cunqa_instruction = OneQubitFourParam{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(), 
                    params
                };
                break;
            }
            case InstructionTag::ECR:
            case InstructionTag::SWAP:
            case InstructionTag::ISWAP:
            case InstructionTag::CX:
            case InstructionTag::CY:
            case InstructionTag::CZ:
            case InstructionTag::CH:
            case InstructionTag::CSX:
            case InstructionTag::CSXDG:
            case InstructionTag::CS:
            case InstructionTag::CSDG:
            case InstructionTag::CT:
            case InstructionTag::DCX:
            {
                cunqa_instruction = TwoQubitNoParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>()
                };
                break;
            }
            case InstructionTag::CRX:
            case InstructionTag::CRY:
            case InstructionTag::CRZ:
            case InstructionTag::CP:
            case InstructionTag::CU1:
            case InstructionTag::RXX:
            case InstructionTag::RYY:
            case InstructionTag::RZZ:
            case InstructionTag::RZX:
            {
                auto param = instruction.at("params").get<double>();
                circuit.params.push_back(&param);
                cunqa_instruction = TwoQubitOneParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>(),
                    param
                };
                break;
            }
            case InstructionTag::CU2:
            case InstructionTag::XXMYY:
            case InstructionTag::XXPYY:
            {
                auto params = instruction.at("params").get<std::array<double, 2>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                cunqa_instruction = TwoQubitTwoParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>(),
                    params
                };
                break;
            }
            case InstructionTag::CU3:
            {
                auto params = instruction.at("params").get<std::array<double, 3>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                cunqa_instruction = TwoQubitThreeParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>(),
                    params
                };
                break;
            }
            case InstructionTag::CU:
            {
                auto params = instruction.at("params").get<std::array<double, 4>>();
                circuit.params.push_back(&params[0]);
                circuit.params.push_back(&params[1]);
                circuit.params.push_back(&params[2]);
                circuit.params.push_back(&params[3]);
                cunqa_instruction = TwoQubitFourParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>(),
                    params
                };
                break;
            }
            case InstructionTag::CECR:
            case InstructionTag::CSWAP:
            case InstructionTag::CCX:
            case InstructionTag::CCY:
            case InstructionTag::CCZ:
            {
                cunqa_instruction = ThreeQubitNoParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 3>>()
                };
                break;
            }
            case InstructionTag::MCX:
            case InstructionTag::MCY:
            case InstructionTag::MCZ:
            case InstructionTag::MCSX:
            case InstructionTag::MCSWAP:
            {
                cunqa_instruction = MulticontrolNoParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>()
                };
                break;
            }
            case InstructionTag::MCRX:
            case InstructionTag::MCRY:
            case InstructionTag::MCRZ:
            case InstructionTag::MCP:
            case InstructionTag::MCU1:
            case InstructionTag::MCU2:
            case InstructionTag::MCU3:
            case InstructionTag::MCU:
            {
                auto params = instruction.at("params").get<std::vector<double>>();
                for(auto& param : params)
                    circuit.params.push_back(&param);
                cunqa_instruction = MulticontrolParam{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<int>>(), 
                    params
                };
                break;
            }
            case InstructionTag::UNITARY:
            case InstructionTag::SPARSEMATRIX:
            {
                cunqa_instruction = MatrixGate{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>(), 
                    Matrix() // TODO: Arreglar
                };
                break;
            }
            case InstructionTag::DIAGONAL:
            {
                cunqa_instruction = DiagonalMatrixGate{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>(), 
                    DiagonalMatrix() // TODO: Arreglar
                };
                break;
            }
            case InstructionTag::RANDOMUNITARY:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = RandomUnitary{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<int>>(), 
                    seed
                };
                break;
            }
            case InstructionTag::FUSEDSWAP:
            {
                cunqa_instruction = FusedSwap{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>(),
                    instruction.at("block_size").get<int>()
                };
                break;
            }
            case InstructionTag::MULTIPAULI:
            case InstructionTag::MULTIPAULIROTATION:
            {
                double param = 0;
                if (instruction.contains("param"))
                    param = instruction.at("param");
                cunqa_instruction = MultiPauli{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>(), 
                    param,
                    instruction.at("pauli_id_list").get<std::vector<unsigned int>>()
                };
                break;
            }
            case InstructionTag::AMPLITUDEDAMPINGNOISE:
            case InstructionTag::BITFLIPNOISE:
            case InstructionTag::DEPHASINGNOISE:
            case InstructionTag::DEPOLARIZINGNOISE:
            case InstructionTag::INDEPENDENTXZNOISE:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = OneQubitNoise{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(), 
                    instruction.at("params").get<double>(),
                    seed
                };
                break;
            }
            case InstructionTag::TWOQUBITDEPOLARIZINGNOISE:
            {
                int seed = instruction.contains("seed") ? instruction.at("seed").get<int>() : 0;
                cunqa_instruction = TwoQubitNoise{
                    instruction_tag,
                    instruction.at("qubits").get<std::array<Qubit, 2>>(), 
                    instruction.at("params").get<double>(),
                    seed
                };
                break;
            }
            case InstructionTag::SEND:
            case InstructionTag::RECV:
            {
                cunqa_instruction = ClassicalComm{
                    instruction_tag,
                    instruction.at("clbits").get<std::vector<Clbit>>(), 
                    instruction.at("qpus").get<std::vector<std::string>>()
                };
                break;
            }
            case InstructionTag::QSEND:
            case InstructionTag::QRECV:
            case InstructionTag::EXPOSE:
            case InstructionTag::UNEXPOSE:
            {
                cunqa_instruction = QuantumComm{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>(),
                    instruction.at("qpus").get<std::vector<std::string>>()
                };
                break;
            }
            case InstructionTag::CIF:
            case InstructionTag::ENDCIF:
            {
                cunqa_instruction = ClassicalIf{
                    instruction_tag,
                    instruction.at("clbits").get<std::vector<Clbit>>(), 
                };
                break;
            }
            case InstructionTag::COPY:
            {
                cunqa_instruction = Copy{
                    instruction_tag,
                    instruction.at("l_clbits").get<std::vector<Clbit>>(),
                    instruction.at("r_clbits").get<std::vector<Clbit>>()
                };
                break;
            }
            case InstructionTag::RESET:
            {
                cunqa_instruction = Reset{
                    instruction_tag,
                    instruction.at("qubits").get<std::vector<Qubit>>()
                };
                break;
            }
            case InstructionTag::SAVE_STATE:
            {
                // TODO: Hacerlo
                break;
            }    
            case InstructionTag::MEASURE:
            {
                cunqa_instruction = Measure{
                    instruction_tag,
                    instruction.at("qubits").get<Qubit>(),
                    instruction.at("clbits").get<Clbit>()
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

