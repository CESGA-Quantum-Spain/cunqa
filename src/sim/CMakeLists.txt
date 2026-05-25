add_library(backend INTERFACE)
target_sources(backend
  INTERFACE
    backend.hpp
)
target_link_libraries(backend INTERFACE quantum_task)

add_library(simulator INTERFACE)
target_sources(simulator
  INTERFACE
    simulator.hpp
)
target_link_libraries(simulator INTERFACE quantum_task)

add_subdirectory(simulators)
add_subdirectory(backends)