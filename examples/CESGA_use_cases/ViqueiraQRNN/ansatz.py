"""
 Title: QRNN ansatz class
 Description: class that manages ansatzes for QRNN circuits with the structure from https://arxiv.org/abs/2310.20671

Created 10/07/2025
@author: dexposito (algorithm idea: jdviqueira)
"""

import os, sys
import math
import inspect
import functools
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from cunqa.logger import logger

class AnsatzQRNNError(Exception):
    """Exception for error during QRNN ansatz creation."""
    pass



def order_parameters(self, **args):
    """
    Method to order new parameter values to assign to a circuit. Needed to easily handle the parameter input on the repeated encoder and evolver
    without having to manually repeat the values to assign.

    Args:
        marked_params (dict[list | float | int]): dict with keys the labels of the variable parameters and associated values 
        the new parameters to assign to them. The values are entered with the syntax theta_1 = 3.14, theta_2 = [2, 7], theta_3 = 9
    """
    ordered_params = self.current_params # Copy current params and only modify the given ones
    labels = self.param_labels

    for index, label in enumerate(labels):
        if label in args:
            if isinstance(args[label], (int, float)):
                ordered_params[index]=args[label]
            elif isinstance(args[label], list):
                ordered_params[index]=args[label].pop(0)
            else:
                logger.error(f"Parameters must be list[int, float], int or float but {type(args[label])} was given.")
                raise SystemExit
                
    if not all([len(value)==0 for value in args.values() if isinstance(value, list)]):
            logger.warning(f"Some of the given parameters were not used, check name or lenght of the following keys: {[k for k, v in args.items() if isinstance(v, list) and len(v)!=0]}. \n Use circuit.param_info to obtain the names and numbers of the variable parameters.")
    
    return ordered_params

CunqaCircuit.order_parameters = order_parameters



