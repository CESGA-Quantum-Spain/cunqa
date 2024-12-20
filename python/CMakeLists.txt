
pybind11_add_module(qclient bindings.cpp)

target_include_directories(qclient PRIVATE "${CMAKE_SOURCE_DIR}/src"
                                          "${PROJECT_BINARY_DIR}/src/comm")

pybind11_add_module(from_json_to_bin bindings.cpp)

target_include_directories(from_json_to_bin PRIVATE "${CMAKE_SOURCE_DIR}/src"
                                          "${PROJECT_BINARY_DIR}/src/comm")

install(TARGETS qclient from_json_to_bin DESTINATION python)

