# QRAISE executable
add_executable(qraise qraise.cpp)
target_link_libraries(qraise PRIVATE json logger_client morrisfranken::argparse)

if(USE_MPI_BTW_QPU)
    target_compile_definitions(qraise PUBLIC USE_MPI_BTW_QPU)
elseif(USE_ZMQ_BTW_QPU)
    target_compile_definitions(qraise PUBLIC USE_ZMQ_BTW_QPU)
endif()

install(TARGETS qraise DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QDROP executable
add_executable(qdrop qdrop.cpp)
target_link_libraries(qdrop PRIVATE json morrisfranken::argparse)
install(TARGETS qdrop DESTINATION "${CMAKE_INSTALL_BINDIR}")

# SETUP_QPUS executable
add_executable(setup_qpus setup_qpus.cpp)
target_link_libraries(setup_qpus PRIVATE json logger_qpu qpu 
                                         aer_simple_simulator aer_cc_simulator aer_qc_simulator
                                         munich_simple_simulator munich_cc_simulator munich_qc_simulator 
                                         cunqa_simple_simulator cunqa_cc_simulator cunqa_qc_simulator)
target_include_directories(setup_qpus PRIVATE "${CMAKE_SOURCE_DIR}/src")
install(TARGETS setup_qpus DESTINATION "${CMAKE_INSTALL_BINDIR}")

# EXECUTOR executable
add_executable(setup_executor setup_executor.cpp)
target_link_libraries(setup_executor PRIVATE aer_executor munich_executor cunqa_executor logger_executor)
target_include_directories(setup_executor PRIVATE "${CMAKE_SOURCE_DIR}/src")
install(TARGETS setup_executor DESTINATION "${CMAKE_INSTALL_BINDIR}")

# QINFO executable
add_executable(qinfo qinfo.cpp)
target_link_libraries(qinfo PRIVATE json logger_client morrisfranken::argparse)
install(TARGETS qinfo DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Epilog tool erase_key and noise models
add_executable(erase_key erase_key.cpp)
target_link_libraries(erase_key PRIVATE json)
install(TARGETS erase_key DESTINATION "${CMAKE_INSTALL_BINDIR}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/epilog.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/epilog.sh"
    @ONLY
)
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/epilog.sh" DESTINATION "$ENV{STORE}/.cunqa") 


