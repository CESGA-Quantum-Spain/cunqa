#include "backends/simulators/Munich/munich_adapters/circuit_simulator_adapter.hpp"
#include "backends/simulators/Munich/munich_adapters/quantum_computation_adapter.hpp"

const std::string circuit = R"(
{
    "id": "circuito1",
    "config": {
        "shots": 1024,
        "method": "statevector",
        "num_clbits": 2,
        "num_qubits": 25
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
        "qubits": [17]
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
    auto qc = std::make_unique<cunqa::sim::QuantumComputationAdapter>(quantum_task);

    cunqa::sim::CircuitSimulatorAdapter simulator(std::move(qc));
    auto result = simulator.simulate(1024);

    std::cout << "Counts: " << result.at("counts").dump() << "\n";
}
    
