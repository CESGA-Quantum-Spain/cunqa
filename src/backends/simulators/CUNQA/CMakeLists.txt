
add_library(cunqa_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_simple_simulator.cpp")
target_include_directories(cunqa_simple_simulator PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/cunqasimulator")
target_link_libraries(cunqa_simple_simulator PUBLIC json
                                             PRIVATE cunqasimulator logger_qpu)


add_library(cunqa_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_cc_simulator.cpp")
target_include_directories(cunqa_cc_simulator PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/cunqasimulator")
target_link_libraries(cunqa_cc_simulator PUBLIC json classical_channel
                                         PRIVATE cunqasimulator logger_qpu)



