import unittest
from tesst_transpile import TestTranspErrors
from tesst_circuit import TestCircuitConversion
from tesst_qjob import TestResult, TestQJob, TestGather
from tesst_qpu_and_backend import TestBackend, TestQPU, Test_getQPUs
from tesst_qraise_qdrop import Test_qraise_qdrop

if __name__ == "__main__":
    # Create a TestLoader instance
    loader = unittest.TestLoader()

    # Create a TestSuite
    suite = unittest.TestSuite()

    # Add test cases to the suite using the same loader
    suite.addTests(loader.loadTestsFromTestCase(Test_getQPUs)) #I run this test first to try to avoid encountering error w getQPUs reading dropped QPUs
    suite.addTests(loader.loadTestsFromTestCase(TestTranspErrors))
    suite.addTests(loader.loadTestsFromTestCase(TestCircuitConversion))
    suite.addTests(loader.loadTestsFromTestCase(TestResult))
    suite.addTests(loader.loadTestsFromTestCase(TestQJob))
    suite.addTests(loader.loadTestsFromTestCase(TestGather))
    suite.addTests(loader.loadTestsFromTestCase(TestBackend))
    suite.addTests(loader.loadTestsFromTestCase(TestQPU))
    suite.addTests(loader.loadTestsFromTestCase(Test_qraise_qdrop))

    # Run the tests
    runner = unittest.TextTestRunner()
    runner.run(suite)

