add_subdirectory(no_comm)
add_subdirectory(classical_comm)
add_subdirectory(quantum_comm)

add_library(backend_factory)
target_sources(backend_factory
  PRIVATE
    backend_factory.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      backend_factory.hpp
)
target_link_libraries(backend_factory
  PUBLIC 
    backend
    simulator 
    json
  PRIVATE 
    nc_backend
    cc_backend
    qc_backend
)
target_include_directories(backend_factory PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
