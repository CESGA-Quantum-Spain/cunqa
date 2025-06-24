import os
import sys
import unittest
from cunqa.backend import Backend
from cunqa.qclient import QClient
from cunqa.qpu import getQPUs, QPU
from qpu_api import create_QPU, qdrop

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


#I test the class Backend here, too, as it's relevant to QPU and it's short
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




class TestQPU(unittest.TestCase):  
    """
    Class to check the correct functioning of the cunqa.qpu.QPU class
    """
    def __init__(self, methodName = "runTest"):
        self.backend_instance = Backend({"basis_gates": ["sx", "x", "rz", "ecr"], "conditional": True, "coupling_map": [[0, 1], [2, 1], [2, 3], [4, 3], [5, 4], [6, 3], [6, 12], [7, 0], [7, 9], [9, 10], [11, 10], [11, 12], [13, 21], [14, 11], [14, 18], [15, 8], [15, 16], [18, 17], [18, 19], [20, 19], [22, 21], [22, 31], [23, 20], [23, 30], [24, 17], [24, 27], [25, 16], [25, 26], [26, 27], [28, 27], [28, 29], [30, 29], [30, 31]], "custom_instructions": "", "description": "FakeQmio backend", "gates": [], "is_simulator": True, "max_shots": 1000000, "memory": True, "n_qubits": 32, "name": "FakeQmio", "simulator": "AerSimulator", "url": "", "version": "/opt/cesga/qmio/hpc/calibrations/2025_04_03__12_00_03.json"})
        self.qclient_instance = QClient(info_path)
        super().__init__(methodName)

    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = create_QPU(1, '00:10:00') # 'qraise -n 1 -t 00:30:00' is run and its SLURM_JOB_ID is captured
        os.system('sleep 3')
        cls.qpu = getQPUs()[-1]

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)
        os.system('sleep 1')


    ###################### INIT #############################

    def test_no_id_provided(self):
        return self.assertRaises(SystemExit, QPU, None, self.qclient_instance, self.backend_instance)
    
    def test_id_ivalid_type(self):
        return self.assertRaises(SystemExit, QPU, 'invalid id (bc str)', self.qclient_instance, self.backend_instance)

    def test_no_qclient(self):
        return self.assertRaises(SystemExit, QPU, 0, None, self.backend_instance) #the zero id is arbitrary

    def test_qclient_wrong_type(self):
        return self.assertRaises(SystemExit, QPU, 0, 'not a Qclient', self.backend_instance)

    def test_no_backend(self):
        return self.assertRaises(SystemExit, QPU, 0, self.qclient_instance, None)

    def test_backend_wrong_type(self):
        return self.assertRaises(SystemExit, QPU, 0, self.qclient_instance, 'not a backend')

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
        #The next line checks that the 'handwritten' dict is contained in the first one. I do this to avoid having to compare job._future which is a instance that I don't have access to
        return self.assertEqual(job.__dict__, job.__dict__ | {'_QPU': self.qpu, '_result': None, '_updated': False, '_cregisters': {'meas': [0, 1, 2, 3, 4]}, '_circuit': [{'name': 'x', 'qubits': [0], 'params': []}, {'name': 'x', 'qubits': [4], 'params': []}, {'name': 'x', 'qubits': [3], 'params': []}, {'name': 'QFT', 'qubits': [0, 1, 2, 3, 4], 'params': []}, {'name': 'z', 'qubits': [3], 'params': []}, {'name': 'h', 'qubits': [1], 'params': []}, {'name': 'IQFT', 'qubits': [0, 1, 2, 3, 4], 'params': []}, {'name': 'measure', 'qubits': [0], 'memory': [0]}, {'name': 'measure', 'qubits': [1], 'memory': [1]}, {'name': 'measure', 'qubits': [2], 'memory': [2]}, {'name': 'measure', 'qubits': [3], 'memory': [3]}, {'name': 'measure', 'qubits': [4], 'memory': [4]}], '_execution_config': ' {"config":{"shots": 1024, "method": "statevector", "memory_slots": 5, "seed": 188}, "instructions":[{"name": "x", "qubits": [0], "params": []}, {"name": "x", "qubits": [4], "params": []}, {"name": "x", "qubits": [3], "params": []}, {"name": "QFT", "qubits": [0, 1, 2, 3, 4], "params": []}, {"name": "z", "qubits": [3], "params": []}, {"name": "h", "qubits": [1], "params": []}, {"name": "IQFT", "qubits": [0, 1, 2, 3, 4], "params": []}, {"name": "measure", "qubits": [0], "memory": [0]}, {"name": "measure", "qubits": [1], "memory": [1]}, {"name": "measure", "qubits": [2], "memory": [2]}, {"name": "measure", "qubits": [3], "memory": [3]}, {"name": "measure", "qubits": [4], "memory": [4]}] }'})

    def test_run_error_passed_from_qjob(self): #I want to test that the error is correctly captured during the call to QJob. One unique test should suffice as all errors share behaviour
        invalid_circ = [0,'h', 2, 'some gate', 3,'x']
        return self.assertRaises(SystemExit, self.qpu.run, invalid_circ)


    
class Test_getQPUs(unittest.TestCase):

    #I found errors that are currently being addressed, I'll make this test later-on
    """
    Class to check wether the getQPUs method functions correctly on all situations.
    """
    def test_not_raised(self):
        os.system('qdrop --all')
        os.system('sleep 5')
        return self.assertRaises(SystemExit, getQPUs)
    
    # def test_getQPUs_twice(self):
    #     os.system('qraise -n 8 -t 00:30:00')
    #     os.system('sleep 3')
    #     qpus=getQPUs()
    #     return self.assertRaises(SystemError, getQPUs)
    
    # def test_QPUs_with_diff_backends(self):
    #     return
    
if __name__ == "__main__":
    unittest.main(verbosity=2)