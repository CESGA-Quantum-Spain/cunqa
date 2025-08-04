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
from viqueira_QRNN_circuit import CircuitQRNN
from viqueira_QRNN_model import ViqueiraQRNN
from cunqa.logger import logger
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
            self.deriv = self.rmse_deriv

        # Add more cost functions as needed

        else:
            logger.error(f"Chosen cost function is not supported: {choose_function}.")
            raise CostFunctionError

    #################### COST FUNCTIONS ####################

    def rmse(self, prediction, y_true):
        """
        Root mean squared error.
        """
        if len(prediction) != len(y_true):
            logger.error("Lenghts of predictions and the true labels do not match.")
            raise CostFunctionError

        return math.sqrt(sum([(prediction[i] - y_true[i])**2 for i in range(len(y_true))])/len(y_true)) # if they are np.arrays the for can be eliminated
    
    #################### DERIVATIVES ####################

    def rmse_deriv(self, prediction, y_true):
        rmse = self.rmse(prediction, y_true)

        return 2*sum([prediction[i]-y_true[i] for i in range(len(y_true))])/(len(y_true)*2*rmse)
    
    #################### INTERFACE METHODS ####################

    def update_cost_function(self, new_cost_function):
        logger.debug(f"Changing cost function from {self.choice_function} to {new_cost_function}")
        self.__init__(new_cost_function)

    def __call__(self, *args, **kwds):
        return self.function(*args, **kwds)
    




 


class GradientMethodError(Exception):
    """Exception for signaling errors during gradient calculations."""
    pass

