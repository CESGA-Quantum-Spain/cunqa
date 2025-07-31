import os
import sys
import unittest
import subprocess


# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class Test_compiles(unittest.TestCase):
    """
    Class to test if the code compiles without crashing.
    """

    def test_does_it_compile(self): 
        try:
            # cpu_count = os.cpu_count() or 1
            subprocess.run('ninja -C /mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/CUNQA/build_test -j $(nproc)', shell=True, check=True, capture_output=True, text=True) # Check=true raises exception if error appears

        except subprocess.CalledProcessError as e:
            self.fail(f"Compilation failed") # . Error output: {e.stderr}

    
        

if __name__ == "__main__":
    unittest.main(verbosity=2)