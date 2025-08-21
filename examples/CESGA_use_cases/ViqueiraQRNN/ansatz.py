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

from cunqa.circuit import CunqaCircuit, _is_parametric
from cunqa.logger import logger

class AnsatzQRNNError(Exception):
    """Exception for error during QRNN ansatz creation."""
    pass

# The following wrapper for CunqaCircuit is not as elegant as i would like. I'll maybe integrate this functionality on cunqa to avoid it
def parametric_wrapper(cls_inst):
    """
    Class decorator to provide abstract Parameter functionality for parametric gates.
    It records the order of parametric gates, marks which of the entries where abstract parameters to be filled - which are identified with
    the parameter = "name_str" keyword argument - , fills those positions with 0. placeholders, and gives the correct order to feed to
    qjob.upgrade_parameters in order to correctly bind the values to the parameter.
    """
    # Store the input parameters in order, wether they are concrete values or abstract parameters
    cls_inst.param_instructions = []
    
    # Go through class methods, extract the ones for parametric gates and add abstract parameter functionality
    for name, method in cls_inst.__dict__.items():
        if callable(method):
            try:
                signature = inspect.signature(method)
                inputs = list(signature.parameters.keys()) 

                if (("qubit" in inputs or "qubits" in inputs) and _is_parametric( {"instructions":[{"name": name}]})):
                    # Create a wrapper for each parametric method
                    def remembering_wrapper(parametric_method):
                        def wrapper(*args, label: str = "NoName", **kwargs):

                            cls_inst.param_instructions.append(label) # save the name of the parameter

                            # Ensure all necessary parameter keys are provided and if not, put zeros on them
                            missing_args = {}
                            if not len(inputs) == len(args)+len(kwargs): 
                                inputs.remove("self")
                                if "qubit" in inputs:
                                    inputs.remove("qubit")
                                else:
                                    inputs.remove("qubits")
                                for arg in inputs.remove(1,-1): # remove self and qubits
                                    missing_args[arg]=float("Infinity")

                            return parametric_method(*args, **missing_args, **kwargs)
                        return wrapper
                    
                    # Replace the method with the wrapped version
                    setattr(cls_inst, name, remembering_wrapper(method))
                
            except Exception as error:
                logger.error(f"Error while inspecting method arguments and creating wrapper.")
                raise SystemExit

    # Extend the "from_instructions" method to ensure abstract parameters work with any input method
    def extended_from_instructions(self, instructions):
        single_param = ['u1', 'p', 'rx', 'ry', 'rz', 'rxx', 'ryy', 'rzz', 'rzx', 'crx', 'cry', 'crz', 'cp', 'cu1']
        two_params = ['u2', 'r']
        three_params = ['u', 'u3', 'cu3']
        four_params = ['cu']

        for instruction in instructions:

            if _is_parametric(instruction):
                if "label" in instruction:
                    # Add zero parameters as placeholders to later apply upgrade_parameters
                    if instruction["name"] in  single_param: # one param
                        cls_inst.param_instructions.append(instruction["label"])
                        if not "param" in instruction:
                            instruction["param"] = 0.

                    if instruction["name"] in two_params: # two params: theta, phi
                        cls_inst.param_instructions + [instruction["label"], instruction["label"]]
                        if not "theta" in instruction:
                            instruction["theta"] = 0.
                        if not "phi" in instruction:
                            instruction["phi"] = 0.

                    if instruction["name"] in three_params: # three params: theta, phi, lam
                        cls_inst.param_instructions + [instruction["label"], instruction["label"], instruction["label"]]
                        if not "theta" in instruction:
                            instruction["theta"] = 0.
                        if not "phi" in instruction:
                            instruction["phi"] = 0.
                        if not "lam" in instruction:
                            instruction["lam"] = 0.

                    if instruction["name"] in four_params: # four params: theta, phi, lam, gamma
                        cls_inst.param_instructions + [instruction["label"], instruction["label"], instruction["label"], instruction["label"]]
                        if not "theta" in instruction:
                            instruction["theta"] = 0.
                        if not "phi" in instruction:
                            instruction["phi"] = 0.
                        if not "lam" in instruction:
                            instruction["lam"] = 0.
                        if not "gamma" in instruction:
                            instruction["gamma"] = 0.

                    del instruction["label"]

                else: # Record non-abstract parameters to preserve their values when upgrading the abstract ones
                    if instruction["name"] in single_param:
                        cls_inst.param_instructions + [instruction["param"]]
                    if instruction["name"] in two_params:
                        cls_inst.param_instructions + [instruction["theta"], instruction["phi"]]
                    if instruction["name"] in three_params:
                        cls_inst.param_instructions + [instruction["theta"], instruction["phi"], instruction["lam"]]
                    if instruction["name"] in four_params:
                        cls_inst.param_instructions + [instruction["theta"], instruction["phi"], instruction["lam"], instruction["gamma"]]
                
            self._add_instruction(instruction)
        return self
     
    # Create methods to extract information about the parameters and obtain their correct order
    def order_parameters(self, **args: dict[Union[int, float]]):
        ordered_params = self.param_instructions
        for arg, value in args.items():
            for index, name in enumerate(self.param_instructions):
                ordered_params[index] = value[index] if arg == name else ordered_params[index]
        
        return ordered_params
    
    def lenght_parameters(self, *args):
        parameters = self.param_instructions
        lenghts = {arg: 0 for arg in args}

        for param in parameters:
            if param in lenghts:
                lenghts[param] += 1

        return lenghts

    # Assign new methods to the wrapped class
    cls_inst.order_parameters = order_parameters
    cls_inst.lenght_parameters = lenght_parameters
    cls_inst.from_instructions = extended_from_instructions
    
    return cls_inst



