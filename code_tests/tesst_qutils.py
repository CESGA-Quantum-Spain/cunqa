import os
import sys
import unittest
from cunqa.qutils import qraise, qdrop, getQPUs
from cunqa.circuit import CunqaCircuit
import random

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class Test_qdrop(unittest.TestCase):
    """
    Class to test correct functioning of the qdrop commands
    """

    def test_qdrop(self):
        last_raised = qraise(1, '00:10:00', family='unique name 25.06.2025 goku super saiyan')
        qpus_before = getQPUs(local=False)
        qdrop(last_raised) # I don't wait cause qdrop locks the file, let's see if this raises any problem 
        qpus_after = getQPUs(local=False)
        return self.assertNotEqual(qpus_before, qpus_after), self.assertNotEqual(qpus_after[-1]._family, 'unique name 25.06.2025 goku super saiyan')
    
    def test_nothing_to_drop(self):
        qdrop()
        return self.assertRaises(SystemError, qdrop)

class Test_getQPUs(unittest.TestCase):
    """
    Class to check wether the getQPUs method functions correctly on all situations.
    """
    def __init__(self, methodName = "runTest"):
        self.jobs_to_drop = []
        super().__init__(methodName)

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.jobs_to_drop)

    def test_not_raised(self):
        qdrop()
        os.system('sleep 5')
        return self.assertRaises(SystemExit, getQPUs)
    
    def test_local_flag(self):
        self.jobs_to_drop.append(qraise(1, '00:30:00', family='Merlín e familia'))
        qpus = getQPUs(local=False)
        return self.assertEqual(qpus[-1]._family, 'Merlín e familia')
    
    def test_family(self):
        self.jobs_to_drop.append(qraise(1, '00:30:00', family='test_family'))
        self.jobs_to_drop.append(qraise(1, '00:10:00', family='not_the_same_family'))
        qqpus = getQPUs(local=False, family='test_family')

        return self.assertNotEqual(qqpus[-1]._family, 'not_the_same_family')

    # def test_QPUs_with_diff_backends(self):
    #     return

class Test_qraise(unittest.TestCase):
    """
    Class to test correct functioning of the qraise command
    """

    def __init__(self, methodName = "runTest"):
        self.jobs_to_drop =[]
        super().__init__(methodName)

    def tearDown(self):
        qdrop(self.jobs_to_drop[0]) #drops exactly the QPU we created on each test
        self.jobs_to_drop =[]
        os.system('sleep 1')



    def test_simulator_flag(self):
        self.jobs_to_drop.append(qraise(1, '00:10:00', simulator='Munich'))
        qpus=getQPUs(local=False)
        return self.assertEqual(qpus[-1].backend.__dict__["simulator"], "MunichSimulator")
    
    def test_backend_flag(self):
        # I use a simple backend that I configurated for the test
        family = qraise(1, '00:10:00', backend='/mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/CUNQA/code_tests/backend_simple.json')
        self.jobs_to_drop.append(family) 
        qqpus=getQPUs(local=False)
        # On the next line we test by name, which is not perfect but should be enough
        return self.assertEqual(qqpus[-1].backend.__dict__["name"],"Backend test test test this name is unique and won't coincide by chance")
    
    def test_fakeqmio_flag(self):
        self.jobs_to_drop.append(qraise(1, '00:10:00', fakeqmio=True))
        qpuss=getQPUs(local=False)
        return self.assertEqual(qpuss[-1].backend.__dict__["name"], "FakeQmio" )
    
    def test_family_name_flag(self):
        self.jobs_to_drop.append(qraise(1, '00:10:00', family='test_family_name'))
        qppus = getQPUs(local=False)
        return self.assertEqual(qppus[-1]._family, 'test_family_name')

    def test_family_name_unique(self):
        self.jobs_to_drop.append(qraise(1, '00:10:00', family='im unique'))
        return self.assertRaises(SystemError, qraise, 1, '00:10:00', family='im unique')

    def test_hpc_mode(self):
        # We will raise a QPU on a node different from ours (a login one) and check that we get an error if we try to run something on it
        self.jobs_to_drop.append(qraise(1,'00:10:00', cloud=False))
            
        qpus_one_hpc = getQPUs(local=False)
        return self.assertRaises(SystemError, qpus_one_hpc[-1].run, CunqaCircuit(1,1))

    # def test_communications(self):
    #     pass
    
if __name__ == "__main__":
    unittest.main(verbosity=2)