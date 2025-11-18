
message(STATUS "Classical channel uses ZMQ")
add_library(classical_channel STATIC zmq_classical_channel.cpp)
target_link_libraries(classical_channel PRIVATE json logger_qpu cppzmq)
target_compile_definitions(classical_channel PUBLIC USE_ZMQ_BTW_QPU)