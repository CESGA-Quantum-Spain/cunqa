
add_library(backend INTERFACE)
target_sources(backend
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/backend.hpp"
)
target_link_libraries(backend
  INTERFACE
    json
)

add_library(run_config INTERFACE)
target_sources(run_config
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/run_config.hpp"
)
target_link_libraries(run_config
  INTERFACE
    json
)

add_library(simulator INTERFACE)
target_sources(simulator
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/simulator.hpp"
)
target_link_libraries(simulator
  INTERFACE
    run_config
    dynamic_circuit
)

add_subdirectory(simulators)
add_subdirectory(backends)