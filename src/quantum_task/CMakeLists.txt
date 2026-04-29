add_library(circuit INTERFACE)
target_sources(circuit
    INTERFACE
        circuit.hpp
)
target_link_libraries(circuit INTERFACE json)

add_library(run_config INTERFACE)
target_sources(run_config
    INTERFACE
        run_config.hpp
)
target_link_libraries(run_config INTERFACE json)

add_library(quantum_task INTERFACE)
target_sources(quantum_task
    INTERFACE
        quantum_task.hpp
)
target_link_libraries(quantum_task INTERFACE circuit run_config)