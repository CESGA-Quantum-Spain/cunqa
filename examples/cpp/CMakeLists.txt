#Example: Minimal example
add_executable(simplest simplest_example.cpp)
target_link_libraries(simplest PRIVATE client json) 
install(TARGETS simplest DESTINATION cunqa_cpp_examples)

#Example: Minimal Quantum Communications example
add_executable(qc_example qc_example.cpp)
target_link_libraries(qc_example PRIVATE client json) 
install(TARGETS qc_example DESTINATION cunqa_cpp_examples)

