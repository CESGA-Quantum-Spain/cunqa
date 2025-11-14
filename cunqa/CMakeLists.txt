
pybind11_add_module(qclient bindings.cpp)

target_link_libraries(qclient PRIVATE client) #${GNUTLS_LIB} ${SODIUM_LIB} bsd)

install(TARGETS qclient DESTINATION cunqa)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.py"
    @ONLY
)

file(GLOB PYTHON_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/constants.py" DESTINATION cunqa)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/qiskit_deps/noise_instructions.py" 
              "${CMAKE_CURRENT_SOURCE_DIR}/qiskit_deps/cunqabackend.py" 
        DESTINATION cunqa/qiskit_deps)