class AnsatzQRNN:
    def __init__(self, nE, nM, repeat_encode, repeat_evolution, name="ansatz"):

        self.nE = nE
        self.nM = nM
        self._repeat_encode = repeat_encode
        self._repeat_evolution = repeat_evolution

        self._encoder = parametric_wrapper(CunqaCircuit)(nE)
        self._final_encoding = parametric_wrapper(CunqaCircuit)(nE)
        self._evolver = parametric_wrapper(CunqaCircuit)(nE + nM)
        self._final_evolution = parametric_wrapper(CunqaCircuit)(nE)

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
            try:
                circuit.param_instructions
            except:
                logger.error(f"The encoder provided doesn't support the abstract parameter functionalities. Try creating it through the Ansatz QRNN class.")
                raise AnsatzQRNNError
        
            self._encoder = circuit

    @final_encoding.setter
    def final_encoding(self, circuit):
        if not (circuit.num_qubits == self.nE):
            logger.error(f"The provided final_encoding doesn't match the number of qubits of the Exchange/Environment register: provided has {circuit.num_qubits}, which is different from {self.nE}.")
            raise AnsatzQRNNError
        else:
            try:
                circuit.param_instructions
            except:
                logger.error(f"The final_encoding provided doesn't support the abstract parameter functionalities. Try creating it through the Ansatz QRNN class.")
                raise AnsatzQRNNError
        
            self._final_encoding = circuit

    @evolver.setter
    def evolver(self, circuit):
        if not (circuit.num_qubits == (self.nE + self.nM)):
            logger.error(f"The provided evolver doesn't match the number of qubits of the ansatz: provided has {circuit.num_qubits}, which is different from {self.nE + self.nM}.")
            raise AnsatzQRNNError
        else:
            try:
                circuit.param_instructions
            except:
                logger.error(f"The evolver provided doesn't support the abstract parameter functionalities. Try creating it through the Ansatz QRNN class.")
                raise AnsatzQRNNError
        
            self._evolver = circuit  

    @final_evolution.setter
    def final_evolution(self, circuit):
        if not (circuit.num_qubits == self.nE):
            logger.error(f"The provided final_evolution doesn't match the number of qubits of the Exchange/Environment register: provided has {circuit.num_qubits}, which is different from {self.nE}.")
            raise AnsatzQRNNError
        else:
            try:
                circuit.param_instructions
            except:
                logger.error(f"The final_evolution provided doesn't support the abstract parameter functionalities. Try creating it through the Ansatz QRNN class.")
                raise AnsatzQRNNError
        
            self._final_evolution = circuit  

    def get_full_circuit(self):
        """
        Combines all blocks (appropriately repeated) to generate the full ansatz.

        Returns:
            self.full_circuit ( parametric_wrapper(CunqaCircuit) ): circuit (with abstract parameter functionalities) describing the whole ansatz.
        """
        self.full_circ = parametric_wrapper(CunqaCircuit)(self.nE + self.nM)

        for _ in range(self._repeat_encode):
            self.full_circ.from_instructions(self._encoder.instructions)
        
        self.full_circ.from_instructions(self._final_encoding.instructions)

        for _ in range(self._repeat_evolution):
            self.full_circ.from_instructions(self._evolver.instructions)

        self.full_circ.from_instructions(self._final_evolution.instructions)

        return self.full_circ

    def total_order_params(self, recompile = False, **params: dict[str, Union[int, float, list[Union[int, float]]]]) -> list[Union[int, float]]:
        """
        Finds the occurences of the parameters on the full circuit and returns the input values on the
        appropriate order so that upgrade_parameters places the values on the correct gates.

        Args:
            recompile (bool): lets the function know if the full circuit should be calculated again
            **params (dict): any parameters to be input, using keyword arguments with the format
                             label1 = value1, label2 = [value2_a, value2_b]

        Return:
            (list[float, int]): ordered values to be placed on the parametric gates
        """
        if hasattr(self, "full_circ"):
            if recompile:
                self.get_full_circuit()
            return self.full_circ.order_parameters(params)
        else:
            self.get_full_circuit()
            return self.full_circ.order_parameters(params)
            
        


        
