# Find Catch2 (assumes Catch2 is installed, e.g., via package manager)
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

# Add the test executable
add_executable(example_test example_test.cpp)
target_link_libraries(example_test PRIVATE Catch2::Catch2WithMain munich_adapters quantum_task)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(example_test)