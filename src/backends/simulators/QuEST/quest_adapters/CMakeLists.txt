add_library(quest_adapters "${CMAKE_CURRENT_SOURCE_DIR}/quest_simulator_adapter.cpp")
target_include_directories(quest_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" 
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/.." 
                                                   "${quest_SOURCE_DIR}/lib"
                                                   ) 


target_link_libraries(quest_adapters PUBLIC classical_channel 
                                    PRIVATE json logger_qpu
                                            OpenMP::OpenMP_CXX)
target_compile_definitions(quest_adapters PRIVATE OPENMP_IN_QC)