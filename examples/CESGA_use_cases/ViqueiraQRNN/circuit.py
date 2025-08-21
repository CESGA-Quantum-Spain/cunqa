"""
 Title: EMCZ circuit class
 Description: class that manages quantum circuits with the structure from https://arxiv.org/abs/2310.20671

Created 10/07/2025
@author: dexposito (algorithm idea: jdviqueira)
"""

import os, sys
import math
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit, _is_parametric
from ViqueiraQRNN.ansatz import AnsatzQRNN, EMCZ2, EMCZ3
from cunqa.logger import logger
from cunqa.qpu import QPU
from cunqa.qjob import QJob


class CircuitQRNNError(Exception):
    """Exception for error during QRNN circuit creation."""
    pass

class CircuitQRNN:
    def __init__(self, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int, ansatz: AnsatzQRNN = EMCZ2, init_state_mem: CunqaCircuit = None):
        """
        Class to manage a QRNN circuit. This circuit modifies a time series 
        to obtain another time series after executing. The default ansatz is the one from the 
        Exchange-Memory w Controlled Z-gates algorithm (https://arxiv.org/abs/2310.20671).

        Args:
            nE (int): number of qubits for the Environment/Exchange register
            nM (int): number of qubits for the Memory register
            nT (int): number of time steps of the time series
            repeat_encode (int): number of times that the encoding block should be repeated in the circuit
            repeat_evolution (int): number of times that the evolution block should be repeated in the circuit
            ansatz (int): AnsatzQRNN instance to apply on each time step
            init_state_mem (<class CunqaCircuit>): initial state for the memory register
            
        Return:
            circuit (<class CunqaCircuit>): circuit implementing the QRNN structure
        """
        self.nE = nE
        self.nM = nM
        self.nT = nT
        self._repeat_encode = repeat_encode
        self._repeat_evolution = repeat_evolution
        
        self.circuit = CunqaCircuit(nE + nM, nE*nT) # Number of cl_bits motivated by the measure of the Environment/Exchange register on each time_step


        # Determine which ansatz to use on the circuit
        if ansatz == EMCZ2:
            self.ansatz_object = EMCZ2(nE, nM, repeat_encode, repeat_evolution)
            self.ansatz = self.ansatz_object._get_full_circuit()

        elif (ansatz == "EMCZ3" or ansatz == EMCZ3):
            self.ansatz_object = EMCZ3(nE, nM, repeat_encode, repeat_evolution)
            self.ansatz = self.ansatz_object._get_full_circuit()

        else:
            if not all([ansatz.nE == self.nE, ansatz.nM == self.nM, ansatz._repeat_encode == self._repeat_encode, ansatz._repeat_evolution == self._repeat_evolution]):
                logger.error(f"Provided ansatz has incorrect dimensions, nE: {ansatz.nE} vs {self.nE} (ansatz vs circuit), nM: {ansatz.nM} vs {self.nM}, repeat_encode: {ansatz._repeat_encode} vs {self._repeat_encode} and repeat_evolution: {ansatz._repeat_evolution} vs {self._repeat_evolution}.")
                raise CircuitQRNNError
            
            self.ansatz_object = ansatz
            self.ansatz = self.ansatz_object._get_full_circuit()


        # If we have an intial state for the memory register, add it here
        if init_state_mem is not None:
            if init_state_mem.num_qubits == self.nM:

                self.init_state_mem = init_state_mem
                self.circuit += (CunqaCircuit(nE) | init_state_mem )

            else:
                logger.error(f"Initial state for the memory register has {init_state_mem.num_qubits} qubits while the memory register has {self.nM} qubits.")
                raise CircuitQRNNError



        ##################### BUILD THE CIRCUIT ##########################
        for time_step in range(nT):
            try:
                self.circuit += self.ansatz
        
            except Exception as error:
                logger.error(f"An error occurred while creating the circuit:\n {error}.")
                raise CircuitQRNNError

            self.circuit.measure([i for i in range(nE)], [time_step*nE + i for i in range(nE)])
            self.circuit.save_state(label=f"State_{time_step}") 
            self.circuit.reset([i for i in range(nE)]) 
    
        
    def parameters(self, new_x: np.array, new_theta: np.array) -> list[Union[float, int]]:
        """
        Method for combining the data from the time series and the theta parameters to update the circuit.

        Args:
            theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*repeat_encode + 2(nE + nM)*repeat_evolution + nE.
            x (numpy.array): Input data representing a time series. Its shape must be (nT, nE)

        Return:
            all_params (list[float, int]): parameters to insert on the circuit organized in the right order
        """
        if not new_x.shape() == (self.nT, self.nE):
            logger.error(f"The time series provided doesn't have the correct shape. It should be {(self.nE, self.nT)} while it is {new_x.shape()}.")
            raise CircuitQRNNError
        
        if not new_theta.shape() == (2*self.nE*self._repeat_encode + 2*(self.nE+self.nM)*self._repeat_evolution + self.nE):
            logger.error(f"The theta provided doesn't have the correct lenght. It should be {2*self.nE*self._repeat_encode + 2*(self.nE+self.nM)*self._repeat_evolution + self.nE} while it is {new_theta.shape()}.")
            raise CircuitQRNNError
        
        all_params = []
        for t in self.nT:
            all_params += self.ansatz.total_order_params(x=new_x[t, :], theta=new_theta)

        return all_params

        

    def run_on_QPU(self, QPU: QPU, **run_parameters: Any) -> QJob:
        """
        Method for running the EMC circuit on a selected QPU. 

        Args:
            QPU (class cunqa.QPU): virtual quantum processing unit where the circuit will be simulated
            **run_parameters : any other simulation instructions. For instance method="density_matrix", transpile=True, initial_layout (list with qubit layout for transpilation) 
        
        Returns:
            (class cunqa.QJob): object with the quantum simulation job. Results can be obtained doing QJob.result
        """
        try:
            qjob = QPU.run(self, **run_parameters)

        except Exception as e:
            logger.error(f"Error while running the EMCZ circuit on a QPU:\n {e}")
            raise CircuitQRNNError
        
        return qjob
          

