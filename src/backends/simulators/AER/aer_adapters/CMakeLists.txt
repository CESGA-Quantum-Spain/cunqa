add_library(aer_adapters "${CMAKE_CURRENT_SOURCE_DIR}/aer_simulator_adapter.cpp")

if (DEFINED AER_GPU)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/aer_simulator_adapter.cpp"
        PROPERTIES LANGUAGE CUDA)

    set_target_properties(aer_adapters PROPERTIES
        LANGUAGE CUDA
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON)
endif()

target_include_directories(aer_adapters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(aer_adapters PUBLIC classical_channel aer_headers
                                            PRIVATE json logger_qpu LAPACK::LAPACK ${Python_LIBRARIES})
