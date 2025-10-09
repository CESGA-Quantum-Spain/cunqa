cmake_minimum_required(VERSION 3.10)
project(composer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(QAER True)
if (NOT "$ENV{NO_QISKIT_AER}" STREQUAL "")
	add_definitions( -DNO_QISKIT_AER)
	set(QAER False)
endif()
 
SET( EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}" )
IF( NOT EIGEN3_INCLUDE_DIR )
    MESSAGE( FATAL_ERROR "Please set EIGEN3_INCLUDE_DIR env variable to the include directory of Eigen3")
ENDIF()

SET( QCSIM_INCLUDE_DIR "$ENV{QCSIM_INCLUDE_DIR}" )
IF( NOT QCSIM_INCLUDE_DIR )
    MESSAGE( FATAL_ERROR "Please set QCSIM_INCLUDE_DIR env variable to the include directory of qcsim")
ENDIF()

if (QAER)
	SET( JSON_INCLUDE_DIR "$ENV{JSON_INCLUDE_DIR}" )
	IF( NOT JSON_INCLUDE_DIR )
		MESSAGE( FATAL_ERROR "Please set JSON_INCLUDE_DIR env variable to the single include directory of nlohmann json")
	ENDIF()

	SET( AER_INCLUDE_DIR "$ENV{AER_INCLUDE_DIR}" )
	IF( NOT AER_INCLUDE_DIR )
		MESSAGE( FATAL_ERROR "Please set AER_INCLUDE_DIR env variable to the src directory of qiskit aer")
	ENDIF()
endif()

# add here common sources
						
set(MAESTROSRC maestrolib/dllmain.cpp
			   maestrolib/Interface.cpp)


if(APPLE)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
      set(OpenMP_C "${CMAKE_C_COMPILER}")
      set(OpenMP_C_FLAGS "-fopenmp")
      set(OpenMP_CXX_LIB_NAMES "omp")
      set(OpenMP_omp_LIBRARY omp)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
      set(OpenMP_CXX_FLAGS "-fopenmp")
      set(OpenMP_CXX_LIB_NAMES "omp")
      set(OpenMP_omp_LIBRARY omp)
    endif()
endif()

find_package(OpenMP REQUIRED)

include(CheckCXXCompilerFlag)
function(enable_cxx_flag flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_set)
    if(flag_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "amd64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")
    if(UNIX OR APPLE)
        if (NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
            message("Setting SIMD...")
            set(SIMD_FLAGS "-mfma;-mavx2")
            enable_cxx_flag("-mpopcnt")
        endif()
    elseif(MSVC)
        message("Setting SIMD...")	
        set(SIMD_FLAGS "/arch:AVX2")
    endif()
endif()


set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_DEBUG_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS json container serialization) 

IF(NOT Boost_FOUND)
    MESSAGE( FATAL_ERROR "Couldn't find boost libs")
ENDIF()

if (QAER)
	INCLUDE_DIRECTORIES ( ${EIGEN3_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${QCSIM_INCLUDE_DIR} ${JSON_INCLUDE_DIR} ${AER_INCLUDE_DIR})
	
	find_package(BLAS REQUIRED)
	IF(NOT BLAS_FOUND)
		MESSAGE( FATAL_ERROR "Couldn't find blas libs")
	ENDIF()	
else()
	INCLUDE_DIRECTORIES ( ${EIGEN3_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${QCSIM_INCLUDE_DIR})
endif()

LINK_LIBRARIES()

add_library(maestro SHARED ${MAESTROSRC})

if(OpenMP_CXX_FOUND)
    message("Setting OpenMP")
	
	target_link_libraries(maestro PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(maestro PRIVATE ${OpenMP_CXX_FLAGS})
    target_compile_options(maestro PRIVATE ${OpenMP_CXX_FLAGS})	
endif()

IF(QAER AND BLAS_FOUND)
    message("Setting linking blas")

	target_link_libraries(maestro PUBLIC ${BLAS_LIBRARIES})
ENDIF()

IF(SIMD_FLAGS)
    message("Setting SIMD flags")

	target_compile_options(maestro PUBLIC ${SIMD_FLAGS})	
ENDIF()

if (UNIX)
	target_link_libraries(maestro PRIVATE dl)
endif()

IF(Boost_FOUND)
	target_link_libraries(maestro PRIVATE Boost::serialization)
ENDIF()


