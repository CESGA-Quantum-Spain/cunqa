cmake_minimum_required(VERSION 3.21)

project(CUNQA VERSION 0.0.1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Adding C++20 standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "C++ version ${CXX_STANDARD} configured.")
message(STATUS "${CMAKE_VERSION}")
if(${CMAKE_VERSION} VERSION_EQUAL "3.27.6")
    cmake_policy(SET CMP0144 OLD)
endif()
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

# Set INSTALL paths
message(STATUS "Install location will be: $ENV{HOME}")
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib64")
set(CMAKE_INSTALL_BINDIR "bin")

# Load pybind11 depending on the environment
if("$ENV{LMOD_SYSTEM_NAME}" STREQUAL "QMIO")
    set(PYBIND_PATH "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11")
    set(pybind11_DIR "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")
else("$ENV{LMOD_SYSTEM_NAME}" STREQUAL "FT3")
    set(PYBIND_PATH "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11")
    set(pybind11_DIR "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")    
endif()

find_package(Python 3.9.9 EXACT COMPONENTS Interpreter Development)
set(PYBIND11_PYTHON_VERSION 3.9 CACHE STRING "")

find_package(pybind11 2.12 REQUIRED)
find_package(nlohmann_json 3.7.3 REQUIRED)
find_package(Threads REQUIRED)
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/logger" "${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${Python_INCLUDE_DIRS}" "${MPI_INCLUDE_PATH}")

# Selecting communication library for classical communications between QPUs
option(USE_MPI_BTW_QPU "Using the MPI library for communication between QPUs" OFF)
option(USE_ZMQ_BTW_QPU "Using the ZMQ library for communication between QPUs" OFF)

# Set default if both are OFF
if(NOT USE_MPI_BTW_QPU AND NOT USE_ZMQ_BTW_QPU)
    set(USE_ZMQ_BTW_QPU ON CACHE BOOL "Using ZMQ by default" FORCE)
endif()

if(USE_MPI_BTW_QPU)
    add_compile_definitions(USE_MPI_BTW_QPU)
    message(STATUS "COMMUNICATIONS BETWEEN QPUS WITH MPI.")
elseif(USE_ZMQ_BTW_QPU)
    add_compile_definitions(USE_ZMQ_BTW_QPU)
    message(STATUS "COMMUNICATIONS BETWEEN QPUS WITH ZMQ.")
else()
    # We set the default communication library to ZMQ
    add_compile_definitions(USE_ZMQ_BTW_QPU)
    message(STATUS "COMMUNICATIONS BETWEEN QPUS WITH ZMQ BY DEFAULT.")
endif()

add_subdirectory(src)
add_subdirectory(cunqa)
add_subdirectory(examples)

# Enable testing
enable_testing()
add_subdirectory(cpptests)
