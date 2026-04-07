add_subdirectory(qsim_adapters)

# Simple simulator
add_library(qsim_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qsim_simple_simulator.cpp")
target_link_libraries(qsim_simple_simulator PUBLIC json
                                           PRIVATE logger_qpu qsim_adapters)

# Classical communications simulator
add_library(qsim_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qsim_cc_simulator.cpp")
target_link_libraries(qsim_cc_simulator PUBLIC json
                                       PRIVATE logger_qpu qsim_adapters)

# Quantum communications simulator
add_library(qsim_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/qsim_qc_simulator.cpp")
target_link_libraries(qsim_qc_simulator PUBLIC quantum_task classical_channel json
                                       PRIVATE logger_qpu qsim_adapters)

# Executor of circuits with quantum communications
add_library(qsim_executor "${CMAKE_CURRENT_SOURCE_DIR}/qsim_executor.cpp")
target_link_libraries(qsim_executor PUBLIC classical_channel json
                                   PRIVATE quantum_task logger_qpu qsim_adapters)