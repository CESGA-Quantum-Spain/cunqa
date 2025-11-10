# save your flags
set(_old_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

include(FetchContent)

set(JSON_VERSION
    3.11.3
    CACHE STRING "nlohmann_json version")
set(JSON_URL https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.tar.xz)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  FetchContent_Declare(nlohmann_json URL ${JSON_URL} FIND_PACKAGE_ARGS ${JSON_VERSION})
  FetchContent_MakeAvailable(nlohmann_json)
else()
  find_package(nlohmann_json ${JSON_VERSION} QUIET)
  if(NOT nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json")
    FetchContent_Declare(nlohmann_json URL ${JSON_URL})
    FetchContent_MakeAvailable(nlohmann_json)
  endif()
endif()

set(SPDLOG_SYSTEM_INCLUDES OFF   CACHE BOOL "Disable libsodium"      FORCE)
set(SPDLOG_BUILD_SHARED ON       CACHE BOOL "Disable CPACK"          FORCE)
set(SPDLOG_BUILD_PIC ON          CACHE BOOL "Disable performance tool" FORCE)
FetchContent_Declare(
    spdlog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
)
FetchContent_MakeAvailable(spdlog)
install(TARGETS spdlog DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64")

FetchContent_Declare(
  mqt-ddsim
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mqt-ddsim
)
FetchContent_MakeAvailable(mqt-ddsim)


# ZMQ options to build
set(WITH_DOCS OFF               CACHE BOOL "Disable libsodium"      FORCE)
set(ENABLE_CPACK OFF            CACHE BOOL "Disable CPACK"          FORCE)
set(WITH_PERF_TOOL OFF          CACHE BOOL "Disable performance tool" FORCE)
set(ZMQ_BUILD_TESTS OFF         CACHE BOOL "Disable tests"          FORCE)
set(ENABLE_DRAFTS ON            CACHE BOOL "Enable drafts"          FORCE)
set(BUILD_SHARED ON             CACHE BOOL "Build shared library"   FORCE)
set(BUILD_STATIC OFF            CACHE BOOL "Build static library"   FORCE)

FetchContent_Declare(
    libzmq
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libzmq
)
FetchContent_MakeAvailable(libzmq)


FetchContent_Declare(
  cunqasimulator
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cunqasimulator
)
FetchContent_MakeAvailable(cunqasimulator)

# TODO: Change this to non global
message(STATUS "ZMQ BINARY DIR: ${libzmq_BINARY_DIR}")
set(ZMQ_BINARY_DIR "${libzmq_BINARY_DIR}"
    CACHE PATH "ZeroMQ binary directory from FetchContent")
set(ZMQ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libzmq/include"
    CACHE PATH "ZeroMQ include directory")

set(CMAKE_CXX_FLAGS "${_old_CXX_FLAGS}")