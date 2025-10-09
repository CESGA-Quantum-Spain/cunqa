add_library(aer_adapters "${CMAKE_CURRENT_SOURCE_DIR}/aer_simulator_adapter.cpp")
target_include_directories(aer_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                         "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                                         "${PYBIND_PATH}/include"
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(aer_adapters PUBLIC classical_channel 
                                            PRIVATE json logger_qpu LAPACK::LAPACK ${Python_LIBRARIES})
target_link_libraries(aer_adapters PRIVATE AerCPP)
