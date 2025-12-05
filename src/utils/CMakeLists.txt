configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.hpp"
    @ONLY
)

add_library(json json.cpp)
target_link_libraries(json PRIVATE Threads::Threads
                           PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(json PRIVATE -fPIC)
add_subdirectory(logger)