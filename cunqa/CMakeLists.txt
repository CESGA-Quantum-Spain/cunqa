add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall> $<$<COMPILE_LANGUAGE:CXX>:-Wextra>)

pybind11_add_module(qclient qclient.cpp)

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

file(GLOB MAIN_PY "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${MAIN_PY} DESTINATION cunqa)

file(GLOB CIRCUIT_PY "${CMAKE_CURRENT_SOURCE_DIR}/circuit/*.py")
install(FILES ${CIRCUIT_PY} DESTINATION cunqa/circuit)

file(GLOB QISKIT_DEPS_PY "${CMAKE_CURRENT_SOURCE_DIR}/qiskit_deps/*.py")
install(FILES ${QISKIT_DEPS_PY} DESTINATION cunqa/qiskit_deps)

file(GLOB REAL_QPUS_PY "${CMAKE_CURRENT_SOURCE_DIR}/real_qpus/*.py")
install(FILES ${REAL_QPUS_PY} DESTINATION cunqa/real_qpus)

file(GLOB QC_PROTOCOLS_PY "${CMAKE_CURRENT_SOURCE_DIR}/qc_protocols/*.py")
install(FILES ${QC_PROTOCOLS_PY} DESTINATION cunqa/qc_protocols)

add_subdirectory(utils)
add_subdirectory(tools)