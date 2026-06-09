add_library(maestro_simulator_adapter STATIC)

target_sources(maestro_simulator_adapter
  PRIVATE
    maestro_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/maestro_simulator_adapter.hpp"
)

target_include_directories(maestro_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
)
target_link_libraries(maestro_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE 
    maestro 
    logger_qpu 
    ${Python_LIBRARIES}
)