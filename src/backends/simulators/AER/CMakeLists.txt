add_subdirectory(aer_adapters)

add_library(aer_cc_simulator "${CMAKE_CURRENT_SOURCE_DIR}/aer_cc_simulator.cpp")
target_include_directories(aer_cc_simulator PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                                 "${PYBIND_PATH}/include")
target_link_libraries(aer_cc_simulator PUBLIC json 
                                       PRIVATE logger_qpu aer_simulator_adapter)

add_library(aer_simple_simulator "${CMAKE_CURRENT_SOURCE_DIR}/aer_simple_simulator.cpp")
target_include_directories(aer_simple_simulator PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                                 "${PYBIND_PATH}/include")
target_link_libraries(aer_simple_simulator PUBLIC json 
                                           PRIVATE logger_qpu aer_simulator_adapter)