add_library(qc_executor STATIC)
target_sources(qc_executor
  PRIVATE
    qc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qc_executor.hpp"
)

target_include_directories(qc_executor
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(qc_executor 
  PUBLIC
    simulator
    classical_channel
  PRIVATE
    constants
    dynamic_circuit
    logger_executor
    run_config
)

add_library(qc_backend INTERFACE)
target_sources(qc_backend
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qc_backend.hpp"
)
target_link_libraries(qc_backend 
  INTERFACE
    backend
    simulator
    classical_channel
    json
)
