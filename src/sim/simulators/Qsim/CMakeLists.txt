add_library(qsim_simulator_adapter STATIC)

target_sources(qsim_simulator_adapter
  PRIVATE
    qsim_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qsim_simulator_adapter.hpp"
)

target_include_directories(qsim_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
    "${qsim_SOURCE_DIR}/lib" 
) 

target_link_libraries(qsim_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE
    logger_qpu
    MQT::DDSim
)