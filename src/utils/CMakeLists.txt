configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.hpp"
    @ONLY
)

add_library(json json.cpp)
target_link_libraries(json PRIVATE Threads::Threads
                           PUBLIC nlohmann_json::nlohmann_json)
set_target_properties(json PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(logger)