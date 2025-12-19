#Example 1: Simple sending of circuit
add_executable(example1 example1.cpp)
target_link_libraries(example1 PRIVATE client json) 

#Example 2: Sending consecutive circuits, both correct
add_executable(example2 example2.cpp)
target_link_libraries(example2 PRIVATE client json) 

#Example 3: Sending consecutive circuits, both incorrect
add_executable(example3 example3.cpp)
target_link_libraries(example3 PRIVATE client json) 

#Example 4: First QC example
add_executable(qc_example qc_example.cpp)
target_link_libraries(qc_example PRIVATE client json) 