
if(COMM_LIB STREQUAL "ASIO")
    add_library(strategy INTERFACE)
    target_link_libraries(strategy INTERFACE compiler_flags Boost::Boost)
    target_include_directories(strategy INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/asio")
    message(DEBUG "ASIO library added.")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio_client.hpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio_server.hpp" 
            DESTINATION include/comm/strategies/asio
        )
elseif(COMM_LIB == "ZMQ")
    message(FATAL_ERROR "No implementation of ZMQ servers developed.")
elseif(COMM_LIB == "CROW")
    message(FATAL_ERROR "No implementation of CROW servers developed.")
endif()