#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
// or, if you only need macros:
// #include <catch2/catch_test_macros.hpp>
#include "backends/simulators/Munich/munich_adapters/circuit_simulator_adapter.hpp"
#include "backends/simulators/Munich/munich_adapters/quantum_computation_adapter.hpp"

const std::string circuit = R"(
{
    "id": "circuito1",
    "config": {
        "shots": 1024,
        "method": "statevector",
        "num_clbits": 2,
        "num_qubits": 2
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


TEST_CASE("Simulation of Bell pair") {
    cunqa::QuantumTask quantum_task{circuit};
    auto qc = std::make_unique<cunqa::sim::QuantumComputationAdapter>(quantum_task);

    cunqa::sim::CircuitSimulatorAdapter simulator(std::move(qc));
    auto result = simulator.simulate(1024);

    REQUIRE( result.at("counts").find("00") != result.at("counts").end() );
    REQUIRE( result.at("counts").find("11") != result.at("counts").end() );
    REQUIRE( result.at("counts").find("01") == result.at("counts").end() );
    REQUIRE( result.at("counts").find("10") == result.at("counts").end() );

    auto total = result.at("counts").at("00").get<int>() + result.at("counts").at("11").get<int>();
    REQUIRE( total == 1024 );
}