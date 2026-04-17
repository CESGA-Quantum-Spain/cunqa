add_subdirectory(quest_adapters)

# Simple simulator
add_library(quest_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/quest_simple_simulator.cpp")
target_link_libraries(quest_simple_simulator PUBLIC json
                                           PRIVATE logger_qpu quest_adapters)

# Classical communications simulator
add_library(quest_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/quest_cc_simulator.cpp")
target_link_libraries(quest_cc_simulator PUBLIC json
                                       PRIVATE logger_qpu quest_adapters)

# Quantum communications simulator
add_library(quest_qc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/quest_qc_simulator.cpp")
target_link_libraries(quest_qc_simulator PUBLIC quantum_task classical_channel json
                                       PRIVATE logger_qpu quest_adapters)

# Executor of circuits with quantum communications
add_library(quest_executor "${CMAKE_CURRENT_SOURCE_DIR}/quest_executor.cpp")
target_link_libraries(quest_executor PUBLIC classical_channel json
                                   PRIVATE quantum_task logger_qpu quest_adapters)