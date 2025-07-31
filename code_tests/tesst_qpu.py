import os
import sys
import unittest

# HOME = os.getenv("HOME")
# sys.path.insert(0, HOME)

from cunqa.backend import Backend
from cunqa.qclient import QClient
from cunqa.qpu import QPU
from cunqa.qutils import qraise, qdrop, getQPUs

from qiskit import QuantumCircuit
from qiskit.circuit.library import QFT

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)
# path to access to json file holding information about the raised QPUs
info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.api_simulator/qpus.json"


class TestQPU(unittest.TestCase):  
    """
    Class to check the correct functioning of the cunqa.qpu.QPU class
    """
    def __init__(self, methodName = "runTest"):
        self.backend_instance = Backend({"basis_gates": ["sx", "x", "rz", "ecr"], "conditional": True, "coupling_map": [[0, 1], [2, 1], [2, 3], [4, 3], [5, 4], [6, 3], [6, 12], [7, 0], [7, 9], [9, 10], [11, 10], [11, 12], [13, 21], [14, 11], [14, 18], [15, 8], [15, 16], [18, 17], [18, 19], [20, 19], [22, 21], [22, 31], [23, 20], [23, 30], [24, 17], [24, 27], [25, 16], [25, 26], [26, 27], [28, 27], [28, 29], [30, 29], [30, 31]], "custom_instructions": "", "description": "FakeQmio backend", "gates": [], "is_simulator": True, "max_shots": 1000000, "memory": True, "n_qubits": 32, "name": "FakeQmio", "simulator": "AerSimulator", "url": "", "version": "/opt/cesga/qmio/hpc/calibrations/2025_04_03__12_00_03.json"})
        self.qclient_instance = QClient()
        super().__init__(methodName)

    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = qraise(1, '00:10:00')
        qpus = getQPUs(local=False)
        cls.qpu = qpus[-1]

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)
        os.system('sleep 1')


    ###################### INIT #############################

    # def test_no_id_provided(self):
    #     return self.assertRaises(SystemExit, QPU, None, self.qclient_instance, self.backend_instance, "family", ("127.0.0.1","15615"))
    
    # def test_id_ivalid_type(self):
    #     return self.assertRaises(SystemExit, QPU, 'invalid id (bc str)', self.qclient_instance, self.backend_instance, "family", ("127.0.0.1","15615"))

    # def test_no_qclient(self):
    #     return self.assertRaises(SystemExit, QPU, 0, None, self.backend_instance, "family", ("127.0.0.1","15615")) #the zero id is arbitrary

    # def test_qclient_wrong_type(self):
    #     return self.assertRaises(SystemExit, QPU, 0, 'not a Qclient', self.backend_instance, "family", ("127.0.0.1","15615"))

    # def test_no_backend(self):
    #     return self.assertRaises(SystemExit, QPU, 0, self.qclient_instance, None, "family", ("127.0.0.1","15615"))

    # def test_backend_wrong_type(self):
    #     return self.assertRaises(SystemExit, QPU, 0, self.qclient_instance, 'not a backend', "family", ("127.0.0.1","15615"))

    ######################## RUN #############################

    def test_correct_qjob_is_returned(self):
        n = 5 # number of qubits
        qc = QuantumCircuit(n)
        qc.x(0); qc.x(n-1); qc.x(n-2)
        qc.append(QFT(n), range(n))
        qc.z(3)
        qc.h(1)
        qc.append(QFT(n).inverse(), range(n))
        qc.measure_all() 
        job = self.qpu.run(qc, transpile=False)
        print(job.__dict__)
        #The next line checks that the 'handwritten' dict is contained in the first one. I do this to avoid having to compare job._future which is a instance that I don't have access to
        return self.assertEqual(job.__dict__, job.__dict__ | {'_result': None, '_updated': False, '_cregisters': {'meas': [0, 1, 2, 3, 4]}, '_circuit': [{'name': 'x', 'qubits': [0], 'params': []}, {'name': 'x', 'qubits': [4], 'params': []}, {'name': 'x', 'qubits': [3], 'params': []}, {'name': 'QFT', 'qubits': [0, 1, 2, 3, 4], 'params': []}, {'name': 'z', 'qubits': [3], 'params': []}, {'name': 'h', 'qubits': [1], 'params': []}, {'name': 'IQFT', 'qubits': [0, 1, 2, 3, 4], 'params': []}, {'name': 'measure', 'qubits': [0], 'memory': [0]}, {'name': 'measure', 'qubits': [1], 'memory': [1]}, {'name': 'measure', 'qubits': [2], 'memory': [2]}, {'name': 'measure', 'qubits': [3], 'memory': [3]}, {'name': 'measure', 'qubits': [4], 'memory': [4]}], '_execution_config': ' {"config":{"shots": 1024, "method": "statevector", "num_clbits": 5, "num_qubits": 5, "seed": 188}, "instructions":[{"name": "x", "qubits": [0], "params": []}, {"name": "x", "qubits": [4], "params": []}, {"name": "x", "qubits": [3], "params": []}, {"name": "QFT", "qubits": [0, 1, 2, 3, 4], "params": []}, {"name": "z", "qubits": [3], "params": []}, {"name": "h", "qubits": [1], "params": []}, {"name": "IQFT", "qubits": [0, 1, 2, 3, 4], "params": []}, {"name": "measure", "qubits": [0], "memory": [0]}, {"name": "measure", "qubits": [1], "memory": [1]}, {"name": "measure", "qubits": [2], "memory": [2]}, {"name": "measure", "qubits": [3], "memory": [3]}, {"name": "measure", "qubits": [4], "memory": [4]}], "num_qubits":5 }'})

    def test_run_error_passed_from_qjob(self): #I want to test that the error is correctly captured during the call to QJob. One unique test should suffice as all errors share behaviour
        invalid_circ = [0,'h', 2, 'some gate', 3,'x']
        return self.assertRaises(SystemExit, self.qpu.run, invalid_circ)

    
if __name__ == "__main__":
    unittest.main(verbosity=2)