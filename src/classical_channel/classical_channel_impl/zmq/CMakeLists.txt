
message(STATUS "Classical channel uses ZMQ")
add_library(classical_channel STATIC zmq_classical_channel.cpp)
target_include_directories(classical_channel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                     "${ZMQ_BINARY_DIR}"
                                                     "${ZMQ_INCLUDE_DIR}"
                                                     "${CMAKE_SOURCE_DIR}/src/third-party/zmq"
                                                     "${CMAKE_SOURCE_DIR}/src/classical_channel")
target_link_directories(classical_channel PUBLIC "${ZMQ_BINARY_DIR}/lib")
target_link_libraries(classical_channel PRIVATE libzmq json logger_qpu)
target_compile_definitions(classical_channel PUBLIC USE_ZMQ_BTW_QPU)