set(CPPLOGGER_GLOBAL_LEVEL SPDLOG_LEVEL_DEBUG)

# =====================================================
#  QPU logger
# =====================================================
add_library(logger_qpu STATIC)
target_sources(logger_qpu
  PRIVATE
    logger_qpu.cpp
  PUBLIC
    FILE_SET HEADERS
    FILES
      logger.hpp
)
target_link_libraries(logger_qpu 
  PUBLIC 
    spdlog::spdlog
    constants
)
target_compile_definitions(logger_qpu 
  PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)

# =====================================================
#  Client logger
# =====================================================
add_library(logger_client STATIC)
target_sources(logger_client
  PRIVATE
    logger_client.cpp
  PUBLIC
    FILE_SET HEADERS
    FILES
      logger.hpp
)
target_link_libraries(logger_client 
  PUBLIC
    spdlog::spdlog
    constants
)
target_compile_definitions(logger_client 
  PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)
set_target_properties(logger_client 
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# =====================================================
#  Quantum comm executor logger
# =====================================================
add_library(logger_executor STATIC)
target_sources(logger_executor
  PRIVATE
    logger_executor.cpp
  PUBLIC
    FILE_SET HEADERS
    FILES
      logger.hpp
)
target_link_libraries(logger_executor PUBLIC spdlog::spdlog)
target_compile_definitions(logger_executor 
  PUBLIC
    SPDLOG_ACTIVE_LEVEL=${CPPLOGGER_GLOBAL_LEVEL}
)
set_target_properties(logger_executor 
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)