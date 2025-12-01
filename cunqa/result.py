"""
    Contains the :py:class:`~cunqa.result.Result` class, which gathers the information about the output of a simulations, also other functions
    to manage them.

    Once we have submmited a :py:class:`~cunqa.qjob.QJob`, for obtaining its results we call for its propperty :py:attr:`QJob.result`:

        >>> qjob.result
        <cunqa.result.Result object at XXXX>
    
    This object has two main attributes for out interest: the counts distribution from the simulation and the time that the simulation took in seconds:

        >>> result = qjob.result
        >>> result.counts
        {'000':34, '111':66}
        >>> result.time_taken
        0.056
    

"""
import logging
from cunqa.logger import logger
from typing import Union, Optional
import numpy as np

class ResultError(Exception):
    """Exception for error received from a simulation."""
    pass

class Result:
    """
    Class to describe the result of a simulation.

    It has two main attributes:

    - :py:attr:`Result.counts` : returns the distribution of counts from the sampling of the simulation.

            >>> result.counts
            {'000':34, '111':66}

    .. note::
        If the circuit sent has more than one classical register, bit strings corresponding to each one of them will be separated
        by blank spaces in the order they were added:

            >>> result.counts
            {'001 11':23, '110 10':77}
    
    - :py:attr:`Result.time_taken` : the time that the simulation took.

            >>> result.time_taken
            0.056

    Nevertheless, depending on the simulator used, more output data is provided. For checking all the information from the simulation as a ``dict``, one can
    access the attribute :py:attr:`Result.result`.

    If an error occurs at the simulation, an exception will be raised at the pyhton program, :py:exc:`ResultError`.
    """
    _result: dict
    _id: str
    _cl_registers: dict
    
    def __init__(self, result: dict, circ_id: str, cl_registers: dict):
        """
        Initializes the Result class.

        Args:
            result (dict): dictionary given as the output of the simulation.

            circ_id (str): circuit identificator.

            cl_registers (dict): dictionary specifying the classical registers defined for the circuit. This is neccessary for the correct formating of the counts bit strings.
        """

        self._result = {}
        self._id = circ_id
        self._cl_registers = cl_registers
        
        if result is None or len(result) == 0:
            logger.error(f"Empty object passed, result is {None} [{ValueError.__name__}].")
            raise ValueError
        
        elif "ERROR" in result:
            #logger.debug(f"Result received: {result}\n")
            message = result["ERROR"]
            logger.error(f"Error during simulation, please check availability of QPUs, run arguments syntax and circuit syntax: {message}")
            raise ResultError
        
        else:
            #logger.debug(f"Result received: {result}\n")
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
        """Raw output of the simulation, the ``dict`` format depends on the simulator used."""
        return self._result
    

    @property
    def counts(self) -> dict:
        """Counts distribution from the sampling of the simulation, format is ``{"<bit string>":<number of counts as int>}``."""
        try:
            if "results" in list(self._result.keys()): # aer
                counts = self._result["results"][0]["data"]["counts"]

            elif "counts" in list(self._result.keys()): # munich and cunqa
                counts = self._result["counts"]
            else:
                logger.error(f"Some error occured with counts.")
                raise ResultError
            
            if len(self._cl_registers) > 1:
                counts = _convert_counts(counts, self._cl_registers)

        except Exception as error:
            logger.error(f"Some error occured with counts [{type(error).__name__}]: {error}.")
            raise error
        
        return counts

    @property
    def time_taken(self) -> str:
        """Time that the simulation took in seconds, since it is recieved at the virtual QPU until it is finished."""
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
        
    @property
    def statevector(self) -> Union[dict[np.array], np.array]:
        """Statevector or dictionary of statevectors captured at the moment that `.save_state()` was performed on the circuit."""
        try:
            if ("results" in list(self._result.keys()) 
                and "result_types" in self._result["results"][0]["metadata"] 
                and "save_statevector" in self._result["results"][0]["metadata"]["result_types"].values()): # AER
                    
                    statevector = {} # Dict because we can store multiple statevecs with labels different from "statevector"
                    for k, v in self._result["results"][0]["metadata"]["result_types"].items():
                        if v == "save_statevector":
                            statevector[k] = np.array(self._result["results"][0]["data"][k]).view(np.complex128)

                    if len(statevector) == 1:
                        statevector = list(statevector.values())[0] # Extract the statevector if we only have one
                
            # TODO: ensure this actually works in C++
            elif "statevector" in self._result:             # MUNICH and CUNQA_SIMULATOR
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
        """Density matrix or dictionary of density matrices captured at the moment that `.save_state()` was performed on a circuit runned with `method = density_matrix` on AER."""
        try:
            if ("results" in list(self._result.keys()) 
                and "result_types" in self._result["results"][0]["metadata"] 
                and "save_density_matrix" in self._result["results"][0]["metadata"]["result_types"].values()): # AER
                
                density_matrix = {} # Dict because we can store multiple densmats with labels different from "density_matrix"
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
    
    def probabilities(self) -> Union[dict[np.array],  np.array]:
        """
        Extracts probabilities from result information. If we have statevector or density matrix 
        exact probabilities are obtained, otherwise frequencies are calculated from counts.

        Returns:
            probs (dict, np.array): probabilities per bitstring found on counts. The probabilities are
                                    returned on an array unless multiple cl_registers are found, 
                                    in which case a dict is returned instead. Probs include zero probabilities.
        """
        # Temporarily disable logging
        logging.disable(logging.CRITICAL)

        try:
            there_is_statevec = False 
            statevecs = self.statevector
            there_is_statevec = True
        except Exception:
            pass

        try:
            there_is_densmat = False
            densmats = self.density_matrix
            there_is_densmat = True
        except Exception:
            pass
        
        # Logging will be re-enabled after this block
        logging.disable(logging.NOTSET)

        # Statevector
        if there_is_statevec:
            logger.debug("Extracting probabilities from statevector.")
            if isinstance(statevecs, dict):
                probs={}
                for k, statevec in statevecs.items():
                    probs[k] = np.reshape(np.power(np.abs(statevec), 2), np.shape(statevec)[0])

            else:
                probs = np.reshape(np.power(np.abs(statevecs), 2), np.shape(statevecs)[0])

            return probs

        # Density matrix
        elif there_is_densmat:
            logger.debug("Extracting probabilities from density_matrix.")
            if isinstance(densmats, dict):
                probs = {}
                for k, densmat in densmats.items():
                    probs[k] =  np.diagonal(densmat, axis1=0).real
            else:
                probs =  np.diagonal(densmats, axis1=0).real
            
            return probs

        # Get frequencies from counts as estimation of probabilities if state is not available ---------------------
        else: 
            logger.debug(f"Estimating probabilities from the available counts. First ten counts: { {k: v for i, k, v in enumerate(self.counts.items()) if i<11} }")
            if len(self._cl_registers) > 1:
                logger.debug(f"Computing probabilities of a circuit with {len(self._cl_registers)} classical registers. Lenght of probabilities may not correspond with 2^num_qubits.")

                n = len(next(iter(self.counts.keys())).replace(" ", ""))
                num_bitstrings = 2**n
                if len(self.counts) != num_bitstrings:
                    new_counts = {**_convert_counts({f"{i:0{n}b}": 0 for i in range(num_bitstrings)}), **self.counts} # Python 3.7+ is needed to preserve first dict's order
                else:
                    new_counts = self.counts

                probs = {}
                numpy_counts = np.array(list(new_counts.values()))
                all_shots = np.sum(numpy_counts)

                probs_array = numpy_counts/all_shots
                for k, v in zip(new_counts.keys(), probs_array):
                    probs[k] = v

                return probs
            
            # If not all bitstrings are present, add them with count 0 (Consistent with state vector and density matrix methods)
            num_qubits = len(next(iter(self.counts.keys())))
            num_bitstrings = 2**num_qubits
            if len(self.counts) != num_bitstrings:
                new_counts = {**{f"{i:0{num_qubits}b}": 0 for i in range(num_bitstrings)}, **self.counts} # Python 3.7+ is needed to preserve first dict's order

            else:
                new_counts = self.counts

            numpy_counts = np.array(list(new_counts.values()))
            all_shots = np.sum(numpy_counts)

            probs = numpy_counts/all_shots

            return probs
    

