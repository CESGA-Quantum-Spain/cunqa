add_library(write_sbatch STATIC)
target_sources(write_sbatch
  PRIVATE
    write_sbatch.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      write_sbatch.hpp
      args_qraise.hpp
      utils_qraise.hpp
)
target_link_libraries(write_sbatch 
  PUBLIC
    morrisfranken::argparse
    json
    logger_client
  PRIVATE
    constants
)