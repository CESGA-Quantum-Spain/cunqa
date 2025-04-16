# QRAISE executable
add_executable(qraise qraise.cpp)
target_include_directories(qraise PRIVATE "${CMAKE_SOURCE_DIR}/src" 
                                          "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                          "${CMAKE_SOURCE_DIR}/src/third-party/cunqasimulator/src"
                                          "${CMAKE_SOURCE_DIR}/src/third-party/argparse/include/argparse")
target_link_libraries(qraise PRIVATE custom-json logger_client
                                     nlohmann_json::nlohmann_json 
                                     Threads::Threads
                                     m ${Python_LIBRARIES} MPI::MPI_CXX
                                     OpenMP::OpenMP_CXX LAPACK::LAPACK stdc++fs)
install(TARGETS qraise DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QDROP executable
include(${CMAKE_SOURCE_DIR}/cmake/FindSLURM.cmake)

add_executable(qdrop qdrop.cpp)
target_include_directories(qdrop PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/argparse/include/argparse")
target_link_libraries(qdrop PRIVATE logger_client SLURM::SLURM 
                                    nlohmann_json::nlohmann_json 
                                    Threads::Threads
                                    m ${Python_LIBRARIES} MPI::MPI_CXX
                                    OpenMP::OpenMP_CXX LAPACK::LAPACK stdc++fs)
install(TARGETS qdrop DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/epilog.sh" DESTINATION "${CMAKE_INSTALL_BINDIR}")

if("$ENV{LMOD_SYSTEM_NAME}" STREQUAL "QMIO")
    install(FILES "slurm_qmio.conf" DESTINATION "${CMAKE_INSTALL_PREFIX}" RENAME "slurm.conf")
else()
    install(FILES "slurm_ft3.conf" DESTINATION "${CMAKE_INSTALL_PREFIX}" RENAME "slurm.conf")
endif()



get_directory_property(LINK_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} LINK_DIRECTORIES)
message(STATUS "Link directories globales: ${LINK_DIRS}")

# SETUP_QPUS executable
add_executable(setup_qpus setup_qpus.cpp)

message(STATUS "libzmq_LIBRARIES: ${libzmq_LIBRARIES}")
target_include_directories(setup_qpus PRIVATE "${CMAKE_SOURCE_DIR}/src" 
                                              "${CMAKE_SOURCE_DIR}/src/third-party/aer-cpp/src"
                                              "${CMAKE_SOURCE_DIR}/src/third-party/cunqasimulator/src"
                                              "${libzmq_INCLUDE_DIRS}"
                                              "${libzmq_BINARY_DIR}"
                                              "${CMAKE_SOURCE_DIR}/src/third-party/zmq"
                                              "${PROJECT_BINARY_DIR}/src/comm"
                                              "${PYBIND_PATH}/include")
target_link_libraries(setup_qpus PRIVATE custom-json logger_qpu
                             nlohmann_json::nlohmann_json 
                             Threads::Threads MQT::DDSim
                             m ${Python_LIBRARIES} MPI::MPI_CXX
                             OpenMP::OpenMP_CXX LAPACK::LAPACK
                             libzmq)
target_compile_options(setup_qpus PRIVATE -fvisibility=hidden)
target_link_options(setup_qpus PRIVATE -Wl,-Bsymbolic)

install(TARGETS setup_qpus DESTINATION "${CMAKE_INSTALL_BINDIR}")

# INFOQPUS executable
add_executable(infoqpus infoqpus.cpp)
target_include_directories(infoqpus PRIVATE "${CMAKE_SOURCE_DIR}/src/third-party/argparse/include/argparse")
install(TARGETS infoqpus DESTINATION "${CMAKE_INSTALL_BINDIR}")


