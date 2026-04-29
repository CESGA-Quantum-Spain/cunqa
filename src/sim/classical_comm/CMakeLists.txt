add_library(cc_executor)
target_sources(cc_executor
  PRIVATE
    cc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      cc_executor.hpp
      cc_backend.hpp
)
target_link_libraries(cc_executor 
  PUBLIC 
    backend 
    simulator 
    quantum_task
    classical_channel
    json
  PRIVATE
    logger_qpu
)

