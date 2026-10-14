# =====================================================================
#  C++ unit tests (configure with -DCUNQA_BUILD_TESTS=ON, run with ctest)
#  GoogleTest is fetched in cmake/dependencies/TestDeps.cmake.
# =====================================================================
include(GoogleTest)

add_executable(cunqa_cpp_tests
  test_timing.cpp
  test_nc_executor.cpp
)

target_include_directories(cunqa_cpp_tests
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Every library logs through a global `logger` defined by one of the logger libraries. The
# executor one is linked whole so that it is the one used: the QPU one needs SLURM variables.
target_link_libraries(cunqa_cpp_tests
  PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,logger_executor>"
    GTest::gtest_main
    timing
    nc_executor
    AER_simulator_adapter
)

gtest_discover_tests(cunqa_cpp_tests DISCOVERY_MODE PRE_TEST)