#################### DEFINITION OF OUR MAIN ANSATZES ####################

def EMCZ2(nE, nM, repeat_encode, repeat_evolution):
    """
    Creates the ansatz for the EMCZ2 QRNN algorithm. A picture of the ansatz form: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZladder2p1.png
    """
    emcz2 = AnsatzQRNN(nE, nM, repeat_encode, repeat_evolution, name = "EMCZ2")

    # Define encoding
    for qubit in range(nE):
            emcz2.encoder.ry(qubit, label = "x")
            emcz2.encoder.rx(qubit, label = "theta")
            emcz2.encoder.rz(qubit, label = "theta")

    for qubit in range(nE):
        emcz2.final_encoding.ry(qubit, label = "x")

    # Define evolution
    for qubit in range(nE+nM-1): # Last qubit missing

        emcz2.evolver.rx(qubit, label = "theta")
        emcz2.evolver.rz(qubit, label = "theta")
        emcz2.evolver.cz(qubit, qubit + 1)
         
    emcz2.evolver.rx(nE + nM - 1, label = "theta") # I separate the last iteration beacause it doesn't have a CZ with the next qubit
    emcz2.evolver.rz(nE + nM - 1, label = "theta")

    for qubit in range(nE):
        emcz2.final_evolution.rx(qubit, label = "theta")

    return emcz2




def EMCZ3(nE, nM, repeat_encode, repeat_evolution):
    """
    Creates the ansatz for the EMCZ2 QRNN algorithm. A picture of the ansatz form: https://github.com/jdani98/qutims/blob/release/0.2/.images/quantum_ansatz_CZme3.png
    """
    emcz3 = AnsatzQRNN(nE, nM, repeat_encode, repeat_evolution, name = "EMCZ3")

    # Define encoding
    for qubit in range(nE):
            emcz3.encoder.ry(qubit, label = "x")
            emcz3.encoder.rx(qubit, label = "theta")
            emcz3.encoder.rz(qubit, label = "theta")
            emcz3.encoder.rx(qubit, label = "theta")

    for qubit in range(nE):
        emcz3.final_encoding.ry(qubit, label = "x")

    # Define evolution
    # E register part (CZ are performed)
    for qubit in range(nE): 

        emcz3.evolver.rx(qubit, label = "theta")
        emcz3.evolver.rz(qubit, label = "theta")
        emcz3.evolver.rx(qubit, label = "theta")

        for qubit_m in range(nM):
            emcz3.evolver.cz(qubit, nE + qubit_m)

    # M register part (CZ are received)
    for qubit in range(nM): 

        emcz3.evolver.rx(qubit, label = "theta")
        emcz3.evolver.rz(qubit, label = "theta")
        emcz3.evolver.rx(qubit, label = "theta")

    for qubit in range(nE):
        emcz3.final_evolution.rx(qubit, label = "theta")
        emcz3.final_evolution.rz(qubit, label = "theta")
        emcz3.final_evolution.rx(qubit, label = "theta")

    return emcz3