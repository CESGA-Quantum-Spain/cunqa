"""
 Title: Gradients and Cost Functions
 Description: class implementing all gradient calculation methods + cost functions of interest

Created 15/07/2025
@author: dexposito (algorithm idea: jdviqueira)
"""

import os, sys
import math
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from viqueira_EMCZ_circuit import CircuitEMCZ
from cunqa.logger import logger
from cunqa.qutils import getQPUs, qraise, qdrop, QRaiseError
from cunqa.mappers import run_distributed
from cunqa.qjob import QJob, gather

class GradientMethodError(Exception):
    """Exception for signaling errors during gradient calculations."""
    pass

class GrandientMethod:
    """ Callable class that handles all methods to calculate the gradient for the EMCZ algorithm. """
    def __init__(self, choose_method: str = "finite_differences"):
        """
        Args:
            choose_method (str): string describing the method that should be used. Default: finite_differences
        """
        self.choose_method = choose_method

        if choose_method in ["finite_differences", "fd"]:
            self.method = self.finite_differences

        elif choose_method in ["parameter_shift_rule", "psr"]:
            self.method = self.parameter_shift_rule

        elif choose_method in ["gradient_with_bias", "bias"]:
            self.method = self.gradient_with_bias
        
        # Add any desired gradient methods :-)

        else:
            logger.error(f"Chosen gradient method is not supported: {choose_method}.")
            raise GradientMethodError

    def finite_differences(self):
        pass

    def parameter_shift_rule(self):
        pass

    def gradient_with_bias(self):
        pass

    def update_gradient_method(self, new_gradient_method):
        self.__init__(new_gradient_method)

    def __call__(self, *args, **kwds):
        return self.method(*args, **kwds)





class CostFunctionError(Exception):
    """Exception for error during cost calculations."""
    pass

class CostFunction:
    """ Callable class that handles all cost functions for the EMCZ algorithm. """
    def __init__(self, choose_function: str = "rmse"):
        
        self.choose_function = choose_function

        if choose_function == "rmse":
            self.function = self.rmse

        # Add more cost functions as needed

        else:
            logger.error(f"Chosen cost function is not supported: {choose_function}.")
            raise CostFunctionError

    def rmse(self, prediction, y_true):
        """
        Root mean squared error.
        """
        if len(prediction) != len(y_true):
            logger.error("Lenghts of predictions and the true labels do not match.")
            raise CostFunctionError

        return math.sqrt(sum([(prediction[i] - y_true[i])**2 for i in range(len(y_true))]))
    
    def update_cost_function(self, new_cost_function):
        self.__init__(new_cost_function)

    def __call__(self, *args, **kwds):
        return self.function(*args, **kwds)