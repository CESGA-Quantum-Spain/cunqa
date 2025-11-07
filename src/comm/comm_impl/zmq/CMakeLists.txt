# Server
add_library(server "${CMAKE_CURRENT_SOURCE_DIR}/zmq_server.cpp")
target_include_directories(server PRIVATE "${ZMQ_BINARY_DIR}"
                                          "${ZMQ_INCLUDE_DIR}"
                                          "${CMAKE_SOURCE_DIR}/src/third-party/zmq")
target_link_directories(server PRIVATE "${ZMQ_BINARY_DIR}/lib")                       
target_link_libraries(server PRIVATE libzmq logger_qpu PUBLIC json)

# Client
add_library(client STATIC "${CMAKE_CURRENT_SOURCE_DIR}/zmq_client.cpp")
target_include_directories(client PRIVATE "${ZMQ_BINARY_DIR}"
                                          "${ZMQ_INCLUDE_DIR}"
                                          "${CMAKE_SOURCE_DIR}/src/third-party/zmq")
target_link_directories(client PRIVATE "${ZMQ_BINARY_DIR}/lib")  
target_link_libraries(client PRIVATE libzmq logger_client)
set_target_properties(client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
