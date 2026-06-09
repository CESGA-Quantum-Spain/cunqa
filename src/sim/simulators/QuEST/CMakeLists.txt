add_library(quest_simulator_adapter STATIC)

target_sources(quest_simulator_adapter
  PRIVATE
    quest_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/quest_simulator_adapter.hpp"
)

target_include_directories(quest_simulator_adapter 
  PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_PROJECT_SOURCE}/src"
) 

target_link_libraries(quest_simulator_adapter 
  PUBLIC 
    json
    simulator
    circuit
  PRIVATE
    logger_qpu
    QuEST::QuEST
)