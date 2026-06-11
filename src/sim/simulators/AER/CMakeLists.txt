add_library(AER_simulator_adapter STATIC)

if(USE_GPU)
  set_source_files_properties(
    AER_simulator_adapter.cpp PROPERTIES LANGUAGE CUDA
  )
  set_target_properties(AER_simulator_adapter
    PROPERTIES
      CUDA_ARCHITECTURES "${GPU_ARCH}"
      LANGUAGE CUDA
      CUDA_STANDARD 20
      CUDA_STANDARD_REQUIRED ON
  )
endif()
target_sources(AER_simulator_adapter
  PRIVATE
    AER_simulator_adapter.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src/sim/simulators"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/AER_simulator_adapter.hpp"
)

target_include_directories(AER_simulator_adapter
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(AER_simulator_adapter 
  PUBLIC
    simulator
    circuit
    json
  PRIVATE 
    aer_headers 
    logger_qpu 
    LAPACK::LAPACK 
    ${Python_LIBRARIES}
    OpenMP::OpenMP_CXX
)

set_target_properties(AER_simulator_adapter 
  PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
)