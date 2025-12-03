configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qmio_linker.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qmio_linker.py"
    @ONLY
)

file(GLOB REAL_QPUS_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${REAL_QPUS_SCRIPTS} DESTINATION cunqa/real_qpus)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qmio_linker.py" DESTINATION cunqa/real_qpus)