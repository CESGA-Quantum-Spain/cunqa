add_subdirectory(maestro_adapters)

# Simple simulator
add_library(maestro_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/maestro_simple_simulator.cpp")
target_include_directories(maestro_simple_simulator PRIVATE "${PYBIND_PATH}/include")
target_link_libraries(maestro_simple_simulator PUBLIC json 
                                           PRIVATE logger_qpu maestro_adapters)

# Classical communications simulator
add_library(maestro_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/maestro_cc_simulator.cpp")
target_include_directories(maestro_cc_simulator PRIVATE "${PYBIND_PATH}/include")
target_link_libraries(maestro_cc_simulator PUBLIC json 
                                       PRIVATE logger_qpu maestro_adapters)

# Quantum communications simulator
add_library(maestro_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/maestro_qc_simulator.cpp")
target_link_libraries(maestro_qc_simulator PUBLIC quantum_task classical_channel json
                                          PRIVATE logger_qpu maestro_adapters)

# Executor of circuits with quantum communications
add_library(maestro_executor "${CMAKE_CURRENT_SOURCE_DIR}/maestro_executor.cpp")
target_link_libraries(maestro_executor PUBLIC classical_channel json
                                      PRIVATE quantum_task logger_qpu maestro_adapters)

