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

from cunqa.circuit import CunqaCircuit
from cunqa.logger import logger
from cunqa.qpu import QPU
from cunqa.qjob import QJob


class CircuitQRNNError(Exception):
    """Exception for error during EMC circuit creation."""
    pass


def EMCZ2_encoder(nE: int, nM: int, x: np.array, theta: np.array, repeat_encode: int, repeat_evolution: int, time_step: int) -> CunqaCircuit:
    """
    Creates the encoder block circuit from the EMCZ QRNN algorithm, see https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png for a picture.

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*R + 2(nE + nM)*L + nE.
        x (numpy.array): Input data. Its shape must be (nT, nE), where this is the number of qubits on each register of the EMC QRNN circuit (function below).
        repeat_encode (int): number of times that the encoding block should be repeated.
        repeat_evolution (int): number of times that the evolution block should be repeated.
        time_step (int): indicates in which point of the time series we're on, ie the row of x that should be used.

    """
    assert np.shape(x)[1] == nE, "Error: The data x should have nE columns." 

    encoder = CunqaCircuit(nE)

    group1 = 2*nE*repeat_encode
    group2 = 2*(nE+nM)*repeat_evolution
    group3 = 1*nE
    total_lenght = group1+group2+group3

    # Check correct format of theta
    if len(theta)==total_lenght:
        pass
    else:
        logger.error(f"Theta must have lenght equal to {total_lenght} but a lenght {len(theta)} vector was provided.")
        raise  CircuitQRNNError
    
    # Slice theta into the parameters that will be used for the encoding part (orange on the image),
    # the evolution part (blue on the image) and the final evolution part (white on the image).
    # Image, again: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png
    orange = theta[:group1]
    blue = theta[group1:group1+group2]
    white = theta[group1+group2:]

    # Encoding block
    for i in range(repeat_encode):
        for qubit in range(nE):
            encoder.ry(x[time_step][qubit], qubit)
            encoder.rx(orange[2*i*nE + 2*qubit], qubit)
            encoder.rz(orange[2*i*nE + 2*qubit + 1], qubit)
    
    # Non-theta dependent part of the encoding
    for qubit in range(nE):
        encoder.ry(x[time_step][qubit], qubit)
        
def EMCZ2_evolver(nE: int, nM: int, theta: np.array, repeat_encode: int, repeat_evolution: int) -> CunqaCircuit:
    """
    Creates the evolution block circuit from the EMCZ QRNN algorithm, see https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png for a picture.

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*R + 2(nE + nM)*L + nE.
        repeat_encode (int): number of times that the encoding block should be repeated.
        repeat_evolution (int): number of times that the evolution block should be repeated.

    Returns:
        evolver (<class cunqa.CunqaCircuit>): (nE + nM)-qubit circuit implementing the evolution block of the EMCZ2 algorithm
    """
    encoder = CunqaCircuit(nE)

    group1 = 2*nE*repeat_encode
    group2 = 2*(nE+nM)*repeat_evolution
    group3 = 1*nE
    total_lenght = group1+group2+group3

    # Check correct format of theta
    if len(theta)==total_lenght:
        pass
    else:
        logger.error(f"Theta must have lenght equal to {total_lenght} but a lenght {len(theta)} vector was provided.")
        raise  CircuitQRNNError
    
    # Slice theta into the parameters that will be used for the encoding part (orange on the image),
    # the evolution part (blue on the image) and the final evolution part (white on the image).
    # Image, again: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png
    orange = theta[:group1]
    blue = theta[group1:group1+group2]
    white = theta[group1+group2:]

    evolver = CunqaCircuit(nE+nM)

    # Evolution block
    for j in range(repeat_evolution):
        for qubit in range(nE+nM-1): # Last qubit missing

            evolver.rx(blue[2*j*(nE+nM-1) + 2*qubit], qubit)
            evolver.rz(blue[2*j*(nE+nM-1) + 2*qubit + 1], qubit)
            evolver.cz(qubit, qubit + 1)

        # I separate the last iteration beacause it doesn't have a CZ with the next qubit
        evolver.rx(blue[(2*j+1)*(nE+nM-1)], nE + nM - 1)
        evolver.rz(blue[(2*j+1)*(nE+nM-1) + 1], nE + nM - 1)

    # Final part of the evolution
    for qubit in range(nE):
        evolver.rx(white[qubit], qubit)

    return evolver

