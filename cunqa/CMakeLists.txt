
pybind11_add_module(qclient bindings.cpp)

target_link_libraries(qclient PRIVATE client) #${GNUTLS_LIB} ${SODIUM_LIB} bsd)

install(TARGETS qclient DESTINATION cunqa)

file(GLOB PYTHON_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${PYTHON_SCRIPTS} DESTINATION cunqa)