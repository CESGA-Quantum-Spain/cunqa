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
    _registers: dict
    
    def __init__(self, result: dict, circ_id: str, registers: dict):
        """
        Initializes the Result class.

        Args:
            result (dict): dictionary given as the output of the simulation.

            circ_id (str): circuit identificator.

            registers (dict): dictionary specifying the classical registers defined for the circuit. This is neccessary for the correct formating of the counts bit strings.
        """

        self._result = {}
        self._id = circ_id
        self._registers = registers
        
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
            if "qmio_results" in list(self._result.keys()): 
                counts = self._result["qmio_results"]["meas"] #TODO: More registers?
            elif "results" in list(self._result.keys()): # aer
                counts = self._result["results"][0]["data"]["counts"]
            elif "counts" in list(self._result.keys()): # munich and cunqa
                counts = self._result["counts"]
            else:
                logger.error(f"Some error occured with counts.")
                raise ResultError
            
            if len(self._registers) > 1:
                counts = _convert_counts(counts, self._registers)

        except Exception as error:
            logger.error(f"Some error occured with counts [{type(error).__name__}]: {error}.")
            raise error
        
        return counts

    @property
    def time_taken(self) -> str:
        """Time that the simulation took in seconds, since it is recieved at the virtual QPU until it is finished."""
        try:
            if "qmio_results" in list(self._result.keys()):
                time = self._result["time_taken"]
                return time
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


def _convert_counts(counts: dict, registers: dict) -> dict:

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
            new_counts[_divide(k, lengths)] = v
    else:
        logger.error(f"counts must be dict, but {type(registers)} was provided [TypeError].")
        raise ResultError # I capture this error in QJob.result()
    
    return new_counts