def EMCZ3_encoder(nE: int, nM: int, x: np.array, theta: np.array, repeat_encode: int, repeat_evolution:int , time_step: int) -> CunqaCircuit:
    """
    Creates the encoder block circuit from the EMCZ3 QRNN algorithm, see https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png for a picture.

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*R + 2(nE + nM)*L + nE.
        x (numpy.array): Input data. Its shape must be (nT, nE), where this is the number of qubits on each register of the EMC QRNN circuit (function below).
        repeat_encode (int): number of times that the encoding block should be repeated.
        repeat_evolution (int): number of times that the evolution block should be repeated.
        time_step (int): indicates in which point of the time series we're on, ie the row of x that should be used.

    """
    encoder = CunqaCircuit(nE)
    assert np.shape(x)[1] == nE, "Error: The data x should have nE columns." 

    group1 = 3*nE*repeat_encode
    group2 = 3*(nE+nM)*repeat_evolution
    group3 = 3*nE
    total_lenght = group1+group2+group3

    # Check correct format of theta
    if len(theta)==total_lenght:
        pass
    else:
        logger.error(f"Theta must have lenght equal to {total_lenght} but a lenght {len(theta)} vector was provided.")
        raise  CircuitQRNNError
    
    # Slice theta into the parameters that will be used for the encoding part (orange on the image),
    # the evolution part (blue on the image) and the final evolution part (white on the image).
    # Image, again: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZme3.png
    orange = theta[:group1]
    blue = theta[group1:group1+group2]
    white = theta[group1+group2:]

    # Encoding block
    for i in range(repeat_encode):
        for qubit in range(nE):
            encoder.ry(x[time_step][qubit], qubit)
            encoder.rx(orange[3*i*nE + 3*qubit], qubit)
            encoder.rz(orange[3*i*nE + 3*qubit + 1], qubit)
            encoder.rx(orange[3*i*nE + 3*qubit + 2],qubit)
    
    # Non-theta dependent part of the encoding
    for qubit in range(nE):
        encoder.ry(x[time_step][qubit], qubit)

    return encoder

def EMCZ3_evolver(nE: int, nM: int, theta: np.array, repeat_encode: int, repeat_evolution:int) -> CunqaCircuit:
    """
    Creates the evolution block circuit from the EMCZ QRNN algorithm, see https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png for a picture.

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Vector lenght: 2nE*R + 2(nE + nM)*L + nE.
        repeat_encode (int): number of times that the encoding block should be repeated.
        repeat_evolution (int): number of times that the evolution block should be repeated.

    Returns:
        evolver (<class cunqa.CunqaCircuit>): (nE + nM)-qubit circuit implementing the evolution block of the EMCZ2 algorithm
    """
    evolver = CunqaCircuit(nE+nM)
    assert np.shape(x)[1] == nE, "Error: The data x should have nE columns." 

    group1 = 3*nE*repeat_encode
    group2 = 3*(nE+nM)*repeat_evolution
    group3 = 3*nE
    total_lenght = group1+group2+group3

    # Check correct format of theta
    if len(theta)==total_lenght:
        pass
    else:
        logger.error(f"Theta must have lenght equal to {total_lenght} but a lenght {len(theta)} vector was provided.")
        raise  CircuitQRNNError
    
    # Slice theta into the parameters that will be used for the encoding part (orange on the image),
    # the evolution part (blue on the image) and the final evolution part (white on the image).
    # Image, again: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZme3.png
    orange = theta[:group1]
    blue = theta[group1:group1+group2]
    white = theta[group1+group2:]

    # Evolution block
    for j in range(repeat_evolution):
        # E register part (CZ are performed)
        for qubit in range(nE): 

            evolver.rx(blue[3*j*(nE+nM-1) + 3*qubit], qubit)
            evolver.rz(blue[3*j*(nE+nM-1) + 3*qubit + 1], qubit)
            evolver.rx(blue[3*j*(nE+nM-1) + 3*qubit + 2], qubit)

            for qubit_m in range(nM):
                evolver.cz(qubit, nE + qubit_m)

        # M register part (CZ are received)
        for qubit in range(nE): 

            evolver.rx(blue[3*j*(nE+nM-1) + 3*qubit], qubit)
            evolver.rz(blue[3*j*(nE+nM-1) + 3*qubit + 1], qubit)
            evolver.rx(blue[3*j*(nE+nM-1) + 3*qubit + 2], qubit)

    # Final part of the evolution
    for qubit in range(nE):
        evolver.rx(white[3*i*nE + 3*qubit], qubit)
        evolver.rz(white[3*i*nE + 3*qubit + 1], qubit)
        evolver.rx(white[3*i*nE + 3*qubit + 2], qubit)

    return evolver   

    
    
