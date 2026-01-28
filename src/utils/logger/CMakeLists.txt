set(CPPLOGGER_GLOBAL_LEVEL SPDLOG_LEVEL_DEBUG)

add_library(logger_qpu logger_qpu.cpp)
target_link_libraries(logger_qpu PUBLIC spdlog::spdlog)
target_compile_definitions(logger_qpu PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)

add_library(logger_client logger_client.cpp)
target_link_libraries(logger_client PUBLIC spdlog::spdlog)
set_target_properties(logger_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(logger_client PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)

add_library(logger_executor logger_executor.cpp)
target_link_libraries(logger_executor PUBLIC spdlog::spdlog)
set_target_properties(logger_executor PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(logger_executor PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)