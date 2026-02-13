# QRAISE executable

set(QRAISE_NAME "qraise")
add_executable(${QRAISE_NAME} qraise.cpp)
target_link_libraries(${QRAISE_NAME} PRIVATE json logger_client morrisfranken::argparse)

if(USE_MPI_BTW_QPU)
    target_compile_definitions(${QRAISE_NAME} PUBLIC USE_MPI_BTW_QPU)
elseif(USE_ZMQ_BTW_QPU)
    target_compile_definitions(${QRAISE_NAME} PUBLIC USE_ZMQ_BTW_QPU)
endif()
if (COMPILATION_FOR_GPU)
    target_compile_definitions(${QRAISE_NAME} PUBLIC GPU_ARCH=${GPU_ARCH} 
                                         MAX_GPUS_PER_NODE=${MAX_GPUS_PER_NODE}
                                         COMPILATION_FOR_GPU=1)
else()
    target_compile_definitions(${QRAISE_NAME} PUBLIC COMPILATION_FOR_GPU=0)
endif()
install(TARGETS ${QRAISE_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QDROP executable
add_executable(qdrop qdrop.cpp)
target_link_libraries(qdrop PRIVATE json morrisfranken::argparse)
install(TARGETS qdrop DESTINATION "${CMAKE_INSTALL_BINDIR}")

# SETUP_QPUS executable

set(SETUP_QPUS_NAME "setup_qpus")
add_executable(${SETUP_QPUS_NAME} setup_qpus.cpp)
target_link_libraries(${SETUP_QPUS_NAME} PRIVATE json logger_qpu qpu 
                                         aer_simple_simulator aer_cc_simulator aer_qc_simulator
                                         munich_simple_simulator munich_cc_simulator munich_qc_simulator 
                                         cunqa_simple_simulator cunqa_cc_simulator cunqa_qc_simulator
                                         maestro_simple_simulator maestro_cc_simulator maestro_qc_simulator
                                         qulacs_simple_simulator qulacs_cc_simulator qulacs_qc_simulator)
target_include_directories(${SETUP_QPUS_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
if (COMPILATION_FOR_GPU)
    target_compile_definitions(${SETUP_QPUS_NAME} PUBLIC GPU_ARCH=${GPU_ARCH} 
                                         COMPILATION_FOR_GPU=1)
else()
    target_compile_definitions(${SETUP_QPUS_NAME} PUBLIC COMPILATION_FOR_GPU=0)
endif()
install(TARGETS ${SETUP_QPUS_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")

# EXECUTOR executable

set(SETUP_EXECUTOR_NAME "setup_executor")
add_executable(${SETUP_EXECUTOR_NAME} setup_executor.cpp)
target_link_libraries(${SETUP_EXECUTOR_NAME} PRIVATE aer_executor munich_executor cunqa_executor maestro_executor qulacs_executor logger_executor)
target_include_directories(${SETUP_EXECUTOR_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
if (COMPILATION_FOR_GPU)
    target_compile_definitions(${SETUP_EXECUTOR_NAME} PUBLIC GPU_ARCH=${GPU_ARCH} 
                                                  COMPILATION_FOR_GPU=1)
else()
    target_compile_definitions(${SETUP_EXECUTOR_NAME} PUBLIC COMPILATION_FOR_GPU=0)
endif()
install(TARGETS ${SETUP_EXECUTOR_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QINFO executable
add_executable(qinfo qinfo.cpp)
target_link_libraries(qinfo PRIVATE json logger_client morrisfranken::argparse)
install(TARGETS qinfo DESTINATION "${CMAKE_INSTALL_BINDIR}")

# SETUP_QMIO executable
add_executable(setup_qmio setup_qmio.cpp)
target_link_libraries(setup_qmio PRIVATE json cppzmq server logger_qpu)
target_include_directories(setup_qmio PRIVATE   "${CMAKE_SOURCE_DIR}/src"
                                                "${ZMQ_BINARY_DIR}"
                                                "${ZMQ_INCLUDE_DIR}"
                                                "${CMAKE_SOURCE_DIR}/src/third-party/zmq")
target_link_directories(setup_qmio PRIVATE "${ZMQ_BINARY_DIR}/lib")
target_include_directories(setup_qmio PRIVATE )
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


