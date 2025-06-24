import os
import sys
import unittest
from cunqa.qpu import getQPUs
from qpu_api import create_QPU, qdrop

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class Test_qraise_qdrop(unittest.TestCase):
    #Problematic class:  qraise is quite slow in writing the QPUs on qpus.json and geQPUs finds it empty if you don't wait a long a amount of time
    """
    Class to test correct functioning of qraise and qdrop commands 
    """

    def __init__(self, methodName = "runTest"):
        self.jobs_to_drop =[]
        super().__init__(methodName)

    def tearDown(self):
        qdrop(self.jobs_to_drop[0]) #drops exactly the QPU we created on each test
        os.system('sleep 1')

    def test_simulator_flag(self):
        self.jobs_to_drop.append(create_QPU(1, '00:10:00', '--simulator=Munich')) #creates a QPU, captures its slurm job id, and stores it in the list self.jobs_to_drop
        os.system('sleep 4')
        qpus=getQPUs()
        return self.assertEqual(qpus[-1].backend.__dict__["simulator"], "MunichSimulator")
    
    def test_backend_flag(self):
        #testing by name doesn't guarantee that the rest of the backend is correct tbh :(
        self.jobs_to_drop.append(create_QPU(1, '00:10:00', '--backend=/mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/CUNQA/code_tests/backend_simple.json')) #i use a simple backend that I configurated for the test
        os.system('sleep 2')
        qqpus=getQPUs()
        return self.assertEqual(qqpus[-1].backend.__dict__["name"],"Backend test test test this name is unique and won't coincide by chance")
    
    def test_fakeqmio_flag(self):
        self.jobs_to_drop.append(create_QPU(1, '00:10:00', '--fakeqmio'))
        #os.system('qraise -n 1 -t 00:10:00 --fakeqmio') this does the same as above but doesn't capture the job id
        os.system('sleep 8')
        qpuss=getQPUs()
        return self.assertEqual(qpuss[-1].backend.__dict__["name"], "FakeQmio" )
    
if __name__ == "__main__":
    unittest.main(verbosity=2)