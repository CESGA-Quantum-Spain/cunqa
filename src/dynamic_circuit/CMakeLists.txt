
add_library(dynamic_circuit)

target_sources(dynamic_circuit
  PRIVATE  
    dynamic_circuit.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_circuit.hpp"
)

target_include_directories(dynamic_circuit
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(dynamic_circuit
  PUBLIC
    circuit
    json
  PRIVATE
    logger_qpu
)
