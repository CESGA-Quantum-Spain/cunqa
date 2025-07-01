import os
import sys
import unittest
from cunqa.transpile import transpiler
from cunqa.circuit import qc_to_json
from cunqa.backend import Backend
from cunqa.qutils import qraise, qdrop, getQPUs

from qiskit import QuantumCircuit
from qiskit.qasm2 import dumps

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)
    
class TestTranspErrors(unittest.TestCase):  
    """
    Test class that checks wether or not all necessary error messages are raised during the transpiler execution.
    """

    def __init__(self, methodName = "runTest"):
        #this __init__ form makes methods beginning by "test" run when it's called by main. Init method is implied in general (in unittest) by the main part, I need it to define the qc instance atribute
        self.qc = QuantumCircuit(5)
        super().__init__(methodName)

    @classmethod 
    def setUpClass(cls): #Method that runs once for each class instantiation
        cls.jobs_to_qdrop = [qraise(1, '00:10:00', fakeqmio=True)]
        cls.jobs_to_qdrop.append(qraise(1, '00:10:00'))
        cls.qpus=getQPUs(local=False)

    @classmethod
    def tearDownClass(cls):  #Method that runs after all tests have been performed
        qdrop(cls.jobs_to_qdrop[0], cls.jobs_to_qdrop[1]) #drops exactly the QPUs we created during the tests

    

    #Key detail for the following three tests: qc has 5 qubits, initial_layout=[1, 0, 2] has 3 qubits 

    def test_init_layout_size_error_qc(self):
        #syntax is assertRaises(exception, callable, *args, **kwds)
        return self.assertRaises(SystemExit, transpiler, self.qc, self.qpus[0].backend, initial_layout=[1,0,2])

    def test_init_layout_size_error_json(self):
        return self.assertRaises(SystemExit, transpiler, qc_to_json(self.qc), self.qpus[0].backend, initial_layout=[1,0,2])

    def test_init_layout_size_error_QASM(self):
        return self.assertRaises(SystemExit, transpiler, dumps(self.qc), self.qpus[0].backend, initial_layout=[1,0,2])

    def test_invalid_circ_format(self):
        invalid_circ = [0,'h', 2, 'some gate', 3,'x'] #transpiler won't recognize this circuit format, as expected
        return self.assertRaises(SystemExit, transpiler, invalid_circ, self.qpus[0].backend, initial_layout=[1,0,2])

    # def test_QASM2_Error(self):

    # def test_QiskitError(self):

    def test_QASM2_Exception(self):
        will_be_qasm = QuantumCircuit(2)
        qasm = dumps(will_be_qasm)
        messed_qasm= qasm + "A problem appeared, ouch!"
        return self.assertRaises(SystemExit, transpiler, messed_qasm, self.qpus[0].backend, initial_layout=[1,0])

    def test_Backend_invalid_type(self):
        invalid_backend = { 'backend': "This is not a valid backend :(", 'another_dict' : {'Thing1 key' : "Thing 1 word"}}
        return self.assertRaises(SystemExit, transpiler, self.qc, invalid_backend, initial_layout=[1,0,2,3,4])
    
    def test_KeyError(self):
        aux_backend=Backend(self.qpus[-2].backend.__dict__) 
        aux_backend.__dict__.pop("basis_gates") #erase one key from the backend dictionary
        return self.assertRaises(SystemExit, transpiler, self.qc, aux_backend, initial_layout=[1,0,2,3,4])
    
    def test_configuration_Exception(self):
        aux2_backend=Backend(self.qpus[-1].backend.__dict__)
        aux2_backend.__dict__["basis_gates"] = True #put nonsense on one key
        return self.assertRaises(SystemExit, transpiler, self.qc, aux2_backend, initial_layout=[1,0,2,3,4])
    
        

if __name__ == "__main__":
    unittest.main(verbosity=2)
