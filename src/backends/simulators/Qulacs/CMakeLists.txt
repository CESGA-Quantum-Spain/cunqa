add_subdirectory(qulacs_adapters)

# Simple simulator
add_library(qulacs_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_simple_simulator.cpp")
target_link_libraries(qulacs_simple_simulator PUBLIC json
                                           PRIVATE logger_qpu qulacs_adapters qulacs_headers)

# Classical communications simulator
add_library(qulacs_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_cc_simulator.cpp")
target_link_libraries(qulacs_cc_simulator PUBLIC json
                                       PRIVATE logger_qpu qulacs_adapters)

# Quantum communications simulator
add_library(qulacs_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_qc_simulator.cpp")
target_link_libraries(qulacs_qc_simulator PUBLIC quantum_task classical_channel json
                                       PRIVATE logger_qpu qulacs_adapters)

# Executor of circuits with quantum communications
add_library(qulacs_executor "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_executor.cpp")
target_link_libraries(qulacs_executor PUBLIC classical_channel json
                                   PRIVATE quantum_task logger_qpu qulacs_adapters)