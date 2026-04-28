option(USE_MPI_BTW_QPU "Using the MPI library for communication between QPUs" OFF)
option(USE_ZMQ_BTW_QPU "Using the ZMQ library for communication between QPUs" OFF)

# Set default if both are OFF
if(NOT USE_MPI_BTW_QPU AND NOT USE_ZMQ_BTW_QPU)
    set(USE_ZMQ_BTW_QPU ON CACHE BOOL "Using ZMQ by default" FORCE)
endif()

add_subdirectory(utils)
add_subdirectory(quantum_task)
add_subdirectory(sim)
add_subdirectory(comm)

add_library(qpu qpu.cpp)
target_link_libraries(qpu 
    PUBLIC 
        server
        backend
    PRIVATE 
        json 
        quantum_task
        logger_qpu
)
target_include_directories(qpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(cli)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/utils/json_schema" DESTINATION "$ENV{STORE}/.cunqa")