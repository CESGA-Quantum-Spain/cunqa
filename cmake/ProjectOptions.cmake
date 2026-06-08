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
  option(USE_GPU "Compile CUNQA with GPU (CUDA) support" OFF)

  if(NOT USE_MPI_BTW_QPU AND NOT USE_ZMQ_BTW_QPU)
    set(USE_ZMQ_BTW_QPU ON CACHE BOOL "Using ZMQ by default" FORCE)
  endif()

  # Check that $STORE exists
  if(NOT DEFINED ENV{STORE})
      message(FATAL_ERROR "The STORE environment variable is not defined")
  endif()

  if(USE_GPU)
    # Architecture           GPUs examples         SM
    # -------------------------------------------------
    # Volta                  V100                  70
    # Turing                 T4, RTX 2080          75
    # Ampere                 A100                  80
    # Ampere GA10x           RTX 30xx              86
    # Ada Lovelace           RTX 40xx              89
    # Hopper                 H100                  90

    set(_cunqa_valid_gpu_archs 70 75 80 86 89 90)
    set(_cunqa_special_gpu_archs all-major all)

    # Example default, optional
    if(NOT DEFINED GPU_ARCH OR GPU_ARCH STREQUAL "")
      set(GPU_ARCH "all-major")
    endif()

    # Allow exactly one special value: all or all-major
    if(GPU_ARCH IN_LIST _cunqa_special_gpu_archs)
      set(_cunqa_cuda_architectures "${GPU_ARCH}")
    else()
      foreach(_arch IN LISTS GPU_ARCH)
        if(NOT "${_arch}" IN_LIST _cunqa_valid_gpu_archs)
          message(FATAL_ERROR
            "Invalid GPU_ARCH '${_arch}'. Valid values are: "
            "${_cunqa_valid_gpu_archs};${_cunqa_special_gpu_archs}.")
        endif()
      endforeach()

      set(_cunqa_cuda_architectures "${GPU_ARCH}")
    endif()

    message(STATUS "USE_GPU enabled. Targeting CUDA architecture(s): ${GPU_ARCH}")
  endif()

endfunction()
