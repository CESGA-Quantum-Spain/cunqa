add_library(logger_qpu logger_qpu.cpp)
target_link_libraries(logger_qpu PUBLIC spdlog::spdlog)
target_include_directories(logger_qpu PUBLIC "${CMAKE_SOURCE_DIR}/src/third-party/spdlog/include")

add_library(logger_client logger_client.cpp)
target_link_libraries(logger_client PUBLIC spdlog::spdlog)
target_include_directories(logger_client PUBLIC "${CMAKE_SOURCE_DIR}/src/third-party/spdlog/include")
target_compile_options(logger_client PRIVATE -fPIC)

add_library(logger_executor logger_executor.cpp)
target_link_libraries(logger_executor PUBLIC spdlog::spdlog)
target_include_directories(logger_executor PUBLIC "${CMAKE_SOURCE_DIR}/src/third-party/spdlog/include")
target_compile_options(logger_executor PRIVATE -fPIC)