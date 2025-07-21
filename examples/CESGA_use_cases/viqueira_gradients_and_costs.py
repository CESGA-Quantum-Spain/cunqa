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
from viqueira_EMCZ_model import ViqueiraEMCZModel
from cunqa.logger import logger
from cunqa.qutils import getQPUs, qraise, qdrop, QRaiseError
from cunqa.mappers import run_distributed
from cunqa.qjob import QJob, gather

class CostFunctionError(Exception):
    """Exception for error during cost calculations."""
    pass

class CostFunction:
    """ Callable class that handles all cost functions for the EMCZ algorithm. """
    def __init__(self, choose_function: str = "rmse"):
        
        self.choice_function = choose_function

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

        return math.sqrt(sum([(prediction[i] - y_true[i])**2 for i in range(len(y_true))])/len(y_true)) # if they are np.arrays the for can be eliminated
    
    def update_cost_function(self, new_cost_function):
        logger.debug(f"Changing cost function from {self.choice_function} to {new_cost_function}")
        self.__init__(new_cost_function)

    def __call__(self, *args, **kwds):
        return self.function(*args, **kwds)
    







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
        self.choice_method = choose_method

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

    def finite_differences(self, model: ViqueiraEMCZModel, qjobs: list[QJob], time_series: np.array, theta_now: np.array, y_true: np.array, cost_func: CostFunction, diff: Optional[float] = 1e-7):
        """
        Finite differences method for calculating the gradient. It estimates the derivative on 
        each component of the gradient, parallelizing the calculation between QPUs.

        Args:
            qjobs (list[<class cunqa.qjob>]): 
            time_series (np.array):
            theta_now (np.array):
            y_true (np.array):
            cost_func (class CostFunction):
            diff (float):

        Return:
            gradient (np.array):
            """
        n = len(theta_now)
        n_qjobs = len(qjobs)
        gradient = [0.0 for _ in len(theta_now)]

        # We will traverse the components of theta n_qjobs elements at a time
        # on a loop. First we go through the last n % n_qjobs objects and 
        # WE OPTIMIZE BY ADDING THE NON-PERTURBED CIRCUIT ON THIS BATCH (it will be our reference)
        separate_start = n-(n % n_qjobs)
        separate_results = gather(
            [self.perturbed_circ_result(qjobs[i], model, time_series, theta_now, separate_start + i, diff) for i in range(n % n_qjobs)] 
            + [qjobs[-1].upgrade_parameters(model.circuit.parameters(time_series, theta_now))] 
            )

        reference = separate_results.pop().probabilities
        separate_deriv = list(map(lambda x: (cost_func(reference, y_true) - cost_func(x.probabilities, y_true))/diff, separate_results))
        gradient[separate_start:n] = separate_deriv
        
        # We go through the components of theta n_qjobs at a time (the key use of the index is inside gather statement)
        for i in range(n // n_qjobs):

            # Range of components for which we calculate the derivative on this loop iteration
            start = i*n_qjobs
            end = (i+1)*n_qjobs

            # Concurrent execution of circuits with small differences on one component
            results = gather([self.perturbed_circ_result(qjob, model, time_series, theta_now, start + qjobs.index(qjob), diff) for qjob in qjobs])
            deriv = list(map(lambda x: (cost_func(reference, y_true) - cost_func(x.probabilities, y_true))/diff, results))
            gradient[start:end] = deriv

        return np.array(gradient)

    def parameter_shift_rule(self):
        """ """
        pass

    def gradient_with_bias(self):
        """ """
        pass

    def perturbed_circ_result(qjob: QJob, model : ViqueiraEMCZModel, time_series: np.array, theta: np.array, index: int, diff: float):
        theta_aux = theta; theta_aux[index] += diff
        return qjob.upgrade_parameters(model.circuit.parameters(time_series, theta_aux))

    def update_gradient_method(self, new_gradient_method):
        logger.debug(f"Changing gradient method from {self.choice_method} to {new_gradient_method}")
        self.__init__(new_gradient_method)

    def __call__(self, *args, **kwds):
        return self.method(*args, **kwds)
