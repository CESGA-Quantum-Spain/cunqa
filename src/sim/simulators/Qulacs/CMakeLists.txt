add_library(qulacs_simulator_adapter STATIC)

target_sources(qulacs_simulator_adapter
  PRIVATE
    qulacs_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qulacs_simulator_adapter.hpp"
)

target_include_directories(qulacs_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
    "${qulacs_SOURCE_DIR}/src"
) 

target_link_libraries(qulacs_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE
    logger_qpu
    cppsim_static
    csim_static
    vqcsim_static
    Eigen3::Eigen
)