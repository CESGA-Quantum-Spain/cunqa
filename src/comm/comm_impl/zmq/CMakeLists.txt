# Server
add_library(server "${CMAKE_CURRENT_SOURCE_DIR}/zmq_server.cpp")           
target_link_libraries(server PRIVATE logger_qpu cppzmq 
                             PUBLIC json)

# Client
add_library(client STATIC "${CMAKE_CURRENT_SOURCE_DIR}/zmq_client.cpp")
target_link_libraries(client PRIVATE logger_client cppzmq)