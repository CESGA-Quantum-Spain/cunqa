# =====================================================================
#  GoogleTest - C++ unit tests (only with CUNQA_BUILD_TESTS)
#  Included before SimulatorDeps: Qulacs loads its own copy of FetchContent,
#  which breaks every package fetched with CPM after it.
# =====================================================================
CPMAddPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG v1.15.2
  VERSION 1.15.2
  OPTIONS
    "INSTALL_GTEST OFF"
    "BUILD_GMOCK OFF"
)

if(googletest_ADDED)
  message(STATUS "Fetched googletest with CPM")
else()
  message(STATUS "Using existing googletest package")
endif()
