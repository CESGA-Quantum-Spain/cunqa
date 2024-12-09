cmake_minimum_required(VERSION 3.16)

project(API-Simulator VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_SYSTEM_PREFIX_PATH "")
set(CMAKE_INSTALL_PREFIX "")

# Adding C++17 standard as required
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_17)
message(STATUS "C++ version ${CXX_STANDARD} configured.")

cmake_policy(SET CMP0144 OLD)
set(CMAKE_CXX_COMPILER "/mnt/netapp1/Optcesga_FT2_RHEL7/2022/gentoo/31032022/usr/bin/g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/mnt/netapp1/Optcesga_FT2_RHEL7/2022/gentoo/31032022/usr/lib/gcc/x86_64-pc-linux-gnu/11.2.1/include/g++-v11/")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

if(NOT DEFINED ENV{INSTALL_PATH})
    message(FATAL_ERROR "Install location is not set: define INSTALL_PATH environment variables")
else()
    message(STATUS "Install location will be: (INSTALL_PATH) $ENV{INSTALL_PATH}")
    set(CMAKE_INSTALL_PREFIX "$ENV{INSTALL_PATH}")
endif()

set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_INCLDIR "include")
set(CMAKE_INSTALL_BINDIR "bin")

find_package(Python 3.10 COMPONENTS Interpreter Development)

if(DEFINED PYBIND_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${PYBIND_PATH}")
else(NOT DEFINED PYBIND_PATH)
    string(FIND "$ENV{LOADEDMODULES}" "pybind11" module_pos)
    if(module_pos GREATER -1)
        list(APPEND CMAKE_PREFIX_PATH "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib/python3.9/site-packages/pybind11/share/cmake/pybind11")
    else()
        message(WARNING
        "Yo do not have any module of pybind11 loaded. This will fail " 
        "unless you have a conda environment with the correct pybind11 "
        "versions. \n"
        "This is NOT recomended." 
        )
    endif()
endif()

set(PYBIND11_PYTHON_VERSION 3.10 CACHE STRING "")
find_package(pybind11 2.12 REQUIRED)

add_subdirectory(src)
add_subdirectory(python)
