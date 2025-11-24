import os
import sys

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import unittest
from tesst_circuit import TestCircuitConversion
from tesst_qjob import TestGather
from tesst_result_and_backend import TestBackend, TestResult
from tesst_compiles import Test_compiles



if __name__ == "__main__":
    # Create a TestLoader instance
    loader = unittest.TestLoader()

    # Create a TestSuite
    suite = unittest.TestSuite()

    # Add test cases to the suite using the same loader
    suite.addTests(loader.loadTestsFromTestCase(TestCircuitConversion))
    suite.addTests(loader.loadTestsFromTestCase(TestResult))
    suite.addTests(loader.loadTestsFromTestCase(TestGather))
    suite.addTests(loader.loadTestsFromTestCase(TestBackend))
    suite.addTests(loader.loadTestsFromTestCase(Test_compiles))

    # Run the tests
    runner = unittest.TextTestRunner()
    runner.run(suite)

