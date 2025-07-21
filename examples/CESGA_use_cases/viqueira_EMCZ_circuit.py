"""
 Title: EMCZ circuit class
 Description: class that creates and manages the circuit with the structure froms https://arxiv.org/abs/2310.20671

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

from cunqa.circuit import CunqaCircuit
from cunqa.logger import logger
from cunqa.qpu import QPU
from cunqa.qjob import QJob


class CircuitEMCError(Exception):
    """Exception for error during EMC circuit creation."""
    pass


def add_EMCZ_ansatz(circuit: CunqaCircuit, nE: int, nM: int, x: np.array, theta: np.array, repeat_encode: int, repeat_evolution:int , time_step: int):
    """
    Function that adds to a circuit the ansatz for the EMC QRNN algorithm, see https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png for a picture.

    Args:
        circuit (CunqaCircuit): circuit to which the ansatz will be added. Must have nE + nM qubits.
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*R + 2(nE + nM)*L + nE.
        x (numpy.array): Input data. Its shape must be (nT, nE), where this is the number of qubits on each register of the EMC QRNN circuit (function below).
        repeat_encode (int): number of times that the encoding block should be repeated.
        repeat_evolution (int): number of times that the evolution block should be repeated.
        time_step (int): indicates in which point of the time series we're on, ie the row of x that should be used.

    """
    assert np.shape(x)[1] == nE, "Error: The data x should have nE columns." 

    group1 = 2*nE*repeat_encode
    group2 = 2*(nE+nM)*repeat_evolution
    group3 = 1*nE
    total_lenght = group1+group2+group3

    # Check correct format of theta
    if len(theta)==total_lenght:
        pass
    else:
        logger.error(f"Theta must have lenght equal to {total_lenght} but a lenght {len(theta)} vector was provided.")
        raise  CircuitEMCError
    
    # Slice theta into the parameters that will be used for the encoding part (orange on the image),
    # the evolution part (blue on the image) and the final evolution part (white on the image).
    # Image, again: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png
    orange = theta[:group1]
    blue = theta[group1:group1+group2]
    white = theta[group1+group2:]

    # Encoding block
    for i in range(repeat_encode):
        for qubit in range(nE):
            circuit.ry(x[time_step][qubit], qubit)
            circuit.rx(orange[2*i*nE + 2*qubit], qubit)
            circuit.rz(orange[2*i*nE + 2*qubit + 1], qubit)
    
    # Non-theta dependent part of the encoding
    for qubit in range(nE):
        circuit.ry(x[time_step][qubit], qubit)

    # Evolution block
    for j in range(repeat_evolution):
        for qubit in range(nE+nM-1): # Last qubit missing

            circuit.rx(blue[2*j*(nE+nM-1) + 2*qubit], qubit)
            circuit.rz(blue[2*j*(nE+nM-1) + 2*qubit + 1], qubit)
            circuit.cz(qubit, qubit + 1)

        # I separate the last iteration beacause it doesn't have a CZ with the next qubit
        circuit.rx(blue[(2*j+1)*(nE+nM-1)], nE + nM - 1)
        circuit.rz(blue[(2*j+1)*(nE+nM-1) + 1], nE + nM - 1)

    # Final part of the evolution
    for qubit in range(nE):
        circuit.rx(white[qubit], qubit)
        



    
    
# The next class is non-stateful to not mess with parallelization
class CircuitEMCZ:
    """
    Function to create a EMCZ QRNN circuit. This circuit modifies a time series (sequence of data where each point depends on the ones before it)
    to obtain another time series after executing. Doing this recursively constitutes the Exchange-Memory w Controlled Z-gates algorithm (https://arxiv.org/abs/2310.20671).

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        nT (int): number of time steps of the time series
        repeat_encode (int): number of times that the encoding block should be repeated in the circuit
        repeat_evolution (int): number of times that the evolution block should be repeated in the circuit
    Return:
        circuit (CunqaCircuit): circuit implementing the QRNN structure
    """
    # Later on we could accept another argument which determines the initial state of the Memory register
    def __init__(self, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int):

        self.nE = nE
        self.nM = nM
        self.nT = nT

        self._repeat_encode = repeat_encode
        self._repeat_evolution = repeat_evolution
        x_init = np.zeros((nT,nE)) # Initialize the parameters to zero as placeholders
        theta_init = np.zeros(2*nE*repeat_encode + 2*(nE+nM)*repeat_evolution + 1*nE)
        
        self.circuit = CunqaCircuit(nE + nM, nE*nT) # Number of cl_bits motivated by the measure of the Environment/Exchange register on each time_step

        for time_step in range(nT):
            try:
                add_EMCZ_ansatz(self.circuit, nE, nM, x_init, theta_init, repeat_encode, repeat_evolution, time_step)
            except Exception as error:
                logger.error(f"An error occurred while creating the circuit [{error.__name__}].")
                raise CircuitEMCError

            self.circuit.measure([i for i in range(nE)], [time_step*nE + i for i in range(nE)])
            self.circuit.reset([i for i in range(nE)]) # This instruction needs to be implemented hehe        
        
    def parameters(self, new_x: np.array, new_theta: np.array) -> list:
        """
        Method for combining the data from the time series and the theta parameters to update the circuit.

        Args:
            theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*repeat_encode + 2(nE + nM)*repeat_evolution + nE.
            x (numpy.array): Input data representing a time series. Its shape must be (nT, nE)

        Return:
            all_params (list[float]): parameters to insert on the circuit organized in the right order
        """

        # Join all parameters of the circuit on a list with the right order for the upgrade_params method
        all_params = []
        for t in range(self):
            for i in range(self._repeat_encode):
                all_params += new_x[t,:]
                all_params += new_theta[i*2*self.nE : (i+1)*2*self.nE]
            all_params += new_x[t,:]
            for j in range(self._repeat_evolution):
                all_params += new_theta[j*2*(self.nE+self.nM) : (j+1)*2*(self.nE+self.nM)]
            all_params += new_theta[2*self.nE*self._repeat_encode + 2*(self.nE+self.nM)*self._repeat_evolution:]

        return all_params

    def run_on_QPU(self, QPU: QPU, **run_parameters: Any) -> QJob:
        """
        Method for running the EMC circuit on a selected QPU. 

        Args:
            QPU (class cunqa.QPU): virtual quantum processing unit where the circuit will be simulated
            **run_parameters : any other simulation instructions. For instance transpile (bool), initial_layout (list with qubit layout for transpilation) 
        
        Returns:
            (class cunqa.QJob): object with the quantum simulation job. Results can be obtained doing QJob.result
        """
        try:
            qjob = QPU.run(self, **run_parameters)

        except Exception as e:
            logger.error(f"Error while running the EMCZ circuit on a QPU:\n {e}")
            raise CircuitEMCError
        
        return qjob
          

