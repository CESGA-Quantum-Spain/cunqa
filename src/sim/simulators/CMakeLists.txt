add_subdirectory(AER)
#add_subdirectory(Munich)
#add_subdirectory(CUNQA)
#add_subdirectory(Maestro)
#add_subdirectory(Qulacs)

add_library(simulator_factory simulator_factory.cpp)
target_link_libraries(simulator_factory
    PRIVATE 
    aer_simulator_adapter
)
target_include_directories(simulator_factory PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
