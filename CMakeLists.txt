cmake_minimum_required(VERSION 3.21)

project(CUNQA VERSION 0.0.1 LANGUAGES CXX)

message(STATUS "CMAKE BUILD TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE CONFIGURATION TYPE: ${CMAKE_CONFIGURATION_TYPES}")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Adding C++20 standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
message(STATUS "C++ version ${CXX_STANDARD} configured.")
message(STATUS "${CMAKE_VERSION}")
if(${CMAKE_VERSION} VERSION_EQUAL "3.27.6")
    cmake_policy(SET CMP0144 OLD)
    cmake_policy(SET CMP0135 NEW)
endif()
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

# Check that $STORE exists
if(NOT DEFINED ENV{STORE})
    message(FATAL_ERROR "The STORE environment variable is not defined")
endif()
set(CUNQA_PATH "$ENV{STORE}/.cunqa")
set(QPUS_FILEPATH "$ENV{STORE}/.cunqa/qpus.json")
set(COMM_FILEPATH "$ENV{STORE}/.cunqa/communications.json")
include_directories("${CMAKE_BINARY_DIR}/src")


# Set INSTALL paths
# TODO: I put this if 
if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "Install prefix" FORCE)
endif()

message(STATUS "Install location will be: ${CMAKE_INSTALL_PREFIX}")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

# Fine tuning for CESGA machines (pybind11 cannot be found by default)
set(SYSTEM_NAME $ENV{LMOD_SYSTEM_NAME})
if(NOT SYSTEM_NAME OR NOT (SYSTEM_NAME STREQUAL "QMIO" OR SYSTEM_NAME STREQUAL "FT3"))
    set(SYSTEM_NAME "EXTERNAL_CLUSTER")
endif()

if(${SYSTEM_NAME} STREQUAL "QMIO")
  set(pybind11_DIR "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.13.6-python-3.11.9/lib64/python3.11/site-packages/pybind11/share/cmake/pybind11/")
  find_package(Python 3.11.9 EXACT COMPONENTS Interpreter Development)
  find_package(pybind11 2.13 REQUIRED)
  set(PYBIND11_PYTHON_VERSION 3.11 CACHE STRING "")
elseif(${SYSTEM_NAME} STREQUAL "FT3")
  set(pybind11_DIR "/opt/cesga/2022/software/Core/pybind11/2.12.0-python-3.10.8/lib64/python3.10/site-packages/pybind11/share/cmake/pybind11/")
  find_package(Python 3.10.8 EXACT COMPONENTS Interpreter Development)
  find_package(pybind11 2.12 REQUIRED)
  set(PYBIND11_PYTHON_VERSION 3.10 CACHE STRING "")
else() # Other clusters
  find_package(Python 3.9 COMPONENTS Interpreter Development)
  find_package(pybind11 2.7 REQUIRED)
endif()

find_package(Threads REQUIRED)
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

include(FetchContent)

function(find_or_fetch_package PACKAGE_NAME GIT_REPOSITORY VERSION TAG)
  find_package(${PACKAGE_NAME} ${VERSION} QUIET)
  if(NOT ${PACKAGE_NAME}_FOUND)
    message(STATUS "Package ${PACKAGE_NAME} not found, fetching from ${GIT_REPOSITORY}")
    FetchContent_Declare(
        ${PACKAGE_NAME}
        GIT_REPOSITORY ${GIT_REPOSITORY}
        GIT_TAG        ${TAG}
    )
    FetchContent_MakeAvailable(${PACKAGE_NAME})
  else()
    message(STATUS "Found package ${PACKAGE_NAME}")
  endif()
endfunction()

