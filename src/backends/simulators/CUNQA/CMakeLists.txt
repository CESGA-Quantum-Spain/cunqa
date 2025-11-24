add_subdirectory(cunqa_adapters)

add_library(cunqa_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_simple_simulator.cpp")
target_link_libraries(cunqa_simple_simulator PUBLIC json
                                             PRIVATE cunqa_adapters logger_qpu)


add_library(cunqa_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_cc_simulator.cpp")
target_link_libraries(cunqa_cc_simulator PUBLIC json classical_channel
                                         PRIVATE cunqa_adapters logger_qpu)

add_library(cunqa_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_qc_simulator.cpp")
target_link_libraries(cunqa_qc_simulator PUBLIC json
                                             PRIVATE cunqa_adapters logger_qpu)


add_library(cunqa_executor "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_executor.cpp")
target_link_libraries(cunqa_executor PUBLIC classical_channel json
                                      PRIVATE cunqa_adapters quantum_task logger_qpu)



