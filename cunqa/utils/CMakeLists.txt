configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.py"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/constants.py" DESTINATION cunqa/utils)

file(GLOB UTILS_PY "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${UTILS_PY} DESTINATION cunqa/utils)