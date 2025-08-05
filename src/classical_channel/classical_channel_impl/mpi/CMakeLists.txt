
message(STATUS "Classical channel uses MPI")
add_library(classical_channel STATIC mpi_classical_channel.cpp)
target_include_directories(classical_channel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                     "${CMAKE_SOURCE_DIR}/src/classical_channel")
target_link_libraries(classical_channel PRIVATE MPI::MPI_CXX logger_qpu)
target_compile_definitions(classical_channel PRIVATE USE_MPI_BTW_QPU)