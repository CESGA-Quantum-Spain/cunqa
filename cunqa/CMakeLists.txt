add_subdirectory(circuit)
add_subdirectory(real_qpus)

set(LIB_NAME "qclient")

pybind11_add_module(${LIB_NAME} bindings.cpp)

target_include_directories(${LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/utils")
target_link_libraries(${LIB_NAME} PRIVATE client json)

install(TARGETS ${LIB_NAME} DESTINATION cunqa)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.py"
    @ONLY
)

file(GLOB PYTHON_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/constants.py" DESTINATION cunqa)
