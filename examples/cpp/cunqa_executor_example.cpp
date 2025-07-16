#include "backends/simulators/CUNQA/cunqa_executors.hpp"

const std::string circuit = R"(
{
    "id": "circuito1",
    "config": {
        "shots": 1024,
        "method": "statevector",
        "num_clbits": 2,
        "num_qubits": 30
    },
    "instructions": [
    {
        "name": "h",
        "qubits": [0]
    },
    {
        "name": "h",
        "qubits": [24]
    },
    {
        "name": "h",
        "qubits": [29]
    },
    {
        "name": "cx",
        "qubits": [24, 7]
    },
    {
        "name": "cx",
        "qubits": [0, 1]
    },
    {
        "name": "measure",
        "qubits": [0],
        "clreg": [0]
    },
    {
        "name": "measure",
        "qubits": [1],
        "clreg": [1]
    }
    ]
}
)";


int main() 
{
    cunqa::QuantumTask quantum_task{circuit};
    auto result = cunqa_execution_(quantum_task);

    std::cout << "Counts: " << result.at("counts").dump() << "\n";
}
    
