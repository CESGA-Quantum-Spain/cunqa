add_library(munich_simulator_adapter STATIC)

target_sources(munich_simulator_adapter
  PRIVATE
    munich_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/munich_simulator_adapter.hpp"
)

target_include_directories(munich_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
) 

target_link_libraries(munich_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE
    logger_qpu
    MQT::DDSim
)