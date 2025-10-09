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
endif()
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

# Set INSTALL paths
# TODO: I put this if 
if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH "Install prefix" FORCE)
endif()

message(STATUS "Install location will be: ${CMAKE_INSTALL_PREFIX}")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib64")
set(CMAKE_INSTALL_BINDIR "bin")

# Load pybind11 depending on the environment
set(PYBIND_PATH "$ENV{EBROOTPYBIND11}/lib64/python3.9/site-packages/pybind11")
set(pybind11_DIR "$ENV{EBROOTPYBIND11}/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")

# System name to identify cluster
set(SYSTEM_NAME $ENV{LMOD_SYSTEM_NAME})
if(NOT SYSTEM_NAME OR NOT (SYSTEM_NAME STREQUAL "QMIO" OR SYSTEM_NAME STREQUAL "FT3"))
    set(SYSTEM_NAME "MY_CLUSTER")
endif()
message(STATUS "SYSTEM NAME: ${SYSTEM_NAME}")
add_compile_definitions(SYSTEM_NAME="${SYSTEM_NAME}")


#if("$ENV{LMOD_SYSTEM_NAME}" STREQUAL "QMIO")
#    set(PYBIND_PATH "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11")
#    set(pybind11_DIR "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.12.0-python-3.9.9/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")
#else("$ENV{LMOD_SYSTEM_NAME}" STREQUAL "FT3")
#    set(PYBIND_PATH "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11")
#    set(pybind11_DIR "/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11/share/cmake/pybind11")    
#endif()

if(${SYSTEM_NAME} STREQUAL "QMIO")
  set(pybind11_DIR "/opt/cesga/qmio/hpc/software/Compiler/gcccore/12.3.0/pybind11/2.13.6-python-3.11.9/lib64/python3.11/site-packages/pybind11/share/cmake/pybind11/")
  find_package(Python 3.11.9 EXACT COMPONENTS Interpreter Development)
  find_package(pybind11 2.13 REQUIRED)
  find_package(nlohmann_json 3.7.3 REQUIRED)
  set(PYBIND11_PYTHON_VERSION 3.11 CACHE STRING "")
elseif(${SYSTEM_NAME} STREQUAL "FT3")
  set(pybind11_DIR "/opt/cesga/2022/software/Core/pybind11/2.12.0-python-3.10.8/lib64/python3.10/site-packages/pybind11/share/cmake/pybind11/")
  find_package(Python 3.10.8 EXACT COMPONENTS Interpreter Development)
  find_package(pybind11 2.12 REQUIRED)
  find_package(nlohmann_json 3.7.3 REQUIRED)
  set(PYBIND11_PYTHON_VERSION 3.10 CACHE STRING "")
elseif(${SYSTEM_NAME} STREQUAL "MY_CLUSTER") # Other clusters
  message(FATAL_ERROR "Remains to set Python, pybind and nlhomann libraries")
endif()

find_package(Threads REQUIRED)
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/logger" "${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${Python_INCLUDE_DIRS}" "${MPI_INCLUDE_PATH}")

# To link OpenMP with Aer:
add_library(AerCPP INTERFACE)
target_include_directories(AerCPP INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/third-party/aer-cpp/")
if(OpenMP_FOUND)
  target_link_libraries(AerCPP INTERFACE OpenMP::OpenMP_CXX)
  message(STATUS "Linked OpenMP to aer-cpp")
endif()
if(OpenMP_FOUND)
  target_link_libraries(AerCPP INTERFACE pybind11::headers)
  message(STATUS "Linked pybind aer-cpp")
endif()

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
