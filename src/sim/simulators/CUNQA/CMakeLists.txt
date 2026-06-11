add_library(cunqa_simulator_adapter STATIC)

target_sources(cunqa_simulator_adapter
  PRIVATE
    cunqa_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/cunqa_simulator_adapter.hpp"
)

target_include_directories(cunqa_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
)
target_link_libraries(cunqa_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE 
    cunqasimulator 
    logger_qpu 
    ${Python_LIBRARIES}
)
