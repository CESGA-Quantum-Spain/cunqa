add_library(munich_adapters "${CMAKE_CURRENT_SOURCE_DIR}/munich_simulator_adapter.cpp")
target_include_directories(munich_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(munich_adapters PUBLIC classical_channel MQT::DDSim 
                                      PRIVATE json logger_qpu)