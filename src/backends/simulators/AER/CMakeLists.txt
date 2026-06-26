add_subdirectory(aer_adapters)

# Simple simulator
add_library(aer_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/aer_simple_simulator.cpp")
target_link_libraries(aer_simple_simulator PUBLIC json aer_internal
                                           PRIVATE logger_qpu aer_adapters)

# Classical communications simulator
add_library(aer_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/aer_cc_simulator.cpp")
target_link_libraries(aer_cc_simulator PUBLIC json aer_internal
                                       PRIVATE logger_qpu aer_adapters)

# Quantum communications simulator
add_library(aer_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/aer_qc_simulator.cpp")
target_link_libraries(aer_qc_simulator PUBLIC quantum_task classical_channel json aer_headers
                                       PRIVATE logger_qpu aer_adapters)

# Executor of circuits with quantum communications
add_library(aer_executor "${CMAKE_CURRENT_SOURCE_DIR}/aer_executor.cpp")
target_link_libraries(aer_executor PUBLIC classical_channel json aer_internal aer_headers                                   
    PRIVATE quantum_task logger_qpu aer_adapters)

