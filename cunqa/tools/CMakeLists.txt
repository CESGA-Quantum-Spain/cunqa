pybind11_add_module(probs_helpers probs_helpers.cpp)
install(TARGETS probs_helpers DESTINATION cunqa/tools)

file(GLOB TOOLS_PY "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
install(FILES ${TOOLS_PY} DESTINATION cunqa/tools)