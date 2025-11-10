add_library(maestro_adapters "${CMAKE_CURRENT_SOURCE_DIR}/maestro_simulator_adapter.cpp")
target_include_directories(maestro_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                         "${PYBIND_PATH}/include"
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(maestro_adapters PUBLIC classical_channel 
                                            PRIVATE json logger_qpu ${Python_LIBRARIES})