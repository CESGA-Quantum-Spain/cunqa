from qiskit_aer import AerSimulator
from qiskit_aer.backends.backend_utils import MAX_QUBITS_STATEVECTOR
from qiskit_aer.version import __version__


default_backends = {"ideal_aer": {
            "backend_name": "aer_simulator",
            "backend_version": __version__,
            "n_qubits": MAX_QUBITS_STATEVECTOR,
            "url": "https://github.com/Qiskit/qiskit-aer",
            "simulator": True,
            "local": True,
            "conditional": True,
            "memory": True,
            "max_shots": 1e6,
            "description": "A C++ Qasm simulator with noise",
            "coupling_map": None,
            "basis_gates": AerSimulator()._BASIS_GATES["automatic"],
            "custom_instructions": AerSimulator()._CUSTOM_INSTR["automatic"],
            "gates": [],
            "shots":1024, 
            "method":"automatic",
            "device":"CPU",
            "precision":"double",
            "executor":None,
            "max_job_size":None,
            "max_shot_size":None,
            "enable_truncation":True,
            "zero_threshold":1e-10,
            "validation_threshold":None,
            "max_parallel_threads":None,
            "max_parallel_experiments":None,
            "max_parallel_shots":None,
            "max_memory_mb":None,
            "fusion_enable":True,
            "fusion_verbose":False,
            "fusion_max_qubit":None,
            "fusion_threshold":None,
            "accept_distributed_results":None,
            "memory":None,
            "noise_model":None,
            "seed_simulator":None,
            "cuStateVec_enable":False,
            "blocking_qubits":None,
            "blocking_enable":False,
            "chunk_swap_buffer_qubits":None,
            "batched_shots_gpu":False,
            "batched_shots_gpu_max_qubits":16,
            "num_threads_per_device":1,
            "shot_branching_enable":False,
            "shot_branching_sampling_enable":False,
            "statevector_parallel_threshold":14,
            "statevector_sample_measure_opt":10,
            "stabilizer_max_snapshot_probabilities":32,
            "extended_stabilizer_sampling_method":"resampled_metropolis",
            "extended_stabilizer_metropolis_mixing_time":5000,
            "extended_stabilizer_approximation_error":0.05,
            "extended_stabilizer_norm_estimation_samples":100,
            "extended_stabilizer_norm_estimation_repetitions":3,
            "extended_stabilizer_parallel_threshold":100,
            "extended_stabilizer_probabilities_snapshot_samples":3000,
            "matrix_product_state_truncation_threshold":1e-16,
            "matrix_product_state_max_bond_dimension":None,
            "mps_sample_measure_algorithm":"mps_heuristic",
            "mps_log_data":False,
            "mps_swap_direction":"mps_swap_left",
            "chop_threshold":1e-8,
            "mps_parallel_threshold":14,
            "mps_omp_threads":1,
            "mps_lapack":False,
            "tensor_network_num_sampling_qubits":10,
            "use_cuTensorNet_autotuning":False,
            "runtime_parameter_bind_enable":False }

    }

default_fakeqmio = {"calibration_file":"default_qmio_calibration.json"}

allowed_backend_options = {"general":["method", "device", "precision", "executor","max_job_size", "max_shot_size", "enable_truncation", "zero_threshold", "validation_threshold", "max_parallel_threads", "max_parallel_experiments", "max_parallel_shots", "max_memory_mb", "accept_distributed_results", "runtime_parameter_bind_enable", "fusion_enable", "fusion_verbose", "fusion_max_qubit", "fusion_threshold", "accept_distributed_results", "memory", "noise_model", "seed_simulator", "cuStateVec_enable", "blocking_qubits", "blocking_enable", "chunk_swap_buffer_qubits", "batched_shots_gpu", "batched_shots_gpu_max_qubits", "num_threads_per_device", "shot_branching_enable", "shot_branching_sampling_enable"], 
            "when_statevector":["statevector_parallel_threshold", "statevector_sample_measure_opt"], 
            "when_stabilizer":["stabilizer_max_snapshot_probabilities"], 
            "when_extended_stabilizer":["extended_stabilizer_sampling_method", "extended_stabilizer_metropolis_mixing_time", "extended_stabilizer_approximation_error", "extended_stabilizer_norm_estimation_samples", "extended_stabilizer_norm_estimation_repetitions", "extended_stabilizer_parallel_threshold", "extended_stabilizer_probabilities_snapshot_samples"], 
            "when_matrix_product":["matrix_product_state_max_bond_dimension", "matrix_product_state_truncation_threshold", "mps_sample_measure_algorithm", "mps_log_data", "mps_swap_direction", "chop_threshold", "mps_parallel_threshold", "mps_omp_threads", "mps_lapack"], 
            "when_tensor_network":["tensor_network_num_sampling_qubits", "use_cuTensorNet_autotuning"],
            "our_own_options": ["calibration_file"]
            }
    
aer_default_configuration = ["backend_name", "backend_version", "n_qubits", "url", "simulator", "local", "conditional", "memory", "max_shots", "description", "coupling_map", "basis_gates", "custom_instructions", "gates" ]

all_allowed_backend_options = ["backend_name", "backend_version", "n_qubits", "url", "simulator", "local", "conditional", "memory", "max_shots", "description", "coupling_map", "basis_gates", "custom_instructions", "gates", "shots", "method", "device", "precision", "executor","max_job_size", "max_shot_size", "enable_truncation", "zero_threshold", "validation_threshold", "max_parallel_threads", "max_parallel_experiments", "max_parallel_shots", "max_memory_mb", "accept_distributed_results", "runtime_parameter_bind_enable", "fusion_enable", "fusion_verbose", "fusion_max_qubit", "fusion_threshold", "accept_distributed_results", "memory", "noise_model", "seed_simulator", "cuStateVec_enable", "blocking_qubits", "blocking_enable", "chunk_swap_buffer_qubits", "batched_shots_gpu", "batched_shots_gpu_max_qubits", "num_threads_per_device", "shot_branching_enable", "shot_branching_sampling_enable", "statevector_parallel_threshold", "statevector_sample_measure_opt", "stabilizer_max_snapshot_probabilities", "extended_stabilizer_sampling_method", "extended_stabilizer_metropolis_mixing_time", "extended_stabilizer_approximation_error", "extended_stabilizer_norm_estimation_samples", "extended_stabilizer_norm_estimation_repetitions", "extended_stabilizer_parallel_threshold", "extended_stabilizer_probabilities_snapshot_samples", "matrix_product_state_max_bond_dimension", "matrix_product_state_truncation_threshold", "mps_sample_measure_algorithm", "mps_log_data", "mps_swap_direction", "chop_threshold", "mps_parallel_threshold", "mps_omp_threads", "mps_lapack", "tensor_network_num_sampling_qubits", "use_cuTensorNet_autotuning", "calibration_file"]

allowed_fakeqmio_options = ["calibration_file", "thermal_relaxation", "temperature", "gate_error", "readout_error", "logging_level", "logging_filename"]