#  Compiler flags: save current and silence warnings for dependencies
set(_old_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# =====================================================================
#  NLOHMANN JSON - json management library
# =====================================================================
find_or_fetch_package(
  nlohmann_json
  "git@github.com:nlohmann/json.git"
  "3.11.3"
  "v3.11.3"
)

# =====================================================================
#  SPDLOG - logging backend
# =====================================================================
set(SPDLOG_SYSTEM_INCLUDES OFF   CACHE BOOL "Disable libsodium"          FORCE)
set(SPDLOG_BUILD_SHARED ON       CACHE BOOL "Build shared library"       FORCE)
set(SPDLOG_BUILD_PIC ON          CACHE BOOL "Build with position independent code" FORCE)

find_or_fetch_package(
  spdlog
  "git@github.com:gabime/spdlog.git"
  "1.16.0"
  "v1.16.0"
)
install(TARGETS spdlog DESTINATION "${CMAKE_INSTALL_LIBDIR}") # does this fails with FindPackage??

# =====================================================================
#  MQT-DDSIM - quantum circuit simulator
# =====================================================================
find_or_fetch_package(
  mqt-ddsim
  "git@github.com:munich-quantum-toolkit/ddsim.git"
  "1.24.0"
  "v1.24.0"
)

# =====================================================================
#  ZeroMQ stack (libzmq + cppzmq)
# =====================================================================

# -- libzmq -----------------------------------------------------------
set(WITH_DOCS OFF               CACHE BOOL "Disable docs"               FORCE)
set(ENABLE_CPACK OFF            CACHE BOOL "Disable CPACK"              FORCE)
set(WITH_PERF_TOOL OFF          CACHE BOOL "Disable performance tool"   FORCE)
set(ZMQ_BUILD_TESTS OFF         CACHE BOOL "Disable tests"              FORCE)
set(ENABLE_DRAFTS ON            CACHE BOOL "Enable drafts"              FORCE)
set(BUILD_SHARED ON             CACHE BOOL "Build shared library"       FORCE)
set(BUILD_STATIC OFF            CACHE BOOL "Build static library"       FORCE)

find_or_fetch_package(
  libzmq
  "git@github.com:zeromq/libzmq.git"
  "4.3.5"
  "v4.3.5"
)

# -- cppzmq -----------------------------------------------------------
set(CPPZMQ_BUILD_TESTS OFF      CACHE BOOL "Disable tests"              FORCE)

find_or_fetch_package(
  cppzmq
  "git@github.com:zeromq/cppzmq.git"
  "4.11.0"
  "v4.11.0"
)

# =====================================================================
#  CUNQA SIMULATOR - CESGA Quantum Spain
# =====================================================================
find_or_fetch_package(
  cunqasimulator
  "git@github.com:CESGA-Quantum-Spain/cunqasimulator.git"
  "0.1.1"
  "v0.1.1"
)

# =====================================================================
#  ARGPARSE - command line parsing
# =====================================================================
FetchContent_Declare(
    argparse
    GIT_REPOSITORY "git@github.com:morrisfranken/argparse.git"
    GIT_TAG "b1479bf9f2f44010ad79efff00bb9bf8ec56dbab"
)
FetchContent_MakeAvailable(argparse)

# =====================================================================
#  AER - Qiskit simulator headers (fork with version 0.17.2 minimal fix)
# =====================================================================
FetchContent_Declare(
    aer
    GIT_REPOSITORY "git@github.com:CESGA-Quantum-Spain/qiskit-aer_v0.17.2.git"
    GIT_TAG "717e77a5257c35ce49dfbb4e8ba8c23acbadfe14"
)
FetchContent_GetProperties(aer)
if(NOT aer_POPULATED)
    FetchContent_Populate(aer) # Emply of populate to avoid the compilation of wrappers
endif()

add_library(aer_headers INTERFACE)
target_include_directories(aer_headers INTERFACE "${aer_SOURCE_DIR}/src" "${Python_INCLUDE_DIRS}")
if(OpenMP_FOUND)
  target_link_libraries(aer_headers INTERFACE OpenMP::OpenMP_CXX)
  message(STATUS "Linked OpenMP to aer-cpp")
endif()
if(pybind11_FOUND)
  target_link_libraries(aer_headers INTERFACE pybind11::headers)
  message(STATUS "Linked pybind aer-cpp")
endif()

#  Restore original compiler flags
set(CMAKE_CXX_FLAGS "${_old_CXX_FLAGS}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/logger" "${CMAKE_CURRENT_SOURCE_DIR}/src")
#include_directories("${Python_INCLUDE_DIRS}" "${MPI_INCLUDE_PATH}")

add_subdirectory(src)
add_subdirectory(cunqa)
add_subdirectory(examples)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