def _divide(string: str, lengths: "list[int]") -> str:
    """
    Divides a string of bits in groups of given lenghts separated by spaces.

    Args:
        string (str): string that we want to divide.

        lengths (list[int]): lenghts of the resulting strings in which the original one is divided.

    Return:
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


def _convert_counts(counts: dict, cl_registers: dict) -> dict:

    """
    Funtion to convert counts wirtten in hexadecimal format to binary strings and that applies the division of the bit strings.

    Args:
    --------
    counts (dict): dictionary of counts to apply the conversion.

    cl_registers (dict): dictionary of classical registers.

    Return:
    --------
    Counts dictionary with keys as binary string correctly separated with spaces accordingly to the classical registers.
    """

    if isinstance(cl_registers, dict):
        # getting lenghts of bits for the different cl_registers
        lengths = []
        for v in cl_registers.values():
            lengths.append(len(v))
    else:
        logger.error(f"regsters must be dict, but {type(cl_registers)} was provided [TypeError].")
        raise ResultError # I capture this error in QJob.result()
    
    logger.debug(f"Dividing strings into {len(lengths)} classical registers.")

    if isinstance(counts, dict):
        new_counts = {}
        for k,v in counts.items():
            new_counts[_divide(k, lengths)] = v
    else:
        logger.error(f"counts must be dict, but {type(cl_registers)} was provided [TypeError].")
        raise ResultError # I capture this error in QJob.result()
    
    return new_counts


def _recombine_probs(probs: Union[dict[np.array], np.array], partial: Union[None, list[int]], num_qubits: int):
    """
    Processes the probabilities per bitstring to obtain the probabilities per qubit. 

    Args:
        probs (np.array, dict[np.array]): one or more (dict case) set of probabilities per bitstring
        partial (None, list[int]): list of indexes of the qubits that determine the total probability space. Good for excluding ancillae
        num_qubits (int): number of qubits that determines the lenght of the bitstrings

    Returns:
        new_probs (np.array, dict[np.array]): probabilities or list of probabilities per qubit
    """
    if isinstance(probs, dict): # get a dict with probability arrays as values

        new_probs = {}
        for k, probs_k in probs.items():

            new_probs[k] = np.zeros((len(partial), 2))
            for base_ten_bitstring, prob in enumerate(probs_k):
                for i, i_qubit in enumerate(partial):

                    zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit]) # extract wether i have a zero or a one on position i_qubit of the bitstring
                    new_probs[k][i, zero_one] += prob # for each qubit, i have a two element list with prob of one and prob of zero. Which element should be updated
                                                        # is determined by the zero or one on the bitstring

    else: # probs is an array
        new_probs = np.zeros((len(partial), 2))

        # We assume that the bitstring probabilities are ordered from 0000 to 1111 following the binary order, thus the base_ten_bitstring
        for base_ten_bitstring, prob in enumerate(probs):
            for i, i_qubit in enumerate(partial):

                zero_one = int(format(base_ten_bitstring, f"0{num_qubits}b")[i_qubit]) #extract wether there is a "0" or "1" in i_qubit on the binary bitstring, eg 8 -> 1000 which on position 2 has a 0
                new_probs[i, zero_one] += prob

    return new_probs


def recombine_bistring_probs(probs: Union[dict[np.array], np.array], partial: list[int], num_qubits: int):
    """
    Recombine the probabilities per bitstring to show the probabilities per shortened bitstring where we only keep some of the qubits.

    Args:
        probs (dict[np.array], np.array): one or more (dict case) set of probabilities per bitstring
        partial (None, list[int]): list of indexes of the qubits that determine the total probability space
        num_qubits (int): number of qubits that determines the lenght of the bitstrings

    Returns:
        short_bitstring_probs (dict[np.array], np.array): one or more (dict case) set of probabilities per sub-bitstring
    """

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