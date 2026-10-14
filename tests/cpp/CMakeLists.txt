# =====================================================================
#  C++ unit tests (configure with -DCUNQA_BUILD_TESTS=ON, run with ctest)
#  GoogleTest is fetched in cmake/dependencies/TestDeps.cmake.
# =====================================================================
include(GoogleTest)

add_executable(cunqa_cpp_tests
  test_timing.cpp
  test_nc_executor.cpp
  test_random_delay.cpp
  test_channels.cpp
  test_comm_model.cpp
  test_qc_engine.cpp
)

target_include_directories(cunqa_cpp_tests
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(cunqa_cpp_tests
  PRIVATE
    GTest::gtest_main
    timing
    noise
    nc_executor
    comm_model
    qc_engine
    AER_simulator_adapter
    cunqa_simulator_adapter
    munich_simulator_adapter
    qsim_simulator_adapter
    quest_simulator_adapter
    qulacs_simulator_adapter
)

# Every library logs through a global `logger` defined by one of the logger libraries. The
# executor one, which qc_engine links, is linked whole so that it is the one used: the QPU
# one, which the simulators link, needs SLURM variables.
set_property(TARGET cunqa_cpp_tests PROPERTY LINK_LIBRARY_OVERRIDE_logger_executor WHOLE_ARCHIVE)

gtest_discover_tests(cunqa_cpp_tests DISCOVERY_MODE PRE_TEST)
