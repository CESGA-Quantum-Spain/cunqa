add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall> $<$<COMPILE_LANGUAGE:CXX>:-Wextra>)

add_subdirectory(utils)
add_subdirectory(sim)
add_subdirectory(comm)

add_library(qpu STATIC)
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
        logger_qpu
        constants
)

add_library(circuit INTERFACE)
target_sources(circuit
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      circuit.hpp
)

add_subdirectory(dynamic_circuit)
add_subdirectory(cli)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/utils/json_schema" DESTINATION "$ENV{STORE}/.cunqa")