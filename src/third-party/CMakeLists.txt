include(FetchContent)

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