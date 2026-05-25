add_library(qc_executor)
target_sources(qc_executor
  PRIVATE
    qc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      qc_executor.hpp
)

target_link_libraries(qc_executor 
  PUBLIC
    simulator
    classical_channel
  PRIVATE
    constants
    quantum_task
    logger_executor
)

add_library(qc_backend INTERFACE)
target_sources(qc_backend
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      qc_backend.hpp
)
target_link_libraries(qc_backend 
  INTERFACE
    logger_qpu
)
