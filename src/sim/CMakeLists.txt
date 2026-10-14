
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
    timing
)

add_library(timing INTERFACE)
target_sources(timing
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/timing.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/random_delay.hpp"
)
target_link_libraries(timing
  INTERFACE
    json
    dynamic_circuit
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

# Noise channels applied instruction by instruction (idle relaxation, gate and readout errors).
add_library(noise INTERFACE)
target_sources(noise
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/noise/channels.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/noise/channel_applier.hpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/noise/device_noise.hpp"
)
target_link_libraries(noise
  INTERFACE
    simulator
    json
)

add_subdirectory(simulators)
add_subdirectory(backends)