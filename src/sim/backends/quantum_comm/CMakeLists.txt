add_library(comm_model STATIC)
target_sources(comm_model
  PRIVATE
    comm_model.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/comm_model.hpp"
)
target_link_libraries(comm_model
  PUBLIC
    json
    noise
    timing
)
set_target_properties(comm_model
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(qc_engine STATIC)
target_sources(qc_engine
  PRIVATE
    qc_engine.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qc_engine.hpp"
)

target_include_directories(qc_engine
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(qc_engine
  PUBLIC
    simulator
    comm_model
    run_config
    dynamic_circuit
  PRIVATE
    noise
    timing
    logger_executor
)

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
    qc_engine
  PRIVATE
    comm_model
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
    comm_model
    json
)
