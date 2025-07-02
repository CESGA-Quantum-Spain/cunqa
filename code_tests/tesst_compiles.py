import os
import sys
import unittest


# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class Test_compiles(unittest.TestCase):
    """
    Class to test if the code compiles without crashing.
    """

    def test_does_it_compile(self):
        error_occurred = False 
        try:
            os.system('ninja -C build_test -j $(nproc)')
        except:
            error_occurred = True

        # Avoids the giant traceback that would muddy other tests. Otherwise we could let the error unhandled, the test would appear as ERROR and the traceback would be printed
        self.assertFalse(error_occurred, 'There was an error during compilation.')

    # def test_submodules_updated(self):
    #     pass
        

if __name__ == "__main__":
    unittest.main(verbosity=2)