
add_library(nc_executor)
target_sources(nc_executor
  PRIVATE
    nc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/nc_executor.hpp"
)

target_link_libraries(nc_executor 
  PUBLIC 
    backend
    simulator 
    run_config 
    circuit 
    json
  PRIVATE
    logger_qpu
    dynamic_circuit
)

add_library(nc_backend INTERFACE)
target_sources(nc_backend
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/nc_backend.hpp"
)

target_link_libraries(nc_backend 
  INTERFACE 
    backend
    simulator
    nc_executor   
    json
    logger_qpu
)