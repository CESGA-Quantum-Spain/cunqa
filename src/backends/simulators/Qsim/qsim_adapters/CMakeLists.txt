add_library(qsim_adapters "${CMAKE_CURRENT_SOURCE_DIR}/qsim_simulator_adapter.cpp")
target_include_directories(qsim_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" 
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/.." 
                                                   "${qsim_SOURCE_DIR}/lib"
                                                   ) 


target_link_libraries(qsim_adapters PUBLIC classical_channel 
                                    PRIVATE json logger_qpu
                                            OpenMP::OpenMP_CXX)
target_compile_definitions(qsim_adapters PRIVATE OPENMP_IN_QC)