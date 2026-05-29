
add_library(cc_executor)
target_sources(cc_executor
  PRIVATE
    cc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/cc_executor.hpp"
)

target_link_libraries(cc_executor 
  PUBLIC 
    backend 
    simulator 
    run_config
    circuit
    classical_channel
    json
  PRIVATE
    dynamic_circuit
    logger_qpu
)

add_library(cc_backend INTERFACE)
target_sources(cc_backend
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/cc_backend.hpp"
)

target_link_libraries(cc_backend 
  INTERFACE 
    backend
    simulator
    cc_executor   
    json
    logger_qpu
)