class AnsatzQRNN:
    """
    Class to define ansatzes with the QRNN structure. Conceptually these ansatzes are divided into the Encoding and the Evolution part. 
    In our implementation both of these blocks are divided in two parts ''encoder + final_encoding'' and ''evolver + final_evolution''. 
    The reason is that the ''encoder'' and the ''evolver'' will be repeated 'repeat_encode' and 'repeat_evolution' times, respectively, 
    without having to write it out explicitly.

    The full circuit can then be obtained using :py:meth:`ViqueiraQRNN.ansatz.get_full_circuit`.
    """
    def __init__(self, nE, nM, repeat_encode, repeat_evolution, name="ansatz"):

        self.nE = nE
        self.nM = nM
        self._repeat_encode = repeat_encode
        self._repeat_evolution = repeat_evolution

        self._encoder = CunqaCircuit(nE)
        self._final_encoding = CunqaCircuit(nE)
        self._evolver = CunqaCircuit(nE + nM)
        self._final_evolution = CunqaCircuit(nE)

        self.name = name

    @property
    def encoder(self):
        return self._encoder
    
    @property
    def final_encoding(self):
        return self._final_encoding

    @property
    def evolver(self):
        return self._evolver
    
    @property
    def final_evolution(self):
        return self._final_evolution
    
    @encoder.setter
    def encoder(self, circuit):
        if not (circuit.num_qubits == self.nE):
            logger.error(f"The provided encoder doesn't match the number of qubits of the Exchange/Environment register: provided has {circuit.num_qubits}, which is different from {self.nE}.")
            raise AnsatzQRNNError
        else:
            self._encoder = circuit

    @final_encoding.setter
    def final_encoding(self, circuit):
        if not (circuit.num_qubits == self.nE):
            logger.error(f"The provided final_encoding doesn't match the number of qubits of the Exchange/Environment register: provided has {circuit.num_qubits}, which is different from {self.nE}.")
            raise AnsatzQRNNError
        else:
            self._final_encoding = circuit

    @evolver.setter
    def evolver(self, circuit):
        if not (circuit.num_qubits == (self.nE + self.nM)):
            logger.error(f"The provided evolver doesn't match the number of qubits of the ansatz: provided has {circuit.num_qubits}, which is different from {self.nE + self.nM}.")
            raise AnsatzQRNNError
        else:       
            self._evolver = circuit  

    @final_evolution.setter
    def final_evolution(self, circuit):
        if not (circuit.num_qubits == self.nE):
            logger.error(f"The provided final_evolution doesn't match the number of qubits of the Exchange/Environment register: provided has {circuit.num_qubits}, which is different from {self.nE}.")
            raise AnsatzQRNNError
        else:
            self._final_evolution = circuit  

    def get_full_circuit(self):
        """
        Combines all blocks (appropriately repeated) to generate the full ansatz.

        Returns:
            self.full_circuit (CunqaCircuit): (nE+nM)-qubit circuit describing the whole ansatz.
        """
        self.full_circ = CunqaCircuit(self.nE + self.nM)

        for _ in range(self._repeat_encode):
            self.full_circ.from_instructions(self._encoder.instructions)
        
        self.full_circ.from_instructions(self._final_encoding.instructions)

        for _ in range(self._repeat_evolution):
            self.full_circ.from_instructions(self._evolver.instructions)

        self.full_circ.from_instructions(self._final_evolution.instructions)

        return self.full_circ
    
    def total_order_params(self, repeat: list[str] = [], **args):
        """
        Order the given parameters follwoing the way they appear on the circuit.

        Args:
            repeat (list[str]): list of names of parameters that stay the same on all the repeated encoding and evolution blocks
            args (dict[float | int | list[float | int]]): name of the parameter with the associated new values to order

        Returns:
            total_ordered_params (list): the full list of ordered parameters
        """
        # Need to know how to distribute the arguments between the encoding, 
        # final_encoding, evolution and final_evolution blocks
        len_encoder  = self.encoder.param_info(args.keys()) # Problema: si encoder no tiene un parametro luego falla el sacar su longitud (debería ser 0)
        len_fencoder = self.final_encoding.param_info(args.keys())
        len_evolver  = self.evolver.param_info(args.keys())
        len_fevolver = self.final_evolution.param_info(args.keys())
        args_encoder = {}
        args_fencode = {}
        args_evolver = {}
        args_fevolve = {}

        for arg, value in args.items():
            if isinstance(value, list):
                if arg in repeat:

                    arg_lenght_circ = len_encoder[arg] + len_fencoder[arg] + len_evolver[arg] + len_fevolver[arg]
                    if (isinstance(value, list) and not (arg_lenght_circ == len(value))):
                        logger.error(f"Mismatch between number of values for parameter {arg}: {len(value)} values were provided, but the circuit has {arg_lenght_circ} values.")
                        raise AnsatzQRNNError
                    
                    args_encoder[arg] =    self._repeat_encode * value[ : len_encoder[arg]  ]
                    args_fencode[arg] =                          value[ : len_fencoder[arg] ]
                    args_evolver[arg] = self._repeat_evolution * value[ : len_evolver[arg]  ]
                    args_fevolve[arg] =                          value[ : len_fevolver[arg] ]

                else:
                    arg_lenght_circ = (len_encoder[arg] * self._repeat_encode) + len_fencoder[arg] + (len_evolver[arg] * self._repeat_evolution) + len_fevolver[arg]
                    if not (arg_lenght_circ == len(value)):
                        logger.error(f"Mismatch between number of values for parameter {arg}: {len(value)} values were provided, but the circuit has {arg_lenght_circ} values.")
                        raise AnsatzQRNNError
                    
                    args_encoder[arg] = value[                                                                                                            : (len_encoder[arg] * self._repeat_encode) ]
                    args_fencode[arg] = value[(len_encoder[arg] * self._repeat_encode)                                                                    : (len_encoder[arg] * self._repeat_encode) + len_fencoder[arg] ]
                    args_evolver[arg] = value[(len_encoder[arg] * self._repeat_encode) + len_fencoder[arg]                                                : (len_encoder[arg] * self._repeat_encode) + len_fencoder[arg] + (len_evolver[arg] * self._repeat_evolution) ]
                    args_fevolve[arg] = value[(len_encoder[arg] * self._repeat_encode) + len_fencoder[arg] + (len_evolver[arg] * self._repeat_evolution)  : ]

            elif isinstance(value, (float, int)):
                if len_encoder[arg]!=0:
                    args_encoder[arg] = [value] * len_encoder[arg] * self._repeat_encode     
                if len_fencoder[arg]!=0:
                    args_fencode[arg] = [value] * len_fencoder[arg]                         
                if len_evolver[arg]!=0: 
                    args_evolver[arg] = [value] * len_evolver[arg] * self._repeat_evolution 
                if len_fevolver[arg]!=0:
                    args_fevolve[arg] = [value] * len_fevolver[arg]                         


        # Order correctly 
        total_ordered_params = []
        for i in range(self._repeat_encode):
            window = {arg: value[i*len_encoder[arg] : (i+1)*len_encoder[arg]] for arg, value in args_encoder.items()}
            total_ordered_params += self.encoder.order_parameters(window) 

        total_ordered_params += self.final_encoding.order_parameters(args_fencode) 

        for i in range(self._repeat_evolution):
            window = {arg: value[i*len_evolver[arg] : (i+1)*len_evolver[arg]] for arg, value in args_evolver.items()}
            total_ordered_params += self.evolver.order_parameters(window)

        total_ordered_params += self.final_evolution.order_parameters(args_fevolve)

        return total_ordered_params
        

                

