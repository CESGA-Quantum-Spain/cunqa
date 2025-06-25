import unittest
from tesst_transpile import TestTranspErrors
from tesst_circuit import TestCircuitConversion
from tesst_qjob import TestQJob, TestGather
from tesst_qpu import TestQPU
from tesst_qutils import Test_qraise, Test_qdrop, Test_getQPUs
from tesst_result_and_backend import TestBackend, TestResult
from tesst_compiles import Test_compiles

if __name__ == "__main__":
    # Create a TestLoader instance
    loader = unittest.TestLoader()

    # Create a TestSuite
    suite = unittest.TestSuite()

    # Add test cases to the suite using the same loader
    suite.addTests(loader.loadTestsFromTestCase(TestTranspErrors))
    suite.addTests(loader.loadTestsFromTestCase(TestCircuitConversion))
    suite.addTests(loader.loadTestsFromTestCase(TestResult))
    suite.addTests(loader.loadTestsFromTestCase(TestQJob))
    suite.addTests(loader.loadTestsFromTestCase(TestGather))
    suite.addTests(loader.loadTestsFromTestCase(TestBackend))
    suite.addTests(loader.loadTestsFromTestCase(Test_compiles))

    # Run the tests
    runner = unittest.TextTestRunner()
    runner.run(suite)

