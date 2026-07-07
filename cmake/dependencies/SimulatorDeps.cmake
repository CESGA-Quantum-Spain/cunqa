# =====================================================================
#  AER - Qiskit simulator headers
#  Fork with version 0.17.2 minimal fix and GPU setter
# =====================================================================
CPMAddPackage(
  NAME aer
  GIT_REPOSITORY git@github.com:CESGA-Quantum-Spain/qiskit-aer_v0.17.2.git
  GIT_TAG 8679dbe68458a43661222bc86e6e877d1084125b
  DOWNLOAD_ONLY YES
)

add_library(aer_headers INTERFACE)

target_include_directories(aer_headers
  INTERFACE
    "${aer_SOURCE_DIR}/src"
    "${Python_INCLUDE_DIRS}"
)

if(OpenMP_FOUND)
  target_link_libraries(aer_headers INTERFACE OpenMP::OpenMP_CXX)
  message(STATUS "Linked OpenMP to aer-cpp")
endif()

if(pybind11_FOUND)
  target_link_libraries(aer_headers INTERFACE pybind11::headers)
  message(STATUS "Linked pybind aer-cpp")
endif()

if(USE_GPU)
  message(STATUS "Compiling AER simulator with GPU")

  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_library(PMIX_LIB NAMES pmix REQUIRED)
  find_package(CUDAToolkit REQUIRED)

  target_compile_definitions(aer_headers
    INTERFACE
      AER_THRUST_GPU
      AER_THRUST_CUDA
      AER_THRUST_SUPPORTED=TRUE
      THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA
  )

  target_compile_options(aer_headers
    INTERFACE
      $<$<COMPILE_LANGUAGE:CUDA>:
        --expt-extended-lambda
        --use_fast_math
        -use_fast_math
        -Xcompiler=-fPIC
        -Xcompiler=-Wno-psabi
        -Xcompiler=-Wno-ignored-attributes
        -Xcompiler=-fopenmp
      >
  )

  target_link_libraries(aer_headers
    INTERFACE
      Python::Python
      Threads::Threads
      ${PMIX_LIB}
      CUDA::cudart
  )
endif()

