add_subdirectory(utils)
add_subdirectory(sim)
add_subdirectory(comm)

add_library(quantum_task INTERFACE)
target_sources(quantum_task
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      quantum_task/instruction_type.hpp
      quantum_task/circuit.hpp
      quantum_task/run_config.hpp
      quantum_task/quantum_task.hpp
)
target_link_libraries(quantum_task
  INTERFACE
    json
    logger_executor
)

add_library(qpu)
target_sources(qpu
  PRIVATE
    qpu.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      qpu.hpp
)
target_link_libraries(qpu 
    PUBLIC 
        server
        backend
    PRIVATE 
        json 
        quantum_task
        logger_qpu
        constants
)

add_subdirectory(cli)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/utils/json_schema" DESTINATION "$ENV{STORE}/.cunqa")