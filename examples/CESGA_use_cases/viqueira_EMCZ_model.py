"""
 Title: EMCZ main class
 Description: class implementing the EMCZ QRNN algorithm from https://arxiv.org/abs/2310.20671

Created 11/07/2025
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
from viqueira_gradients_and_costs import GrandientMethod, CostFunction
from cunqa.logger import logger
from cunqa.qutils import getQPUs, qraise, qdrop, QRaiseError
from cunqa.mappers import run_distributed
from cunqa.qjob import QJob, gather

class ViqueiraEMCZModel:
    """
    Implementation using CUNQA of the Exchange-Memory with Controlled Z-gates model from the paper https://arxiv.org/abs/2310.20671 .
    """

    def __init__(self, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int, shots: Optional[int] = 1000, rseed: Optional[int] = None):

        # Run a bash script raising QPUs in six empty nodes (should amount to 192 QPUs)
        try:
            command = 'source /mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/cunqa_QRNN_side_project/examples/CESGA_use_cases/raise_QPUs_idle_nodes.sh'
            subprocess.run(command, shell=True, check=True, capture_output=True, text=True) 

        except subprocess.CalledProcessError as error:
            logger.error(f"Error while raising QPUs:\n {error.stderr}.")
            raise SystemExit

        self.nE = nE
        self.nM = nM
        self.nT = nT
        self._trained = False

        self.circuit = CircuitEMCZ(nE, nM, nT, repeat_encode, repeat_evolution)

        self.qpus=getQPUs(local=False)# TODO: add the waiting til QPUs are raised for the getQPUs. Problem: diff qraises and maybe sleeping and awake nodes
        self.qjobs = list(map(lambda x: self.circuit.run_on_QPU(x,shots=shots), self.qpus)) # Submits the circuit structure with parameters zero on all QPUs


    def train(self, population: list[np.array], theta_init: np.array, gradient_method: Optional[str] = "finite_diferences", stop_criteria: float = 1e-5):
        """
        Method for training the theta parameters of the EMCZ recursive neural network. Uses the gradient method chosen by the user, parallelizing between 
        different QPUs using CUNQA.
        """
        calc_gradient = GrandientMethod(gradient_method)
        calc_cost = CostFunction() # By default it's RMSE

        


        self._trained = True
        pass

    def predict(self, new_time_series: np.array) -> np.array:
        """
        Upon receiving a new time series, we evolve it using the EMCZ circuit with the optimal calculated theta and return the predictions.
        """
        if not self._trained:
            logger.error("Model should be trained before trying to make predictions")
            raise SystemExit
        
        pass

    def evaluate(self, new_time_series, y_new, cost_func):
        """
        Method for obtaining the error on new time series with a given cost function and the true labels
        """
        if not self._trained:
            logger.error("Model should be trained before trying to evaluate its predictions")
            raise SystemExit
        
        if (isinstance(new_time_series, list) and isinstance(y_new, list)):
            if len(new_time_series) != len(y_new):
                logger.error("Lenght of the lists of time series and labels do not match")
                raise SystemExit
            
            return [cost_func(self.predict(new_time_series[i]), y_new[i]) for i in range(len(y_new))]
        
        elif (isinstance(new_time_series, np.array) and isinstance(y_new, np.array)): # Here the arrays should be of shape (nT,nE) and (nT), mayeb add checks later
            return cost_func(self.predict(new_time_series), y_new)


        
        