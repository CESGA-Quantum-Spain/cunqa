
message(STATUS "Classical channel uses MPI")
add_library(classical_channel STATIC)
target_sources(classical_channel
  PRIVATE
    mpi_classical_channel.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
    FILES
      comm/classical_channel.hpp
)
target_link_libraries(classical_channel 
  PUBLIC
    json
  PRIVATE 
    MPI::MPI_CXX 
    logger_qpu
)
target_compile_definitions(classical_channel PRIVATE USE_MPI_BTW_QPU)