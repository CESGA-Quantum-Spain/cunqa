add_subdirectory(circuit)
add_subdirectory(qiskit_deps)
add_subdirectory(real_qpus)

if (COMPILATION_FOR_GPU)
    set(QCLIENT_NAME "qclientgpu${GPU_ARCH}")
else()
    set(QCLIENT_NAME "qclient")
endif()

pybind11_add_module(${QCLIENT_NAME} bindings.cpp)

target_include_directories(${QCLIENT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/utils")
target_link_libraries(${QCLIENT_NAME} PRIVATE client json) #${GNUTLS_LIB} ${SODIUM_LIB} bsd)

install(TARGETS ${QCLIENT_NAME} DESTINATION cunqa)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.py"
    @ONLY
)

file(GLOB PYTHON_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/constants.py" DESTINATION cunqa)