#########################################################################        
#################### DEFINITION OF OUR MAIN ANSATZES ####################
#########################################################################

def EMCZ2(nE, nM, repeat_encode, repeat_evolution):
    """
    Creates the ansatz for the EMCZ2 QRNN algorithm. A picture of the ansatz form: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png
    """
    emcz2 = AnsatzQRNN(nE, nM, repeat_encode, repeat_evolution, name = "EMCZ2")

    # Define encoding
    for qubit in range(nE):
            emcz2.encoder.ry(param = "x", qubit = qubit)
            emcz2.encoder.rx(param = "theta", qubit = qubit)
            emcz2.encoder.rz(param = "theta", qubit = qubit)

    # Final encoding
    for qubit in range(nE):
        emcz2.final_encoding.ry(param = "x", qubit = qubit)

    # Define evolution
    for qubit in range(nE+nM): 

        emcz2.evolver.rx(param = "theta", qubit = qubit)
        emcz2.evolver.rz(param = "theta", qubit = qubit)

    for qubit in range(nE+nM-1):
        emcz2.evolver.cz(qubit, qubit + 1)

    # Final encoding
    for qubit in range(nE):
        emcz2.final_evolution.rx(param = "theta", qubit = qubit)

    return emcz2




def EMCZ3(nE, nM, repeat_encode, repeat_evolution):
    """
    Creates the ansatz for the EMCZ2 QRNN algorithm. A picture of the ansatz form: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZme3.png
    """
    emcz3 = AnsatzQRNN(nE, nM, repeat_encode, repeat_evolution, name = "EMCZ3")

    # Define encoding
    for qubit in range(nE):
            emcz3.encoder.ry(param = "x", qubit = qubit)
            emcz3.encoder.rx(param = "theta", qubit = qubit)
            emcz3.encoder.rz(param = "theta", qubit = qubit)
            emcz3.encoder.rx(param = "theta", qubit = qubit)

    # Final encoding
    for qubit in range(nE):
        emcz3.final_encoding.ry(param = "x", qubit = qubit)

    # Define evolution
    # E register part (CZ are performed)
    for qubit in range(nE+nM): 
        emcz3.evolver.rx(param = "theta", qubit = qubit)
        emcz3.evolver.rz(param = "theta", qubit = qubit)
        emcz3.evolver.rx(param = "theta", qubit = qubit)

    for qubit in range(nE):
        for qubit_m in range(nM):
            emcz3.evolver.cz(qubit, nE + qubit_m)
            

    # Final evolution
    for qubit in range(nE):
        emcz3.final_evolution.rx(param = "theta", qubit = qubit)
        emcz3.final_evolution.rz(param = "theta", qubit = qubit)
        emcz3.final_evolution.rx(param = "theta", qubit = qubit)

    return emcz3