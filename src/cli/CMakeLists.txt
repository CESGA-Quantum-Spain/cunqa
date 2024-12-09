add_executable(qraise qraise.cpp)

target_include_directories(qraise PRIVATE "${CMAKE_SOURCE_DIR}/src" 
                                          "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                          "${CMAKE_SOURCE_DIR}/src/third-party/argparse/include/argparse")
                              
message(STATUS "Python Libraries: ${Python_LIBRARIES}")

target_link_libraries(qraise PUBLIC compiler_flags
                             INTERFACE API-Simulator
                             PRIVATE custom-json 
                                     nlohmann_json::nlohmann_json 
                                     Threads::Threads 
                                     m ${Python_LIBRARIES} MPI::MPI_CXX
                                     OpenMP::OpenMP_CXX LAPACK::LAPACK)

install(TARGETS qraise DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/epilog.sh" DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(setup_qpus setup_qpus.cpp)

message(STATUS "${PROJECT_BINARY_DIR}")
target_include_directories(setup_qpus PRIVATE "${CMAKE_SOURCE_DIR}/src" 
                                              "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                              "${PROJECT_BINARY_DIR}/src/comm")
target_link_libraries(setup_qpus PUBLIC compiler_flags
                             INTERFACE API-Simulator
                             PRIVATE custom-json 
                             nlohmann_json::nlohmann_json 
                             Threads::Threads 
                             m ${Python_LIBRARIES} MPI::MPI_CXX
                             OpenMP::OpenMP_CXX LAPACK::LAPACK)

install(TARGETS setup_qpus DESTINATION ${CMAKE_INSTALL_BINDIR})