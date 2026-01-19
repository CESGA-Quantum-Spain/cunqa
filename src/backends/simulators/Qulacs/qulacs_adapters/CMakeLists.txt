add_library(qulacs_adapters "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_simulator_adapter.cpp")
target_include_directories(qulacs_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" 
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/.." 
                                                   "${qulacs_SOURCE_DIR}/src"
                                                   "${qulacs_BINARY_DIR}/eigen/src/eigen/")
target_link_libraries(qulacs_adapters PUBLIC classical_channel 
                                        PRIVATE json logger_qpu
                                        cppsim_static
                                        csim_static
                                        vqcsim_static
                                        OpenMP::OpenMP_CXX)