class GradientMethod:
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

    #################### GRADIENTS ####################

    def finite_differences(self, circuit: CircuitQRNN, qjobs: list[QJob], time_series: np.array, theta_now: np.array, y_true: np.array, cost_func: CostFunction, diff: Optional[float] = 1e-7):
        """
        Finite differences method for calculating the gradient. It estimates the derivative on 
        each component of the gradient, parallelizing the calculation between QPUs.

        Args:
            circuit (<class CircuitEMCZ>): the gradient of the parameters of this circuit will be computed. It iseeded for the .parameters() method that creates the right parameter order
            qjobs (list[<class cunqa.qjob>]): qjob objects representing the QPUs to which we submitted circuit. Use .upgrade_parameters() on them to input the desired parameters
            time_series (np.array): information of the time series being considered
            theta_now (np.array): initial value of theta at which to calculate the gradient 
            y_true (np.array): true label that circuit should produce from time_series
            cost_func (<class CostFunction>): function that calculates the loss 
            diff (float): small difference to put on each coordinate to calculate derivatives

        Return:
            gradient (np.array): array of lenght len(theta_now) with the estimated gradient of theta
        """
        n = len(theta_now)
        n_qjobs = len(qjobs)
        gradient = [0.0 for _ in range(n)]

        # We will traverse the components of theta n_qjobs elements at a time
        # on a loop. First we go through the last n % n_qjobs objects and 
        # WE OPTIMIZE BY ADDING THE NON-PERTURBED CIRCUIT ON THIS BATCH (it will be our reference)
        final_start = n-(n % n_qjobs)
        final_results = gather(
            [self.perturbed_i_circ(qjobs[i], circuit, time_series, theta_now, final_start + i, diff) for i in range(n % n_qjobs)] 
            + [qjobs[-1].upgrade_parameters(circuit.parameters(time_series, theta_now))] 
            )

        reference = final_results.pop().probabilities
        final_deriv = [ (cost_func(reference, y_true) - cost_func(res.probabilities, y_true))/diff for res in final_results]
        gradient[final_start:n] = final_deriv
        
        # We go through the components of theta n_qjobs at a time 
        for i in range(n // n_qjobs):

            # Range of components for which we calculate the derivative on this loop iteration
            start = i*n_qjobs
            end = (i+1)*n_qjobs

            # Concurrent execution of circuits with small differences on one component
            results = gather( [self.perturbed_i_circ(qjob, circuit, time_series, theta_now, start + qjobs.index(qjob), diff) for qjob in qjobs] )
            deriv = [ (cost_func(reference, y_true) - cost_func(res.probabilities, y_true))/diff for res in results]
            gradient[start:end] = deriv

        return np.array(gradient)


    def parameter_shift_rule(self, circuit: CircuitQRNN, qjobs: list[QJob], time_series: np.array, theta_now: np.array, y_true: np.array, cost_func: CostFunction):
        """
        Parameter shift rule method for estimating the gradient of continuous parameters of quantum circuits.
        Based on the formula grad_{theta}f(x; theta) = [f(x; theta + pi/2) + f(x; theta - pi/2)]/2 for quantum functions.

        Args:
            circuit (<class CircuitEMCZ>): the gradient of the parameters of this circuit will be computed. It iseeded for the .parameters() method that creates the right parameter order
            qjobs (list[<class cunqa.qjob>]): qjob objects representing the QPUs to which we submitted circuit. Use .upgrade_parameters() on them to input the desired parameters
            time_series (np.array): information of the time series being considered
            theta_now (np.array): initial value of theta at which to calculate the gradient 
            y_true (np.array): true label that circuit should produce from time_series
            cost_func (<class CostFunction>): function that calculates the loss 

        Return:
            gradient (np.array): array of lenght len(theta_now) with the estimated gradient of theta
        """
        n = len(theta_now)
        n_qjobs = len(qjobs)
        gradient = [0.0 for _ in range(n)]

        # We go through the components of theta n_qjobs at a time 
        for i in range(n // n_qjobs):

            # Range of components for which we calculate the derivative on this loop iteration
            start = i*n_qjobs
            end = (i+1)*n_qjobs

            # Concurrent execution of circuits with the parameter shifted +-pi/2 on one component
            results = gather([self.shifted_i_circ(qjob, circuit, time_series, theta_now, start + qjobs.index(qjob)) for qjob in qjobs])
            deriv = [(cost_func(plus.probabilities, y_true) + cost_func(minus.probabilities, y_true))/2 for plus, minus in zip(results[0::2], results[1::2])]

            gradient[start:end] = deriv
        
        # Last remaining n % n_qjobs components
        final_start = n-(n % n_qjobs)
        final_results = gather( [self.shifted_i_circ(qjobs[i], circuit, time_series, theta_now, final_start + i) for i in range(n % n_qjobs)] )

        final_deriv = [ (cost_func(plus.probabilities, y_true) + cost_func(minus.probabilities, y_true))/2 for plus, minus in zip(final_results[0::2], final_results[1::2])] # the zip goes through even and odd elements together
        gradient[final_start:n] = final_deriv

        return np.array(gradient)

    def gradient_with_bias(self):
        """ """
        pass

    #################### INTERFACE METHODS ####################

    def update_gradient_method(self, new_gradient_method):
        logger.debug(f"Changing gradient method from {self.choice_method} to {new_gradient_method}")
        self.__init__(new_gradient_method)

    def __call__(self, *args, **kwds):
        return self.method(*args, **kwds)

    #################### AUXILIARY METHODS FOR THE GRADIENTS ####################

    def perturbed_i_circ(qjob: QJob, circuit : CircuitQRNN, time_series: np.array, theta: np.array, index: int, diff: float):
        theta_aux = theta; theta_aux[index] += diff
        return qjob.upgrade_parameters(circuit.parameters(time_series, theta_aux))
    
    def shifted_i_circ(qjob: QJob, circuit : CircuitQRNN, time_series: np.array, theta: np.array, index: int, diff: float):
        theta_plus = theta; theta_plus[index] += np.pi/2
        theta_minus = theta; theta_minus[index] -= np.pi/2

        qjob_plus = qjob.upgrade_parameters(circuit.parameters(time_series, theta_plus))
        qjob_minus = qjob.upgrade_parameters(circuit.parameters(time_series, theta_minus))
        return qjob_plus, qjob_minus

