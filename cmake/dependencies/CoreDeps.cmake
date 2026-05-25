find_package(Threads REQUIRED)
find_package(MPI REQUIRED)
find_package(Boost REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(OpenMP REQUIRED COMPONENTS CXX)

set(_old_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# =====================================================================
#  NLOHMANN JSON - json management library
# =====================================================================
CPMAddPackage(
  NAME nlohmann_json
  VERSION 3.12.0
  GITHUB_REPOSITORY nlohmann/json
  GIT_TAG v3.12.0
  OPTIONS
    "JSON_BuildTests OFF"
)

if(nlohmann_json_ADDED)
  message(STATUS "Fetched nlohmann_json with CPM")
else()
  message(STATUS "Using existing nlohmann_json package")
endif()

# =====================================================================
#  Eigen
# =====================================================================
# Prevent finding Eigen from other builds
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)

CPMAddPackage(
  NAME Eigen3
  VERSION 5.0.0
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 5.0.0
  GIT_SHALLOW OFF
  OPTIONS
    "EIGEN_BUILD_BLAS OFF"
    "EIGEN_BUILD_LAPACK OFF"
    "BUILD_TESTING OFF"
)

if(Eigen3_ADDED)
  message(STATUS "Fetched Eigen3 with CPM")
else()
  message(STATUS "Using existing Eigen3 package")
endif()


# =====================================================================
#  SPDLOG - logging backend
# =====================================================================
CPMAddPackage(
  NAME spdlog
  VERSION 1.16.0
  GITHUB_REPOSITORY gabime/spdlog
  GIT_TAG v1.16.0
  OPTIONS
    "SPDLOG_SYSTEM_INCLUDES OFF"
    "SPDLOG_BUILD_SHARED ON"
    "SPDLOG_BUILD_PIC ON"
    "SPDLOG_BUILD_TESTS OFF"
    "SPDLOG_BUILD_EXAMPLE OFF"
)

if(spdlog_ADDED)
  message(STATUS "Fetched spdlog with CPM")
else()
  message(STATUS "Using existing spdlog package")
endif()

install(
  TARGETS spdlog
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)


# =====================================================================
#  ARGPARSE - command line parsing
# =====================================================================
CPMAddPackage(
  NAME argparse
  GIT_REPOSITORY https://github.com/morrisfranken/argparse.git
  GIT_TAG b1479bf9f2f44010ad79efff00bb9bf8ec56dbab
)

if(argparse_ADDED)
  message(STATUS "Fetched argparse with CPM")
else()
  message(STATUS "Using existing argparse package")
endif()

set(CMAKE_CXX_FLAGS "${_old_CXX_FLAGS}")