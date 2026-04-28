# QRAISE executable
add_executable(qraise qraise.cpp)
target_link_libraries(qraise PRIVATE json logger_client morrisfranken::argparse)

if(USE_MPI_BTW_QPU)
    target_compile_definitions(qraise PUBLIC USE_MPI_BTW_QPU)
elseif(USE_ZMQ_BTW_QPU)
    target_compile_definitions(qraise PUBLIC USE_ZMQ_BTW_QPU)
endif()

if (COMPILATION_FOR_GPU)
    target_compile_definitions(qraise PUBLIC GPU_ARCH=${GPU_ARCH} COMPILATION_FOR_GPU=1)
else()
    target_compile_definitions(qraise PUBLIC COMPILATION_FOR_GPU=0)
endif()

install(TARGETS qraise DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QDROP executable
add_executable(qdrop qdrop.cpp)
target_link_libraries(qdrop PRIVATE json morrisfranken::argparse)
install(TARGETS qdrop DESTINATION "${CMAKE_INSTALL_BINDIR}")

# SETUP_QPUS executable
add_executable(setup_qpus setup_qpus.cpp)
target_link_libraries(setup_qpus
    PRIVATE 
    json 
    qpu
    logger_qpu
    nc_executor
    simulator
    simulator_factory
)

if (COMPILATION_FOR_GPU)
    target_compile_definitions(setup_qpus PUBLIC GPU_ARCH=${GPU_ARCH} COMPILATION_FOR_GPU=1)
else()
    target_compile_definitions(setup_qpus PUBLIC COMPILATION_FOR_GPU=0)
endif()

install(TARGETS setup_qpus DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QINFO executable
add_executable(qinfo qinfo.cpp)
target_link_libraries(qinfo PRIVATE json logger_client morrisfranken::argparse)
install(TARGETS qinfo DESTINATION "${CMAKE_INSTALL_BINDIR}")

# SETUP_QMIO executable
add_executable(setup_qmio setup_qmio.cpp)
target_link_libraries(setup_qmio PRIVATE json cppzmq server logger_qpu)
target_include_directories(setup_qmio 
    PRIVATE   
        "${ZMQ_BINARY_DIR}"
        "${ZMQ_INCLUDE_DIR}"
)
target_link_directories(setup_qmio PRIVATE "${ZMQ_BINARY_DIR}/lib")
install(TARGETS setup_qmio DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Epilog tool erase_key
add_executable(erase_key erase_key.cpp)
target_link_libraries(erase_key PRIVATE json)
install(TARGETS erase_key DESTINATION "${CMAKE_INSTALL_BINDIR}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/epilog.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/epilog.sh"
    @ONLY
)
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/epilog.sh" DESTINATION "$ENV{STORE}/.cunqa") 