# =====================================================================
#  MQT-DDSIM - quantum circuit simulator
# =====================================================================
set(_ORIG_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(_ORIG_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(_ORIG_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set(_ORIG_INCLUDE_DIRS "${CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE}")
set(_ORIG_COMPILE_DEFINITIONS "${CMAKE_CXX_FLAGS}")

set(BUILD_MQT_CORE_MLIR OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)

CPMAddPackage(
  NAME mqt-ddsim
  VERSION 2.2.0
  GIT_REPOSITORY git@github.com:munich-quantum-toolkit/ddsim.git
  GIT_TAG v2.2.0
  OPTIONS
    "BUILD_MQT_CORE_MLIR OFF"
)

if(mqt-ddsim_ADDED)
  message(STATUS "Fetched mqt-ddsim with CPM")
else()
  message(STATUS "Using existing mqt-ddsim package")
endif()

include_directories(${_ORIG_INCLUDE_DIRS})
add_definitions(${_ORIG_COMPILE_DEFINITIONS})


# ===================================================================================
#  QuEST - Quantum Exact Simulation Toolkit from the EPCC at University of Edinburgh
# ===================================================================================
CPMAddPackage(
  NAME quest
  GIT_REPOSITORY git@github.com:CESGA-Quantum-Spain/QuEST.git
  GIT_TAG main
)

# ===================================================================================
#  MaestroF
# ===================================================================================

# ---------------------------------------------------------------------
#  QCSim
# ---------------------------------------------------------------------
CPMAddPackage(
  NAME qcsim
  GIT_REPOSITORY git@github.com:aromanro/QCSim.git
  GIT_TAG 76db3f97974221306c9e2b7be4c4c9f6e7318d6e
  DOWNLOAD_ONLY YES
)

# ---------------------------------------------------------------------
#  AER - Qiskit simulator headers, fork for Maestro
# ---------------------------------------------------------------------
CPMAddPackage(
  NAME maestro_aer
  GIT_REPOSITORY git@github.com:InvictusWingsSRL/qiskit-aer.git
  GIT_TAG 96e82fdd1c8c1b17d749863ee5095262372e0d7e
  DOWNLOAD_ONLY YES
)

set(AER_INCLUDE_DIR "${maestro_aer_SOURCE_DIR}/src")
set(QCSIM_INCLUDE_DIR "${qcsim_SOURCE_DIR}/QCSim")

get_target_property(
  EIGEN5_INCLUDE_DIR
  Eigen3::Eigen
  INTERFACE_INCLUDE_DIRECTORIES
)

set(EIGEN5_INCLUDE_DIR "${EIGEN5_INCLUDE_DIR}")

get_target_property(
  JSON_INCLUDE_PATH
  nlohmann_json::nlohmann_json
  INTERFACE_INCLUDE_DIRECTORIES
)

set(JSON_INCLUDE_DIR "${JSON_INCLUDE_PATH}")

set(ENV{EIGEN5_INCLUDE_DIR} "${EIGEN5_INCLUDE_DIR}")
set(ENV{JSON_INCLUDE_DIR}   "${JSON_INCLUDE_DIR}")
set(ENV{QCSIM_INCLUDE_DIR}  "${QCSIM_INCLUDE_DIR}")
set(ENV{AER_INCLUDE_DIR}    "${AER_INCLUDE_DIR}")
set(ENV{BOOST_ROOT}         "${Boost_DIR}/../../../")

set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)

CPMAddPackage(
  NAME maestro
  GIT_REPOSITORY git@github.com:QoroQuantum/maestro.git
  GIT_TAG a4d54721a718189362a4a9c8329d7211b662a8f9
  OPTIONS
    "BUILD_PYTHON_BINDINGS OFF"
)

if(maestro_ADDED)
  message(STATUS "Fetched maestro with CPM")
else()
  message(STATUS "Using existing maestro package")
endif()

if(TARGET maestro)
  if(UNIX AND NOT APPLE)
    target_link_options(maestro PRIVATE -Wl,-Bsymbolic-functions)
  endif()

  target_include_directories(maestro PUBLIC "${maestro_SOURCE_DIR}")

  install(
    TARGETS maestro
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
else()
  message(WARNING "Target maestro was not created")
endif()

# =====================================================================
#  Qsim SIMULATOR - By Google 
# =====================================================================
CPMAddPackage(
  NAME qsim
  VERSION 0.22.0
  GIT_REPOSITORY git@github.com:quantumlib/qsim.git
  GIT_TAG v0.22.0
  SOURCE_SUBDIR intentionally_non_existent_dir
)

# =====================================================================
#  CUNQA SIMULATOR - CESGA Quantum Spain
# =====================================================================
CPMAddPackage(
  NAME cunqasimulator
  VERSION 0.1.2
  GIT_REPOSITORY git@github.com:CESGA-Quantum-Spain/cunqasimulator.git
  GIT_TAG v0.1.2
)

if(cunqasimulator_ADDED)
  message(STATUS "Fetched cunqasimulator with CPM")
  set(BUILD_SHARED_LIBS OFF)
else()
  message(STATUS "Using existing cunqasimulator package")
endif()


# =====================================================================
#  Qulacs - Fujitsu simulator
#  Fork with ECR gate implemented
# =====================================================================
CPMAddPackage(
  NAME qulacs
  GIT_REPOSITORY git@github.com:CESGA-Quantum-Spain/qulacs.git
  GIT_TAG main
  DOWNLOAD_ONLY YES
)

if(qulacs_ADDED)
  message(STATUS "Fetched qulacs with CPM")

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  # Qulacs will not set its own flags
  set(OPT_FLAGS "" CACHE STRING "" FORCE)

  # Prevent Qulacs from changing the output directories
  # This bypasses the ARCHIVE_OUTPUT_DIRECTORY logic
  set(PYTHON_SETUP_FLAG ON CACHE BOOL "" FORCE)

  set(_OLD_MODULE_PATH "${CMAKE_MODULE_PATH}")

  set(USE_PYTHON OFF CACHE BOOL "" FORCE)
  set(USE_TEST OFF CACHE BOOL "" FORCE)
  set(USE_OMP ON CACHE BOOL "" FORCE)
  set(BUILD_QULACS_WITH_MPI OFF CACHE BOOL "" FORCE)

  # Prefer the include dir from the Eigen target instead of relying on
  # eigen3_SOURCE_DIR, which may not exist depending on how Eigen was provided.
  get_target_property(_EIGEN3_INCLUDE_DIRS Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)

  set(EIGEN3_INCLUDE_DIR "${_EIGEN3_INCLUDE_DIRS}" CACHE PATH "" FORCE)
  set(EIGEN3_INCLUDE_DIRS "${_EIGEN3_INCLUDE_DIRS}" CACHE PATH "" FORCE)
  set(EIGEN3_FOUND TRUE CACHE BOOL "" FORCE)

  add_subdirectory(
    "${qulacs_SOURCE_DIR}"
    "${qulacs_BINARY_DIR}"
    EXCLUDE_FROM_ALL
  )

  # Restore paths to fix possible:
  # "File /CMakeLists.cmake.in does not exist"
  set(CMAKE_MODULE_PATH "${_OLD_MODULE_PATH}")
else()
  message(STATUS "Using existing qulacs package")
endif()