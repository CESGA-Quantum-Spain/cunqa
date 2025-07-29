add_subdirectory(munich_adapters)

# Simple simulator
add_library(munich_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/munich_simple_simulator.cpp")
target_link_libraries(munich_simple_simulator PUBLIC quantum_task json
                                              PRIVATE MQT::DDSim logger_qpu)

# Classical communications simulator
add_library(munich_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/munich_cc_simulator.cpp")
target_link_libraries(munich_cc_simulator PUBLIC quantum_task classical_channel json
                                          PRIVATE munich_adapters logger_qpu)

# Quantum communications simulator
add_library(munich_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/munich_qc_simulator.cpp")
target_link_libraries(munich_qc_simulator PUBLIC quantum_task classical_channel json
                                          PRIVATE logger_qpu)

# Executor of circuits with quantum communications
add_library(munich_executor "${CMAKE_CURRENT_SOURCE_DIR}/munich_executor.cpp")
target_link_libraries(munich_executor PUBLIC classical_channel json
                                      PRIVATE quantum_task logger_qpu munich_adapters)


