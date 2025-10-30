# save your flags
set(_old_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

include(FetchContent)

# SPDLOG options to build
set(SPDLOG_SYSTEM_INCLUDES OFF   CACHE BOOL "Disable libsodium"      FORCE)
set(SPDLOG_BUILD_SHARED ON       CACHE BOOL "Disable CPACK"          FORCE)
set(SPDLOG_BUILD_PIC ON          CACHE BOOL "Disable performance tool" FORCE)
FetchContent_Declare(
    spdlog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
)
FetchContent_MakeAvailable(spdlog)
install(TARGETS spdlog DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64")


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


# MQT-DDSIM options to build
FetchContent_Declare(
  mqt-ddsim
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mqt-ddsim
)
FetchContent_MakeAvailable(mqt-ddsim)


# CUNQASIMULATOR options to build
FetchContent_Declare(
  cunqasimulator
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cunqasimulator
)
FetchContent_MakeAvailable(cunqasimulator)


# MAESTRO options to build
find_package(Eigen3 5.0.0 REQUIRED)
if(TARGET Eigen3::Eigen)
    get_target_property(
        EIGEN_INCLUDE_PATH 
        Eigen3::Eigen 
        INTERFACE_INCLUDE_DIRECTORIES
    )

    set(EIGEN_INCLUDE_PATH "${EIGEN_INCLUDE_PATH}") 
    message(STATUS "Eigen include dir extracted from target: ${EIGEN_INCLUDE_PATH}")
else()
    message(FATAL_ERROR "Eigen was not found, or the target Eigen3::Eigen is missing.")
endif()

if(TARGET nlohmann_json::nlohmann_json)
    get_target_property(
        JSON_INCLUDE_PATH 
        nlohmann_json::nlohmann_json 
        INTERFACE_INCLUDE_DIRECTORIES
    )
    set(JSON_INCLUDE_PATH "${JSON_INCLUDE_PATH}") 

    message(STATUS "nlohmann/json include dir extracted from target: ${JSON_INCLUDE_PATH}")
else()
    message(FATAL_ERROR "nlohmann/json was found, but the modern target nlohmann_json::nlohmann_json is missing.")
endif()

set(QCSIM_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/QCSim/QCSim)
set(AER_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/maestro-qiskit-aer/src)

FetchContent_Declare(
    maestro
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/maestro-latest
)
FetchContent_MakeAvailable(maestro)
install(TARGETS maestro DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64")


# TODO: Change this to non global
message(STATUS "ZMQ BINARY DIR: ${libzmq_BINARY_DIR}")
set(ZMQ_BINARY_DIR "${libzmq_BINARY_DIR}"
    CACHE PATH "ZeroMQ binary directory from FetchContent")
set(ZMQ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libzmq/include"
    CACHE PATH "ZeroMQ include directory")

set(CMAKE_CXX_FLAGS "${_old_CXX_FLAGS}")

