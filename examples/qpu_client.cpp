//#include "qpu.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include "qpu.hpp"
#include "config/qpu_config.hpp"
#include "comm/server.hpp"
#include "comm/client.hpp"

using json = nlohmann::json;

json backend_config = json::parse(R"(
{
    "backend":
    {
        "name": "Example Backend",
        "version": "1.2.0",
        "simulator": "Aer",
        "n_qubits": 32,
        "url": "http://example-backend.com",
        "is_simulator": true,
        "conditional": false,
        "memory": true,
        "max_shots": 8192,
        "description": "This is an example backend configuration.",
        "basis_gates": "x",
        "custom_instructions": "custom_gate1"
    } 
}
)");

json execution_config = json::parse(R"(
{
    "config": {
        "shots": 1024,
        "method": "statevector",
        "memory_slots": 7
    },
    "instructions": [
    {
        "name": "h",
        "qubits": [0]
    },
    {
        "name": "cx",
        "qubits": [0, 1]
    },
    {
        "name": "measure",
        "qubits": [0],
        "memory": [0]
    },
    {
        "name": "measure",
        "qubits": [1],
        "memory": [1]
    }
    ]
}
)");

int main(int argc, char *argv[])
{

    /* config::QPUConfig config{backend_config};

    std::cout << config.net_config.hostname << "\n";

    std::unique_ptr<Server> server;
    server = std::make_unique<Server>(config.net_config);
    server->recv_data(); */

    std::string mode(argv[1]);
    std::string close("CLOSE"); 

    if (mode == "qpu") {
        QPU qpu_prueba(backend_config);
        qpu_prueba.turn_ON();
    } else if (mode == "cliente") {
        const auto circuit = to_string(execution_config);
        Client cliente{};
        cliente.connect(0);
        cliente.send_data(circuit);
        
        auto result = cliente.read_result();
        std::cout << result << "\n";
        
        cliente.send_data(close);
        
        result = cliente.read_result();
        std::cout << result << "\n";
        
        cliente.stop();
    } else {
        std::cerr << "Argumento no válido: " << mode << std::endl;
        std::cerr << "Uso: " << argv[0] << " <qpu|cliente>" << std::endl;
        return 1; // Error: argumento no reconocido
    }
    
    return 0;
}