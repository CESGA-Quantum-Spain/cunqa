"""Contains the Result class, which deals with the output of QJobs using any simulator."""
from cunqa.logger import logger
from typing import Union, Optional
import numpy as np

class ResultError(Exception):
    """Exception for error during job submission to QPUs."""
    pass

class Result:
    """
    Class to describe the result of an experiment.
    """
    _result: dict 
    _id: str
    _registers: dict
    
    def __init__(self, result: dict, circ_id: str, registers: dict):
        """
        Initializes the Result class.

        Args:
        -----------
        result (dict): dictionary given as the result of the simulation.

        registers (dict): in case the circuit has more than one classical register, dictionary for the lengths of the classical registers must be provided.
        """

        self._result = {}
        self._id = circ_id
        self._registers = registers
        
        if result is None or len(result) == 0:
            logger.error(f"Empty object passed, result is {None} [{ValueError.__name__}].")
            raise ValueError
        
        elif "ERROR" in result:
            logger.debug(f"Result received: {result}\n")
            message = result["ERROR"]
            logger.error(f"Error during simulation, please check availability of QPUs, run arguments syntax and circuit syntax: {message}")
            raise ResultError
        
        else:
            logger.debug(f"Result received: {result}\n")
            self._result = result
        
        #logger.debug("Results correctly loaded.")


    # TODO: Use length of counts to justify time_taken (ms) at the end of the line.
    def __str__(self):
        RED = "\033[31m"
        YELLOW = "\033[33m"
        RESET = "\033[0m"   
        GREEN = "\033[32m"
        return f"{YELLOW}{self._id}:{RESET} {'{'}counts: {self.counts}, \n\t time_taken: {GREEN}{self.time_taken} s{RESET}{'}'}\n"


    @property
    def result(self) -> dict:
        return self._result
    

    @property
    def counts(self) -> dict:
        try:
            if "results" in list(self._result.keys()): # aer
                counts = self._result["results"][0]["data"]["counts"]

            elif "counts" in list(self._result.keys()): # munich and cunqa
                counts = self._result["counts"]
            else:
                logger.error(f"Some error occured with counts.")
                raise ResultError
            
            if len(self._registers) > 1:
                counts = convert_counts(counts, self._registers)

        except Exception as error:
            logger.error(f"Some error occured with counts [{type(error).__name__}]: {error}.")
            raise error
        
        return counts
    
    @property
    def statevector(self) -> Union[dict[np.array], np.array]:
        try:
            if ("results" in list(self._result.keys()) 
                and "result_types" in self._result["results"][0]["metadata"] 
                and "save_statevector" in self._result["results"][0]["metadata"]["result_types"].values()): # aer
                    
                    statevector = {} # All of this is because we can store multiple statevecs with labels different from "statevector"
                    for k, v in self._result["results"][0]["metadata"]["result_types"].items():
                        if v == "save_statevector":
                            statevector[k] = np.array(self._result["results"][0]["data"][k]).view(np.complex128)

                    if len(statevector) == 1:
                        statevector = list(statevector.values())[0] # Extract the statevector if we only have one
                

            elif "statevector" in self._result: # Munich and Cunqasim
                statevector = self._result["statevector"]
                if isinstance(statevector, dict):
                    for k, v in statevector.items():
                        statevector[k] = np.array(v).view(np.complex128)
                else:
                    statevector = np.array(statevector).view(np.complex128)

            else:
                logger.error(f"Statevector not found, try using circuit.save_state() at some point before executing.")
                raise ResultError

        except Exception as error:
            logger.error(f"Some error occured with Statevector [{type(error).__name__}]: {error}.")
            raise error
        
        return statevector

    @property
    def density_matrix(self) -> Union[dict[np.array], np.array]:
        try:
            if ("results" in list(self._result.keys()) 
                and "result_types" in self._result["results"][0]["metadata"] 
                and "save_density_matrix" in self._result["results"][0]["metadata"]["result_types"].values()): # aer
                
                density_matrix = {} # All of this is because we can store multiple densmats with labels different from "density_matrix"
                for k, v in self._result["results"][0]["metadata"]["result_types"].items():
                    if v == "save_density_matrix":
                        density_matrix[k] = np.array(self._result["results"][0]["data"][k]).view(np.complex128)

                if len(density_matrix) == 1:
                    density_matrix = list(density_matrix.values())[0] # Extract the statevector if we only have one

            else:
                logger.error(f"Density Matrix not found, try using circuit.save_state() before executing with Aer.")
                raise ResultError

        except Exception as error:
            logger.error(f"Some error occured with Density Matrix [{type(error).__name__}]: {error}.")
            raise error
        
        return density_matrix

    @property
    def time_taken(self) -> str:
        try:
            if "results" in list(self._result.keys()): # aer
                time = self._result["results"][0]["time_taken"]
                return time

            elif "counts" in list(self._result.keys()): # munich and cunqa
                time = self._result["time_taken"]          
                return time
            else:
                raise ResultError
        except Exception as error:
            logger.error(f"Some error occured with time taken [{type(error).__name__}]: {error}.")
            raise error
        

    def probabilities(self, per_qubit: bool = False, partial: list[int] = None, interface: bool = True) -> Union[dict[np.array], dict[ float], np.array]:
        """
        Extracts probabilities from result information. If we have statevector or density matrix 
        exact probabilities are obtained, otherwise frequencies are calculated from counts.

        Args:
            per_qubit: if True probabilities of 0 or 1 on each individual qubit are returned
                        instead of probabilities of each possible bitstring.
            partial: list of qubits whose probabilities should be returned, determining the probability space. 
                        Combinations need to be performed so their possible outcomes sum to probability 1.
            interface: controls wether a dictionary with keys "0010" or "qubit_0" should be returned 
                        or just a np.array with the probs in binary order or qubit order.
        Returns:
            probs (dict, np.array): probability information of the selected mode.
        """
        num_qubits = len( next(iter(self.counts.keys())) )
        partial = list(range( num_qubits )) if partial == None else partial 

        try:
            there_is_statevec = False 
            statevecs = self.statevector
            there_is_statevec = True
        except Exception as error:
            pass

        try:
            there_is_densmat = False
            densmats = self.density_matrix
            there_is_densmat = True
        except Exception as error:
            pass

        # Statevector
        if there_is_statevec:
            if isinstance(statevecs, dict):
                probs={}
                for k, statevec in statevecs.items():
                    probs[k] = np.reshape(np.abs(statevec), np.shape(statevec)[0])

            else:
                probs = np.reshape(np.abs(statevecs), np.shape(statevec)[0])

            if per_qubit:
                probs = recombine_probs(probs, partial, interface, num_qubits)
                probs = f"Probabilities: {probs}" if interface else probs

            return probs

        # Density matrix
        elif there_is_densmat:
            if isinstance(densmats, dict):
                probs = {}
                for k, densmat in densmats.items():
                    probs[k] =  np.diagonal(densmat, axis1=0).real
            else:
                probs =  np.diagonal(densmats, axis1=0).real

            if per_qubit:
                probs = recombine_probs(probs, partial, interface, num_qubits)
                probs = f"Probabilities: {probs}" if interface else probs
            
            return probs

        # Get frequencies from counts as estimation of probabilities if state is not available ---------------------
        if not any([there_is_densmat, there_is_statevec]): 
            if per_qubit:
 
                probs = {str(i_qubit): np.array([0, 0]) for i_qubit in partial}

                for bitstr, count in self.counts.items():
                    for i_qubit in partial:
                        probs[str(i_qubit)][bitstr[i_qubit]] += count # Notice that the order of qubits on the bitstring needs to got from 0 to num_qubits

                for i_qubit in partial:
                    probs[str(i_qubit)]/sum(probs[str(i_qubit)]) # Vectorized division to get frequencies

            else:
                probs = {}
                shots = sum(self.counts.values())
                for k, v in self.counts.items():
                    probs[k] = v/shots

            return probs
    

