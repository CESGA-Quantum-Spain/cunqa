#add_library(config INTERFACE)

#target_link_libraries(config PUBLIC compiler_flags)
target_include_directories(API-Simulator INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/backend_config.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/run_config.hpp" 
              "${CMAKE_CURRENT_SOURCE_DIR}/qpu_config.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/net_config.hpp"  
        DESTINATION include/config
       )