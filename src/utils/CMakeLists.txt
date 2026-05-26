configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/constants.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/constants.hpp"
    @ONLY
)

add_library(constants INTERFACE)
target_sources(constants
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS 
      "${CMAKE_BINARY_DIR}/src"
      "${CMAKE_SOURCE_DIR}/src"
    FILES
      "${CMAKE_BINARY_DIR}/src/utils/constants.hpp"
      "${CMAKE_SOURCE_DIR}/src/utils/helpers/environment.hpp"
)

add_library(json)
target_sources(json
  PRIVATE
    json.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/src"
    FILES
      "${CMAKE_SOURCE_DIR}/src/utils/json.hpp"
)
target_link_libraries(json 
    PUBLIC 
        nlohmann_json::nlohmann_json
    PRIVATE 
        Threads::Threads
)
target_compile_options(json PRIVATE -fPIC)
add_subdirectory(logger)