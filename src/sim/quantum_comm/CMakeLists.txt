add_library(qc_executor qc_executor.cpp)
target_link_libraries(qc_executor 
  PUBLIC
    backend
    simulator
    quantum_task
    classical_channel
    json
  PRIVATE
    logger_qpu
)