# The next class is non-stateful to avoid messing with parallelization
class CircuitQRNN:
    # Later on we could accept another argument which determines the initial state of the Memory register
    def __init__(self, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int, ansatz: int = 2):
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
            ansatz (int): specify wether to use the model with U_2 = R_x R_z or the one with U_3 = R_x R_z R_x
        Return:
            circuit (CunqaCircuit): circuit implementing the QRNN structure
        """

        self.nE = nE
        self.nM = nM
        self.nT = nT
        self._repeat_encode = repeat_encode
        self._repeat_evolution = repeat_evolution
        
        self.ansatz = ansatz
        x_init = np.zeros((nT,nE)) # Initialize the parameters to zero as placeholders
        theta_init = np.zeros(2*nE*repeat_encode + 2*(nE+nM)*repeat_evolution + 1*nE)
        
        self.circuit = CunqaCircuit(nE + nM, nE*nT) # Number of cl_bits motivated by the measure of the Environment/Exchange register on each time_step

        # Determine which ansatz to use on the circuit
        if self.ansatz == 2:
            add_ansatz = add_EMCZ_ansatz
        elif self.ansatz == 3:
            add_ansatz = add_EMCZ3_ansatz

        # Build the circuit
        for time_step in range(nT):
            try:
                add_ansatz(self.circuit, nE, nM, x_init, theta_init, repeat_encode, repeat_evolution, time_step)
            except Exception as error:
                logger.error(f"An error occurred while creating the circuit [{error.__name__}].")
                raise CircuitQRNNError

            self.circuit.measure([i for i in range(nE)], [time_step*nE + i for i in range(nE)])
            self.circuit.reset([i for i in range(nE)]) 

        self.circuit.save_state()     
        
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
        if self.ansatz == 2:
            # Case of ansatz with U_2 = R_x R_z
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
        
        elif self.ansatz == 3:
            # Case of ansatz with U_3 = R_x R_z R_x
            all_params3 = []
            for t in range(self):
                for i in range(self._repeat_encode):
                    all_params3 += new_x[t,:]
                    all_params3 += new_theta[i*3*self.nE : (i+1)*3*self.nE]
                all_params3 += new_x[t,:]
                for j in range(self._repeat_evolution):
                    all_params3 += new_theta[j*3*(self.nE+self.nM) : (j+1)*3*(self.nE+self.nM)]
                all_params3 += new_theta[3*self.nE*self._repeat_encode + 3*(self.nE+self.nM)*self._repeat_evolution:]

            return all_params3

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
            raise CircuitQRNNError
        
        return qjob
          

