add_subdirectory(AER)
add_subdirectory(CUNQA)
add_subdirectory(Munich)
add_subdirectory(Maestro)
add_subdirectory(Qsim)
add_subdirectory(QuEST)
add_subdirectory(Qulacs)

add_library(simulator_factory STATIC simulator_factory.cpp)
target_link_libraries(simulator_factory
  PUBLIC
    simulator
    AER_simulator_adapter
    cunqa_simulator_adapter
    munich_simulator_adapter
    maestro_simulator_adapter
    qsim_simulator_adapter
    quest_simulator_adapter
    qulacs_simulator_adapter
)
target_include_directories(simulator_factory PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