def divide(string: str, lengths: "list[int]") -> str:
    """
    Divides a string of bits in groups of given lenghts separated by spaces.

    Args:
    --------
    string (str): string that we want to divide.

    lengths (list[int]): lenghts of the resulting strings in which the original one is divided.

    Return:
    --------
    A new string in which the resulting groups are separated by spaces.

    """

    parts = []
    init = 0
    try:
        if len(lengths) == 0:
            return string
        else:
            for length in lengths:
                parts.append(string[init:init + length])
                init += length
            return ' '.join(parts)
    
    except Exception as error:
        logger.error(f"Something failed with division of string [{error.__name__}].")
        raise SystemExit # User's level


def convert_counts(counts: dict, registers: dict) -> dict:

    """
    Funtion to convert counts wirtten in hexadecimal format to binary strings and that applies the division of the bit strings.

    Args:
    --------
    counts (dict): dictionary of counts to apply the conversion.

    registers (dict): dictionary of classical registers.

    Return:
    --------
    Counts dictionary with keys as binary string correctly separated with spaces accordingly to the classical registers.
    """

    if isinstance(registers, dict):
        # getting lenghts of bits for the different registers
        lengths = []
        for v in registers.values():
            lengths.append(len(v))
    else:
        logger.error(f"regsters must be dict, but {type(registers)} was provided [TypeError].")
        raise ResultError # I capture this error in QJob.result()
    
    logger.debug(f"Dividing strings into {len(lengths)} classical registers.")

    if isinstance(counts, dict):
        new_counts = {}
        for k,v in counts.items():
            new_counts[divide(k, lengths)] = v
    else:
        logger.error(f"counts must be dict, but {type(registers)} was provided [TypeError].")
        raise ResultError # I capture this error in QJob.result()
    
    return new_counts

