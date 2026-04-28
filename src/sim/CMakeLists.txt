add_library(backend INTERFACE)
target_sources(backend
  INTERFACE
    backend.hpp
)
target_include_directories(backend
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(backend INTERFACE quantum_task)

add_library(simulator INTERFACE)
target_sources(simulator
  INTERFACE
    simulator.hpp
)
target_include_directories(simulator
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(simulator INTERFACE quantum_task)

add_subdirectory(no_comm)
#add_subdirectory(classical_comm)
#add_subdirectory(quantum_comm)
add_subdirectory(simulators)