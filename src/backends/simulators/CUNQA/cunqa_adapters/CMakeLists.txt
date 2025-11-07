add_library(cunqa_adapters "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_simulator_adapter.cpp")
target_include_directories(cunqa_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                            "${CMAKE_SOURCE_DIR}/src/third-party/cunqasimulator")
target_link_libraries(cunqa_adapters PUBLIC classical_channel json
                                            PRIVATE cunqasimulator logger_qpu ${Python_LIBRARIES})
