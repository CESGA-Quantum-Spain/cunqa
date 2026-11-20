cmake_minimum_required(VERSION 3.24)
message(STATUS "${CMAKE_VERSION}")
message(STATUS "CMAKE BUILD TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE CONFIGURATION TYPE: ${CMAKE_CONFIGURATION_TYPES}")

if(${CMAKE_VERSION} VERSION_EQUAL "3.27.6")
    cmake_policy(SET CMP0144 OLD)
    cmake_policy(SET CMP0135 NEW)
endif()

# Version: nearest git tag when building from a clone; VERSION.txt otherwise
# (release tarballs, EasyBuild). The sync-version workflow keeps VERSION.txt
# in step with new tags.
set(PROJECT_VERSION "")
find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_TAG MATCHES "^v?([0-9]+\\.[0-9]+\\.[0-9]+)$")
        set(PROJECT_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()
if(NOT PROJECT_VERSION)
    file(READ "${CMAKE_SOURCE_DIR}/VERSION.txt" PROJECT_VERSION)
    string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
endif()
project(CUNQA VERSION ${PROJECT_VERSION} LANGUAGES CXX C)
message(STATUS "Building CUNQA ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions)
include(Platforms)
include(Install)
include(get_cpm)

cunqa_setup_project_defaults()
cunqa_setup_install_paths()
cunqa_detect_platform()
cunqa_configure_python_and_pybind11()

if(USE_GPU)
  enable_language(CUDA)
endif()

include(dependencies/CoreDeps)
include(dependencies/CommDeps)
include(dependencies/SimulatorDeps)

add_subdirectory(src)
add_subdirectory(cunqa)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
