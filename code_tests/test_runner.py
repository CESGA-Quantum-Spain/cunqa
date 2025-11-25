import os
import sys

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import unittest
from code_tests.test_qjob import TestQJob
from code_tests.test_qpu import TestQPU
from code_tests.test_transpile import TestTranspErrors
from test_qutils import Test_qraise, Test_qdrop, Test_getQPUs



if __name__ == "__main__":
    # Create a TestLoader instance
    loader = unittest.TestLoader()

    # Create a TestSuite
    suite = unittest.TestSuite()

    # Add test cases to the suite using the same loader
    suite.addTests(loader.loadTestsFromTestCase(Test_getQPUs)) #I run this test first to try to avoid encountering error w getQPUs reading dropped QPUs
    suite.addTests(loader.loadTestsFromTestCase(TestTranspErrors))
    suite.addTests(loader.loadTestsFromTestCase(TestQJob))
    suite.addTests(loader.loadTestsFromTestCase(TestQPU))
    suite.addTests(loader.loadTestsFromTestCase(Test_qraise))
    suite.addTests(loader.loadTestsFromTestCase(Test_qdrop))

    # Run the tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    # Optional: Exit with non-zero status if tests fail
    # sys.exit(not result.wasSuccessful())

