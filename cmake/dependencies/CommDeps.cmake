# =====================================================================
#  ZeroMQ stack: libzmq + cppzmq + pyzmq
# =====================================================================

# =====================================================================
#  libzmq
# =====================================================================
CPMAddPackage(
  NAME libzmq
  VERSION 4.3.5
  GITHUB_REPOSITORY zeromq/libzmq
  GIT_TAG v4.3.5
  OPTIONS
    "WITH_DOCS OFF"
    "WITH_PERF_TOOL OFF"
    "ZMQ_BUILD_TESTS OFF"
    "WITH_GNUTLS OFF"
    "BUILD_SHARED ON"
    "BUILD_STATIC OFF"
)

if(libzmq_ADDED)
  message(STATUS "Fetched libzmq with CPM")
else()
  message(STATUS "Using existing libzmq package")
endif()

if(TARGET libzmq)
  message(STATUS "libzmq target exists")
elseif(TARGET libzmq-static)
  message(STATUS "libzmq-static target exists")
elseif(TARGET libzmq-shared)
  message(STATUS "libzmq-shared target exists")
else()
  message(WARNING "No known libzmq target found")
endif()


# =====================================================================
#  cppzmq
# =====================================================================
CPMAddPackage(
  NAME cppzmq
  VERSION 4.9.0
  GITHUB_REPOSITORY zeromq/cppzmq
  GIT_TAG v4.9.0
  OPTIONS
    "CPPZMQ_BUILD_TESTS OFF"
)

if(cppzmq_ADDED)
  message(STATUS "Fetched cppzmq with CPM")
else()
  message(STATUS "Using existing cppzmq package")
endif()

if(TARGET cppzmq)
  message(STATUS "cppzmq target exists")
elseif(TARGET cppzmq-static)
  message(STATUS "cppzmq-static target exists")
else()
  message(WARNING "No known cppzmq target found")
endif()


# =====================================================================
#  pyzmq
# =====================================================================
# pyzmq needs to find the ZeroMQ built above.
# Depending on libzmq's generated config location, one of these may be useful.
if(libzmq_BINARY_DIR)
  set(ZeroMQ_DIR "${libzmq_BINARY_DIR}" CACHE PATH "Path to ZeroMQConfig.cmake" FORCE)
endif()

if(DEFINED ZMQ_PREFIX)
  set(LDFLAGS "-Wl,-rpath,${ZMQ_PREFIX}")
endif()

CPMAddPackage(
  NAME pyzmq
  GIT_REPOSITORY git@github.com:zeromq/pyzmq.git
  GIT_TAG v27.1.0
)

if(pyzmq_ADDED)
  message(STATUS "Fetched pyzmq with CPM")
else()
  message(STATUS "Using existing pyzmq package")
endif()

if(DEFINED pyzmq_SOURCE_DIR)
  install(
    DIRECTORY "${pyzmq_SOURCE_DIR}/zmq/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/zmq"
  )
endif()

if(TARGET _zmq)
  install(
    TARGETS _zmq
    DESTINATION "${CMAKE_INSTALL_PREFIX}/zmq/backend/cython"
  )
else()
  message(WARNING "Target _zmq was not created by pyzmq")
endif()