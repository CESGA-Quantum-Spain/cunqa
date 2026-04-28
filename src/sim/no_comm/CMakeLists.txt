add_library(nc_executor)
target_sources(nc_executor
  PRIVATE
    nc_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      nc_executor.hpp
      nc_backend.hpp
)
target_link_libraries(nc_executor 
  PUBLIC 
    backend 
    simulator 
    quantum_task 
    json
  PRIVATE
    logger_qpu
)

