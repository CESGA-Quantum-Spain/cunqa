# Server
add_library(server "${CMAKE_CURRENT_SOURCE_DIR}/asio_server.cpp")
target_include_directories(server PRIVATE "${Boost_INCLUDE_DIRS}")
target_link_libraries(server PRIVATE "${Boost_LIBRARIES}" logger_qpu
                             PUBLIC json)           

# Client
add_library(client STATIC "${CMAKE_CURRENT_SOURCE_DIR}/asio_client.cpp")
target_include_directories(client PRIVATE "${Boost_INCLUDE_DIRS}")
target_link_libraries(client PRIVATE "${Boost_LIBRARIES}" logger_client)
set_target_properties(client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

