
message(STATUS "Classical channel uses ZMQ")
add_library(classical_channel)
target_sources(classical_channel
  PRIVATE
    zmq_classical_channel.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
    FILES
      "${CMAKE_SOURCE_DIR}/src/comm/classical_channel.hpp"
)
target_link_libraries(classical_channel 
  PUBLIC 
    json 
  PRIVATE 
    logger_qpu 
    cppzmq
    constants
)
target_compile_definitions(classical_channel PUBLIC USE_ZMQ_BTW_QPU)