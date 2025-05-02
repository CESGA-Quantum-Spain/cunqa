import os
import sys
import json
import unittest
import cunqa.backend as backend
import cunqa.circuit as circuit
import numpy as np
from cunqa.qjob import _divide, Result, QJob, gather, QJobError
from cunqa.qpu import getQPUs
from qpu_api import create_QPU, qdrop

from qiskit import QuantumCircuit
from qiskit.circuit.library import QFT
from qiskit.qasm2 import dumps
from qiskit.qasm2.exceptions import QASM2Error

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)




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






class TestQJob(unittest.TestCase):
    """
    Class to check the correct functioning of the cunqa.qjob.QJob class
    """

    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = create_QPU(1, '00:10:00')
        os.system('sleep 3')
        cls.qpus = getQPUs()
        cls.qc= QuantumCircuit(3)
        cls.run_config = {"shots":1024, "method":"statevector", "seed": 188}

        
    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)


    ######## INIT METHOD ########################################################################################

    def test_non_boolean_transpile_error(self):
        return self.assertRaises(TypeError, QJob, self.qpus[-1], self.qc, transpile=0)

    # def test_transpilation_exception(self):

    def test_invalid_circuit_format(self):
        invalid_circ = [0,'h', 2, 'some gate', 3,'x'] #this circuit won't be recognized, obviously
        return self.assertRaises(QJobError, QJob, self.qpus[-1], invalid_circ, transpile = False)

    def test_circuit_missing_keys(self):
        circuit = {  #this circuit is missing the instructions key
                "num_qubits": 3,
                "num_clbits": 3,
                "quantum_registers": {"q" : [0, 1, 2] },
                "classical_registers": {"meas": [0, 1, 2]}
            }
        return self.assertRaises(QJobError, QJob, self.qpus[-1], circuit, transpile=False)

    def test_QASM2_translation_error(self):
        will_be_qasm = QuantumCircuit(2)
        qasm = dumps(will_be_qasm)
        messed_qasm= qasm + "a problem appeared, ouch!"
        return self.assertRaises(QJobError, QJob, self.qpus[-1], messed_qasm, transpile= False)

    # def test_Qiskit_circuit_error(self):

    # def test_circuit_exception(self):

    # def test_no_instructions_circuit(self):

    # def test_configuration_exception(self):
    #     return self.assertRaises(SystemExit, QJob, self.qpus[0], self.qc, transpile=False, initial_layout = None, opt_level = 1, bogus_key = "Beautiful morning, innit?")

    ######## SUBMIT #######################################################################################################

    # def test_submission_error(self):

    ######## UPGRADE_PARAMETERS #############################################################################################

    def test_previous_submission_flushed(self):
        theta = np.pi/2
        param_circ = QuantumCircuit(2)
        param_circ.rx(theta, 0)
        param_circ.cx(0,1)
        param_circ.measure_all()

        assert self.qpus[-1].backend.__dict__["name"] == "BasicAer", 'The used QPU doesnt use Aer'
        #IMPORTANT that the self.qpu[-1] we raised is AER
        job = self.qpus[-1].run(param_circ, transpile = False, **self.run_config)   # the result here would be {'00': 502, '11': 498}     (WITH AER)!!!

        updated_job = job.upgrade_parameters([np.pi/4]) #whereas the result here should be {'00': 859, '11': 141}            (WITH AER)!!!

        return self.assertEqual(updated_job.result().get_counts(), {'00': 879, '11': 145})
    
    # def test_error_upgrading(self):                     #When this one got errors, the previous test return an error as the returned counts were those of the non-updated circuit
    #     os.system('qraise -n 1 -t 00:10:00')        #raise AER qpu to ensure we get the desired result
    #     os.system('sleep 2')

    #     qc= QuantumCircuit(1)
    #     qc.rx(np.pi/4, 0)
    #     job = self.qpus[-1].run(qc, transpile = True, shots=1000) 
    #     new_job = job.upgrade_parameters(['not a parameter'])
    #     return self.assertRaises(QJobError, new_job.result().get_counts)

    ########## RESULT ######################################################################################################

    # def test_ask_result_w_out_submission(self):
    #     jobb = QJob(self.qpus[0], self.qc, False)
    #     return self.assertEqual(jobb.result(), None)   #Test passes but it's unimportant. The user should interact with QJob through QPU.run

    # def test_updated_is_true_functioning(self):

    # def test_result_exception(self):

    ########## TIME_TAKEN ##########################################################################################################

    def test_qjob_not_finished(self):
        #I set 'status': "RUNNING" in the following dictionary and eliminate time_taken metrics
        res ={'backend_name': '', 'backend_version': '', 'date': '', 'job_id': '', 'metadata': {'max_gpu_memory_mb': 0, 'max_memory_mb': 1031551, 'omp_enabled': True, 'parallel_experiments': 1}, 'qobj_id': '', 'results': [{'data': {'counts': {'0x13': 243, '0x17': 283, '0x1b': 149, '0x1f': 141, '0x3': 110, '0x7': 9, '0xb': 6, '0xf': 59}}, 'metadata': {'active_input_qubits': [0, 1, 2, 3, 4], 'batched_shots_optimization': False, 'device': 'CPU', 'fusion': {'applied': False, 'enabled': True, 'max_fused_qubits': 5, 'threshold': 14}, 'input_qubit_map': [[4, 4], [3, 3], [2, 2], [1, 1], [0, 0]], 'max_memory_mb': 1031551, 'measure_sampling': True, 'method': 'statevector', 'noise': 'ideal', 'num_bind_params': 1, 'num_clbits': 5, 'num_qubits': 5, 'parallel_shots': 1, 'parallel_state_update': 2, 'remapped_qubits': False, 'required_memory_mb': 1, 'runtime_parameter_bind': False}, 'seed_simulator': 188, 'shots': 1000, 'status': 'DONE', 'success': True}], 'status': 'RUNNING', 'success': True}
        qc=QuantumCircuit(1)
        running_job = QJob(self.qpus[-1], qc, False)
        running_job._result = Result(res, {'meas': [0, 1, 2, 3, 4]})
        return self.assertRaises(SystemExit, running_job.time_taken)

    def test_no_qjob_error(self):
        qc=QuantumCircuit(3)
        not_to_be_submitted = QJob(self.qpus[-1], qc, False)
        return self.assertRaises(SystemExit, not_to_be_submitted.time_taken)




class TestGather(unittest.TestCase):
    """
    Class to check the correct functioning of the cunqa.qjob.gather function
    """
    def test_list_elements_type_error(self): #test wether or not an error is raised when I provide a list w/ elements which are not QJobs
        return self.assertRaises(SystemExit, gather, ["ola mira", "perdona", "soy de ourense"])
    
    def test_qjob_type_error(self): # Check error when I provide something other than a QJob, in this case an int
        return self.assertRaises(SystemError, gather, 0)
    


if __name__ == "__main__":
    unittest.main(verbosity=2)