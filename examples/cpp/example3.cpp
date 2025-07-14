#include "utils/json.hpp"
#include <iostream>
#include "comm/client.hpp"

std::string circuit = R"(
{
    "config": {
        "shots": 1024,
        "method": "statevector",
        "num_clbits": 2,
        "num_qubits": 2
    },
    "instructions": [
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
)";

using namespace cunqa::comm;

int main(int argc, char *argv[])
{
    
    Client cliente{};
    cliente.connect(argv[1], argv[2]);

    cliente.send_circuit(circuit);
    auto result1 = cliente.recv_results();

    cliente.send_circuit(circuit);
    auto result2 = cliente.recv_results();

    std::cout << result1 << "\n";
    std::cout << result2 << "\n";    
    
    return 0;
}