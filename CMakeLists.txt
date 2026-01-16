cmake_minimum_required(VERSION 3.21)

project(CUNQA VERSION 1.2.0 LANGUAGES CXX)

message(STATUS "CMAKE BUILD TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE CONFIGURATION TYPE: ${CMAKE_CONFIGURATION_TYPES}")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Adding C++20 standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg") # Dani, for profiling with gprof
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
  set(pybind11_DIR "/opt/cesga/2020/software/Core/pybind11/2.13.1-python-3.9.9/lib/python3.9/site-packages/pybind11/share/cmake/pybind11/")
  include_directories("/opt/cesga/2020/software/Core/python/3.9.9/include/python3.9/")
  find_package(Python 3.9.9 EXACT COMPONENTS Interpreter Development)
  find_package(pybind11 2.13.1 REQUIRED)
  set(PYBIND11_PYTHON_VERSION 2.13 CACHE STRING "")
  
else() # Other clusters
  find_package(Python 3.9 COMPONENTS Interpreter Development)
  find_package(pybind11 2.7 REQUIRED)
endif()

find_package(Threads REQUIRED)
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED COMPONENTS CXX)
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
    message(STATUS "Found package ${PACKAGE_NAME}: ${PACKAGE_NAME}_DIR = ${${PACKAGE_NAME}_DIR}")
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

# -- pyzmq -----------------------------------------------------------
set(ZMQ_PREFIX "${libzmq_BINARY_DIR}" CACHE PATH "ZMQ_PREFIX" FORCE)
set(LDFLAGS "-Wl,-rpath,${ZMQ_PREFIX}/lib")

FetchContent_Declare(
    pyzmq
    GIT_REPOSITORY "git@github.com:zeromq/pyzmq.git"
    GIT_TAG        "main"
)
FetchContent_MakeAvailable(pyzmq)
install(DIRECTORY  "${pyzmq_SOURCE_DIR}/zmq/" DESTINATION "${CMAKE_INSTALL_LIBDIR}/zmq")
install(TARGETS "_zmq" DESTINATION "${CMAKE_INSTALL_LIBDIR}/zmq/backend/cython")

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
if (DEFINED AER_GPU)
  message(STATUS "Compiling with AER_GPU")
  
  #set(CMAKE_CUDA_COMPILER "/path/to/bin/nvcc")
  #set(CMAKE_CUDA_COMPILER_ID "NVIDIA")
  #set(CMAKE_CUDA_COMPILER_VERSION "12.8.0")
  #set(CMAKE_CUDA_COMPILER_WORKS TRUE)
  
  # Architecture           GPUs (examples)        SM
  # -------------------------------------------------
  # Volta                  V100                   70
  # Turing                 T4, RTX 2080           75
  # Ampere                 A100                   80
  # Ampere (GA10x)         RTX 30xx               86
  # Ada Lovelace           RTX 40xx               89
  # Hopper                 H100                   90
  
  set(CUDA_ARCH 75)
  set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH})
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_library(PMIX_LIB NAMES pmix REQUIRED)

  find_package(CUDAToolkit REQUIRED)
  target_compile_definitions(aer_headers INTERFACE
      AER_THRUST_GPU
      AER_THRUST_CUDA
      AER_THRUST_SUPPORTED=TRUE
      THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA
  )
  target_compile_options(aer_headers INTERFACE
      $<$<COMPILE_LANGUAGE:CUDA>:
          --expt-extended-lambda
          --use_fast_math
          -use_fast_math 
          --expt-extended-lambda
          -Xcompiler=-fPIC 
          -Xcompiler=-Wno-psabi 
          -Xcompiler=-Wno-ignored-attributes 
          -Xcompiler=-fopenmp
      >
  )
  target_link_libraries(aer_headers INTERFACE Python::Python Threads::Threads ${PMIX_LIB} CUDA::cudart)
endif()

# =====================================================================
#  Maestro and its dependencies
# =====================================================================
find_package(Eigen3 5.0 QUIET)
if(NOT Eigen3_FOUND)
  message(STATUS "Package Eigen3 not found, fetching from https://gitlab.com/libeigen/eigen.git")
  FetchContent_Declare(
      MaestroEigen
      GIT_REPOSITORY "https://gitlab.com/libeigen/eigen.git"
      GIT_TAG        "bc3b39870ecb690a623a3f49149a358b95c5781d"
  )
  FetchContent_GetProperties(MaestroEigen)
  if(NOT maestroeigen_POPULATED)
      FetchContent_Populate(MaestroEigen)
  endif()
  #FetchContent_MakeAvailable(MaestroEigen)
  set(EIGEN5_INCLUDE_DIR "${maestroeigen_SOURCE_DIR}") 
else()
  message(STATUS "Eigen 5.0 found")
  set(EIGEN5_INCLUDE_DIR "${EIGEN5_INCLUDE_PATH}") 
endif()

get_target_property(
    JSON_INCLUDE_PATH 
    nlohmann_json::nlohmann_json 
    INTERFACE_INCLUDE_DIRECTORIES
)
set(JSON_INCLUDE_DIR "${JSON_INCLUDE_PATH}") 

# -- QCSim ------------------------------------------------------------
FetchContent_Declare(
    qcsim
    GIT_REPOSITORY "git@github.com:aromanro/QCSim.git"
    GIT_TAG "5a458c8b4bddfbbe445de09376b366a97d474266"
)
if(NOT qcsim_POPULATED)
    FetchContent_Populate(qcsim) # Emply of populate to avoid the compilation of wrappers
endif()

set(QCSIM_INCLUDE_DIR "${qcsim_SOURCE_DIR}/QCSim")

# -- AER - Qiskit simulator headers (fork for Maestro) ----------------
FetchContent_Declare(
    maestro-aer
    GIT_REPOSITORY "git@github.com:InvictusWingsSRL/qiskit-aer.git"
    GIT_TAG "96e82fdd1c8c1b17d749863ee5095262372e0d7e"
)
if(NOT maestro-aer_POPULATED)
    FetchContent_Populate(maestro-aer) # Emply of populate to avoid the compilation of wrappers
endif()

set(AER_INCLUDE_DIR "${maestro-aer_SOURCE_DIR}/src")

# -- Maestro ----------------------------------------------------------

FetchContent_Declare(
    maestro
    GIT_REPOSITORY "git@github.com:QoroQuantum/maestro.git"
    GIT_TAG "7ba339fc5595fd18d2fcb0612a6570c3c5e1fd90"
)
FetchContent_MakeAvailable(maestro)

target_include_directories(maestro PUBLIC "${maestro_SOURCE_DIR}")
install(TARGETS maestro DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# =====================================================================
#  Qulacs - Fujitsu simulator (Fork with ECR gate implemented)
# =====================================================================
FetchContent_Declare(
    qulacs
    GIT_REPOSITORY "git@github.com:usendon/usendon_qulacs.git"
    GIT_TAG "228f617b9e31007b54488f467c031ebbf07efcdb"
)

set(USE_PYTHON OFF CACHE BOOL "" FORCE)
set(BUILD_QULACS_WITH_MPI OFF)
if(BUILD_QULACS_WITH_MPI)
  #set(CXX_COMPILER "mpic++" CACHE STRING "" FORCE) # Warning!
  set(USE_MPI ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(qulacs)


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
