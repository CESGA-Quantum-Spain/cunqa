add_subdirectory(logger)

add_library(json json.cpp)
target_link_libraries(json PRIVATE Threads::Threads logger_qpu
                           PUBLIC nlohmann_json::nlohmann_json)