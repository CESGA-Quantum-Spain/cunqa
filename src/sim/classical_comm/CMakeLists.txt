
add_library(cc_executor cc_executor.cpp)
target_link_directories(cc_executor PUBLIC json)
target_include_directories(cc_executor
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src/quantum_task"
        "${CMAKE_SOURCE_DIR}/src/sim"
        "${CMAKE_SOURCE_DIR}/src/comm"
    PRIVATE
        "${CURRENT_SOURCE_DIR}"
)