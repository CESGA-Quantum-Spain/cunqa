pybind11_add_module(qclient bindings.cpp)

target_include_directories(qclient 
  PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/utils"
    "${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(qclient 
  PRIVATE 
    client
    json
)
install(TARGETS qclient DESTINATION cunqa)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.py"
    @ONLY
)

file(GLOB 
  PYTHON_SCRIPTS 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.py"
)
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/constants.py" DESTINATION cunqa)

file(GLOB CIRCUIT_PY "${CMAKE_CURRENT_SOURCE_DIR}/circuit/*.py")
install(FILES ${CIRCUIT_PY} DESTINATION cunqa/circuit)

file(GLOB QISKIT_DEPS_PY "${CMAKE_CURRENT_SOURCE_DIR}/qiskit_deps/*.py")
install(FILES ${QISKIT_DEPS_PY} DESTINATION cunqa/qiskit_deps)

file(GLOB REAL_QPUS_PY "${CMAKE_CURRENT_SOURCE_DIR}/real_qpus/*.py")
install(FILES ${REAL_QPUS_PY} DESTINATION cunqa/real_qpus)

file(GLOB UTILS_PY "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.py")
install(FILES ${UTILS_PY} DESTINATION cunqa/utils)

file(GLOB QC_PROTOCOLS_PY "${CMAKE_CURRENT_SOURCE_DIR}/qc_protocols/*.py")
install(FILES ${QC_PROTOCOLS_PY} DESTINATION cunqa/qc_protocols)