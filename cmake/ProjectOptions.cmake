include_guard(GLOBAL)

function(cunqa_setup_project_defaults)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()

  set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
  set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
  message(STATUS "C++ version ${CXX_STANDARD} configured.")
  message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

  option(OPENMP_IN_QC "Enable OpenMP parallelization for quantum communications in some simulator adapters" ON)
  option(USE_MPI_BTW_QPU "Using the MPI library for communication between QPUs" OFF)
  option(USE_ZMQ_BTW_QPU "Using the ZMQ library for communication between QPUs" OFF)

  if(NOT USE_MPI_BTW_QPU AND NOT USE_ZMQ_BTW_QPU)
    set(USE_ZMQ_BTW_QPU ON CACHE BOOL "Using ZMQ by default" FORCE)
  endif()

  # Check that $STORE exists
  if(NOT DEFINED ENV{STORE})
      message(FATAL_ERROR "The STORE environment variable is not defined")
  endif()
  set(CUNQA_PATH "$ENV{STORE}/.cunqa" PARENT_SCOPE)
  set(QPUS_FILEPATH "$ENV{STORE}/.cunqa/qpus.json" PARENT_SCOPE)
  set(COMM_FILEPATH "$ENV{STORE}/.cunqa/communications.json" PARENT_SCOPE)

  set(COMPILATION_FOR_GPU FALSE PARENT_SCOPE)
  set(GPU_ARCH "" PARENT_SCOPE)
endfunction()