def recombine_probs(probs: Union[dict[np.array], np.array], partial: Union[None, list[int]], interface: bool, num_qubits: int):
    """
    Processes the probabilities per bitstring to obtain the probabilities per qubit. 

    Args:
        probs (np.array[float], dict[np.array[float]]): probabilities or list of probabilities per bitstring
        partial (None, list[int]): list of qubits (their index) that determine the total probability space
        interface (bool): determines wether we want an bare np.array or a dict with qubit_index as keys
        num_qubits (int): number of qubits that determines the lenght of the bitstrings

    Returns:
        new_probs (np.array[float], dict[np.array[float]]): probabilities or list of probabilities per qubit
    """
    if interface:
        new_probs = {}
        if isinstance(probs, dict): # Case where we have multiple sets of probs
            new_probs = {}

            for k, probs_k in probs.items():
                new_probs[k] = {str(i_qubit): np.array([0., 0.]) for i_qubit in partial}
                for base_ten_bitstring, prob in enumerate(probs_k): # Enumerate to extract the corresponding bitstring (in base 10)
                    for i_qubit in partial:

                        zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit])
                        new_probs[k][str(i_qubit)][zero_one] += prob

        else: # single set of probs to recombine
            new_probs = {str(i_qubit): np.array([0., 0.]) for i_qubit in partial}

            for base_ten_bitstring, prob in enumerate(probs_k):
                for i_qubit in partial:

                    zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit])
                    new_probs[str(i_qubit)][zero_one] += prob

    else:
        if isinstance(probs, dict):
            new_probs = {}

            for k, probs_k in probs.items():
                new_probs[k] = np.zeros((len(partial), 2))
                for base_ten_bitstring, prob in enumerate(probs_k):
                    for i, i_qubit in enumerate(partial):

                        zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit]) # extract wether i have a zero or a one on position i_qubit of the bitstring
                        new_probs[k][i, zero_one] += prob # for each qubit, i have a two element list with prob of one and prob of zero. Which element should be updated
                                                          # is determined by the zero or one on the bitstring
        else:
            new_probs = np.zeros((len(partial), 2))

            for base_ten_bitstring, prob in enumerate(probs_k):
                for i, i_qubit in enumerate(partial):

                    zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit])
                    new_probs[i, zero_one] += prob

    return new_probs


def recombine_bistring_probs(probs: Union[dict[np.array], np.array], partial: list[int], num_qubits: int):

    if isinstance(probs, dict):
        short_bitstring_probs = {} 
        for k, probs_k in probs.items():

            short_bitstring_probs[k] = {format(bitstring_ten, f"0{num_qubits}b"): 0.0 for  bitstring_ten in range(2**num_qubits)}
            for base_ten_bitstring, prob in enumerate(probs_k):

                shortened_bitstring = ''.join([format(base_ten_bitstring, f"0{num_qubits}b")[i] for i in partial])
                short_bitstring_probs[k][shortened_bitstring] += prob

    elif isinstance(probs, np.array):        

        short_bitstring_probs = {format(bitstring_ten, f"0{num_qubits}b"): 0.0 for bitstring_ten in range(2**num_qubits)}
        for base_ten_bitstring, prob in enumerate(probs):

            shortened_bitstring = ''.join([format(base_ten_bitstring, f"0{num_qubits}b")[i] for i in partial])
            short_bitstring_probs[shortened_bitstring] += prob

    return short_bitstring_probs