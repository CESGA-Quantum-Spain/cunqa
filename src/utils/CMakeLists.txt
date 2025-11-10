add_subdirectory(logger)

add_library(json json.cpp)
target_link_libraries(json PRIVATE nlohmann_json::nlohmann_json logger_qpu Threads::Threads)