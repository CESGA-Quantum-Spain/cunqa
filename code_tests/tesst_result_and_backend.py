import os
import sys
import unittest
from cunqa.backend import Backend
from cunqa.result import Result
from cunqa.qjob import QJobError


# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)
# path to access to json file holding information about the raised QPUs
info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.api_simulator/qpus.json"


class TestBackend(unittest.TestCase):
    """
    Class to check that cunqa.backend.Backend is correctly instantiated with different configuration dicts
    """
    def test_fake_qmio(self):
        backend_fakeqmio = {"basis_gates": ["sx", "x", "rz", "ecr"], "conditional": True, "coupling_map": [[0, 1], [2, 1], [2, 3], [4, 3], [5, 4], [6, 3], [6, 12], [7, 0], [7, 9], [9, 10], [11, 10], [11, 12], [13, 21], [14, 11], [14, 18], [15, 8], [15, 16], [18, 17], [18, 19], [20, 19], [22, 21], [22, 31], [23, 20], [23, 30], [24, 17], [24, 27], [25, 16], [25, 26], [26, 27], [28, 27], [28, 29], [30, 29], [30, 31]], "custom_instructions": "", "description": "FakeQmio backend", "gates": [], "is_simulator": True, "max_shots": 1000000, "memory": True, "n_qubits": 32, "name": "FakeQmio", "simulator": "AerSimulator", "url": "", "version": "/opt/cesga/qmio/hpc/calibrations/2025_04_03__12_00_03.json"}
        QMIO = Backend(backend_fakeqmio)
        return self.assertDictEqual(QMIO.__dict__, backend_fakeqmio)

    def test_munich(self):
        backend_munich = {"basis_gates": ["u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "id", "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg", "swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "ccx", "ccz", "crx", "cry", "crz", "cswap"], "conditional": True, "coupling_map": [], "custom_instructions": "", "description": "Usual Munich simulator.", "gates": [], "is_simulator": True, "max_shots": 10000, "memory": True, "n_qubits": 32, "name": "BasicMunich", "simulator": "MunichSimulator", "url": "https://github.com/cda-tum/mqt-ddsim", "version": "0.0.1"}
        MUNICH = Backend(backend_munich)
        return self.assertDictEqual(MUNICH.__dict__, backend_munich)

    def test_aer(self):
        backend_aer = {"basis_gates": ["u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "id", "x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg", "swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "ccx", "ccz", "crx", "cry", "crz", "cswap"], "conditional": True, "coupling_map": [[]], "custom_instructions": "", "description": "Usual AER simulator.", "gates": [], "is_simulator": True, "max_shots": 10000, "memory": True, "n_qubits": 32, "name": "BasicAer", "simulator": "AerSimulator", "url": "https://github.com/Qiskit/qiskit-aer", "version": "0.0.1"}
        AER = Backend(backend_aer)
        return self.assertDictEqual(AER.__dict__, backend_aer)
    

class TestResult(unittest.TestCase):
    """
    Class to check the correct functioning of the cunqa.qjob.Result class
    """

    def __init__(self, methodName = "runTest"):

        # n = 5 # number of qubits
        # qc = QuantumCircuit(n)
        # qc.x(0); qc.x(n-1); qc.x(n-2)
        # qc.append(QFT(n), range(n))
        # qc.z(3)
        # qc.h(1)
        # qc.append(QFT(n).inverse(), range(n))
        # qc.measure_all()                                 #This circuit was used to obtain the mock results by means of json.loads(qjob._future.get())

        self.ex_registers =  {'meas': [0, 1, 2, 3, 4]}  #Full register dict is [{'q': [0, 1, 2, 3, 4]}, {'meas': [0, 1, 2, 3, 4]}]
        self.ex_results_aer = {'backend_name': '', 'backend_version': '', 'date': '', 'job_id': '', 'metadata': {'max_gpu_memory_mb': 0, 'max_memory_mb': 1031551, 'omp_enabled': True, 'parallel_experiments': 1, 'time_taken_execute': 0.016121071, 'time_taken_parameter_binding': 1.2263e-05}, 'qobj_id': '', 'results': [{'data': {'counts': {'0x13': 243, '0x17': 283, '0x1b': 149, '0x1f': 141, '0x3': 110, '0x7': 9, '0xb': 6, '0xf': 59}}, 'metadata': {'active_input_qubits': [0, 1, 2, 3, 4], 'batched_shots_optimization': False, 'device': 'CPU', 'fusion': {'applied': False, 'enabled': True, 'max_fused_qubits': 5, 'threshold': 14}, 'input_qubit_map': [[4, 4], [3, 3], [2, 2], [1, 1], [0, 0]], 'max_memory_mb': 1031551, 'measure_sampling': True, 'method': 'statevector', 'noise': 'ideal', 'num_bind_params': 1, 'num_clbits': 5, 'num_qubits': 5, 'parallel_shots': 1, 'parallel_state_update': 2, 'remapped_qubits': False, 'required_memory_mb': 1, 'runtime_parameter_bind': False, 'sample_measure_time': 0.00029877, 'time_taken': 0.011041229}, 'seed_simulator': 188, 'shots': 1000, 'status': 'DONE', 'success': True, 'time_taken': 0.011041229}], 'status': 'COMPLETED', 'success': True}
        self.ex_results_munich = {'counts': {'00011': 92, '00111': 8, '01011': 7, '01111': 74, '10011': 231, '10111': 279, '11011': 151, '11111': 158}, 'time_taken': 0.0003500320017337799}
        super().__init__(methodName)

    def test_counts_Aer(self):
        res_aer = Result(self.ex_results_aer, self.ex_registers)
        return self.assertEqual(res_aer.get_counts(), {'10011': 243, '10111': 283, '11011': 149, '11111': 141, '00011': 110, '00111': 9, '01011': 6, '01111': 59})
    
    def test_counts_Munich(self):
        res_munich = Result(self.ex_results_munich, self.ex_registers)
        return self.assertEqual(res_munich.get_counts(), {'00011': 92, '00111': 8, '01011': 7, '01111': 74, '10011': 231, '10111': 279, '11011': 151, '11111': 158})

    def test_result_invalid_type(self):
        res_type_list = [ {'00011': 92, '00111': 8, '01011': 7, '01111': 74, '10011': 231, '10111': 279, '11011': 151, '11111': 158}, 0.0003500320017337799]
        return self.assertRaises(TypeError, Result, res_type_list, self.ex_registers)

    def test_error_in_result(self):
        res_w_error = {"ERROR": "Yes, there is an error"}
        return self.assertRaises(QJobError, Result, res_w_error, self.ex_registers)
    
    # def test_no_counts(self):
    #     res_w_out_counts= {'no counts here': {}, 'time_taken': 0.0003500320017337799}
    #     return self.assertRaises(KeyError, Result, res_w_out_counts, self.ex_registers)          #Changing the reading procedure, this will be implemented later
    
    # def test_counts_exception(self):

    #     return self.assertRaises(Exception, Result,  , self.ex_registers)

    def test_dict_aer(self):
        res_aer = Result(self.ex_results_aer, self.ex_registers)
        return self.assertEqual(res_aer.get_dict(), self.ex_results_aer)
    
    def test_dict_munich(self):
        res_munich = Result(self.ex_results_munich, self.ex_registers)
        return self.assertEqual(res_munich.get_dict(), self.ex_results_munich)