add_library(aer_simulator_adapter "aer_simulator_adapter.cpp")

if (DEFINED AER_GPU)
    set_source_files_properties("aer_simulator_adapter.cpp"
        PROPERTIES LANGUAGE CUDA)

    set_target_properties(aer_simulator_adapter PROPERTIES
        LANGUAGE CUDA
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON)
endif()

target_include_directories(aer_simulator_adapter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(aer_simulator_adapter 
    PUBLIC
        circuit
        run_config
        simulator
    PRIVATE 
        aer_headers
        json 
        logger_qpu 
        LAPACK::LAPACK 
        ${Python_LIBRARIES}
        OpenMP::OpenMP_CXX
)
