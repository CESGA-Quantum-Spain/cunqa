"""
 Title: EMCZ main class
 Description: definition of a class implementing the EMCZ QRNN model from https://arxiv.org/abs/2310.20671

Created 11/07/2025
@author: dexposito (algorithm idea: jdviqueira)
"""

import os, sys
import math
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional
from random import randint

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from ViqueiraQRNN.ansatz import AnsatzQRNN, EMCZ2
from ViqueiraQRNN.circuit import CircuitQRNN
from ViqueiraQRNN.gradients_and_costs import GradientMethod, CostFunction
from cunqa.logger import logger
from cunqa.qjob import QJob
from cunqa.circuit import CunqaCircuit


class ViqueiraQRNN:
    """
    Implementation using CUNQA of the QRNN Exchange-Memory with Controlled Z-gates model from the paper https://arxiv.org/abs/2310.20671 .
    """

    def __init__(self, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int, ansatz: AnsatzQRNN = EMCZ2, init_state_mem: CunqaCircuit = None):

        # Run a bash script raising QPUs in six empty nodes (should amount to 192 QPUs). Command waits until jobs are finished configuring
        try:
            command = 'source raise_QPUs_idle_nodes.sh'
            subprocess.run(command, shell=True, check=True, capture_output=True, text=True) 

        except subprocess.CalledProcessError as error:
            logger.error(f"Error while raising QPUs:\n {error.stderr}.")
            raise SystemExit

        self.nE = nE
        self.nM = nM
        self.nT = nT
        self._trained = False

        self.circuit = CircuitQRNN(nE, nM, nT, repeat_encode, repeat_evolution, ansatz, init_state_mem)
    

    ########################## CIRCUIT CALCULATION METHODS #####################

    # def bind_parameters(self, qjob: QJob, x: np.array, theta: np.array) -> QJob:
    #     return qjob.upgrade_parameters(x=x, theta=theta)

    # def calc_observable(self, qjob, observable = None) -> np.array:
    #     # TODO: improve this method to a flexible one once we have an observable calculation pipeline
    #     probs = qjob.result.probabilities(per_qubit = True, partial = list(range(self.nE)))

    #     return np.array([prob_qubit[0]-prob_qubit[1] for prob_qubit in probs]) # ad hoc calculation of <Z> observable
    
    # def evaluate_circ(self, qjob, x, theta) -> np.array:
    #     job = self.bind_parameters(qjob, x, theta)
    #     obs_result = self.calc_observable(job)
    #     return obs_result

    ########################## FIT, PREDICT AND VALIDATE MODEL #########################

    def fit(self, population: list[np.array], y_labels: list[np.array], theta_init: np.array = None, learn_rate: float = 1e-3, epochs: int = 2000, gradient_method: Optional[str] = "finite_differences", cost_func: Optional[str] = "rmse", shots: int = 2000):
        """
        Method for training the theta parameters of the EMCZ recursive neural network. Uses the gradient method chosen by the user, parallelizing between 
        different QPUs using CUNQA.

        Args:
            population (list[np.array]): multiple time series data that will be used for training
            y_labels (list[np.array]): correct results of each of the time series needed for training
            theta_init (np.array): initial values of the parameters theta to be trained. If no theta is given the optimization will be initialized with a random one
            gradient_method (str): method to be used for calculating the gradient on each iteration of the gradient descent. Default: finite_differnces
            cost_func (str): method to be used for calculating the loss. Default: rmse
            learn_rate (float): factor that multiplies the gradient on each optimization step and determines how fast or how accurately the algorithm converges
            stop_criteria (float): describes how low the error should be before stopping the optimization. Default: 1e-5
        """

        self.calc_gradient = GradientMethod(gradient_method) # By default finite_differences
        self.calc_cost = CostFunction(cost_func) # By default RMSE

        if theta_init == None:
            np.random.seed(18) # Set a seed for debugging
            theta_aux = 2.*np.pi * np.random.random(2*self.nE*self.repeat_encode + 2*(self.nE + self.nM)*self.repeat_evolution + self.nE)
        else:
            theta_aux = theta_init


        logfile = "" # Change for the actual file name
        with open(logfile, 'a') as f:

            for epoch in epochs:
                best_loss = float("Infinity") # Best loss will be inmediatly updated without writing a special case for i=0
                new_result = self.calc_gradient.init_evaluation(self.circuit, time_series= population[0], theta = theta_aux, shots = shots) #initialize new result to be used always as reference internally on the 
                for i, time_series in enumerate(population):

                    gradient = self.calc_gradient(circuit=self.circuit, time_series=time_series, theta_now=theta_aux,  y_true=y_labels[i], cost_func=self.calc_cost, shots=shots)
                    theta_aux += learn_rate * gradient
                    
                    new_result = self.calc_gradient.distr_shots(time_series, theta_aux)          

                    loss_i = self.calc_cost(new_result, y_labels[i])
                    if loss_i < best_loss:
                        best_loss = loss_i
                        self.theta = theta_aux 


                f.write(f"==> THETA after epoch {epoch:4d}:  {self.theta:10.6f} \n")
                f.write(f"==> LOSS ({self.calc_cost.choice_function}) after epoch {epoch:4d}:  {loss_i:10.6f} \n")

        self._trained = True   
        logger.debug(f"Optimal theta found: {self.theta}.")


    def predict(self, new_time_series: np.array) -> np.array:
        """
        Upon receiving a new time series, we evolve it using the EMCZ circuit with the optimal calculated theta and return the predictions.

        Args:
            new_time_series (np.array): (nT, nE)-array with the information of a time series, preferrably different from the ones used to train. 
        
        Return:
            (np.array): (nT, nE)-array with the result of running new_time_series through the circuit with the trained theta.
        """
        if not self._trained:
            logger.error("Model should be trained before trying to make predictions")
            raise SystemExit
        
        return self.calc_gradient.distr_shots(new_time_series, self.theta)



    def validate(self, new_population: Union[list[np.array], np.array], y_new: Union[list[np.array], np.array], cost_func: Optional[str]) -> float:
        """
        Method for obtaining the error on new time series with a given cost function and the true labels.

        Args:
            new_time_series (np.array):
            y_new (np.array):
            cost_func (str):
        Return:
            (float): 
        """
        if not self._trained:
            logger.error("Model should be trained before trying to evaluate its predictions")
            raise SystemExit
        
        if cost_func != self.calc_cost.choice_function:
            self.calc_cost.update_cost_function(cost_func)

        if (isinstance(new_population, list) and isinstance(y_new, list)):
            if len(new_population) != len(y_new):
                logger.error("Lenght of the lists of time series and labels do not match")
                raise SystemExit
            
            return [cost_func(self.predict(new_population[i]), y_new[i]) for i in range(len(y_new))] # TODO: Parallelize this 
        
        elif (isinstance(new_population, np.array) and isinstance(y_new, np.array)): # Here the arrays should be of shape (nT,nE) and (nT), mayeb add checks later
            return cost_func(self.predict(new_population), y_new)


        
        