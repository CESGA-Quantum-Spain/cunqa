# Server
add_library(server STATIC) 
target_sources(server
  PRIVATE
    zmq_server.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
    FILES
      "${CMAKE_SOURCE_DIR}/src/comm/server.hpp"
)          
target_link_libraries(server 
  PRIVATE 
    logger_qpu 
    cppzmq
  PUBLIC 
    json)

# Client
add_library(client STATIC)
target_sources(client
  PRIVATE
    zmq_client.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
    FILES
      "${CMAKE_SOURCE_DIR}/src/comm/client.hpp"
)     
target_link_libraries(client 
  PRIVATE 
    logger_client 
    cppzmq
)
set_target_properties(client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)