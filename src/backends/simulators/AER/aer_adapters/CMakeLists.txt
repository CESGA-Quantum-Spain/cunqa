add_library(aer_simulator_adapter "${CMAKE_CURRENT_SOURCE_DIR}/aer_simulator_adapter.cpp")
target_include_directories(aer_simulator_adapter PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                                         "${PYBIND_PATH}/include"
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(aer_simulator_adapter PUBLIC json 
                                            PRIVATE logger_qpu LAPACK::LAPACK ${Python_LIBRARIES})