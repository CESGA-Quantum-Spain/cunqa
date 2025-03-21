cmake_minimum_required(VERSION 3.16)

project(CUNQA VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_SYSTEM_PREFIX_PATH "")
set(CMAKE_INSTALL_PREFIX "")
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Adding C++20 standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

message(STATUS "C++ version ${CXX_STANDARD} configured.")
message(STATUS "${CMAKE_VERSION}")
if(${CMAKE_VERSION} VERSION_EQUAL "3.27.6")
    cmake_policy(SET CMP0144 OLD)
endif()
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

if(NOT DEFINED ENV{INSTALL_PATH})
    message(FATAL_ERROR "Install location is not set: define INSTALL_PATH environment variables")
else()
    message(STATUS "Install location will be: (INSTALL_PATH) $ENV{INSTALL_PATH}")
    set(CMAKE_INSTALL_PREFIX "$ENV{INSTALL_PATH}")
endif()

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib64")
set(CMAKE_INSTALL_BINDIR "bin")

find_package(Python 3.9.9 EXACT COMPONENTS Interpreter Development)

#TODO: Load pybind11 depending on the environment

if(DEFINED PYBIND_PATH)
    set(pybind11_DIR "${PYBIND_PATH}/share/cmake/pybind11")
else(NOT DEFINED PYBIND_PATH)
    string(FIND "$ENV{LOADEDMODULES}" "pybind11" module_pos)
    if(module_pos GREATER -1)
        #list(APPEND CMAKE_PREFIX_PATH "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib/python3.9/site-packages/pybind11/share/cmake/pybind11")
        set(PYBIND_PATH "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11")
        set(pybind11_DIR "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")
        message(STATUS "Loading the default path for pybind11...")
    else()
        message(WARNING
        "Yo do not have any module of pybind11 loaded. This will fail " 
        "unless you have a conda environment with the correct pybind11 "
        "versions. \n"
        "This is NOT recomended." 
        )
    endif()
endif()

set(PYBIND11_PYTHON_VERSION 3.9 CACHE STRING "")
find_package(pybind11 2.12 REQUIRED)

add_subdirectory(src)
add_subdirectory(cunqa)