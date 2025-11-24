add_library(logger_qpu logger_qpu.cpp)
target_link_libraries(logger_qpu PUBLIC spdlog::spdlog)


add_library(logger_client logger_client.cpp)
target_link_libraries(logger_client PUBLIC spdlog::spdlog)
set_target_properties(logger_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(logger_executor logger_executor.cpp)
target_link_libraries(logger_executor PUBLIC spdlog::spdlog)
set_target_properties(logger_executor PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)