"""
    Contains the :py:class:`~cunqa.result.Result` class, which gathers the information about the 
    output of a simulations, also other functions
    to manage them.

    Once we have submmited a :py:class:`~cunqa.qjob.QJob`, for obtaining its results we call for its 
    propperty :py:attr:`QJob.result`:

        >>> qjob.result
        <cunqa.result.Result object at XXXX>
    
    This object has two main attributes for out interest: the counts distribution from the 
    simulation and the time that the simulation took in seconds:

        >>> result = qjob.result
        >>> result.counts
        {'000':34, '111':66}
        >>> result.time_taken
        0.056
    

"""
from cunqa.logger import logger
from itertools import accumulate

class Result:
    """
    Class to describe the result of a simulation.

    It has two main attributes:

    - :py:attr:`Result.counts` : returns the distribution of counts from the sampling of the 
    simulation.

            >>> result.counts
            {'000':34, '111':66}

    .. note::
        If the circuit sent has more than one classical register, bit strings corresponding to each 
        one of them will be separated by blank spaces in the order they were added:

            >>> result.counts
            {'001 11':23, '110 10':77}
    
    - :py:attr:`Result.time_taken` : the time that the simulation took.

            >>> result.time_taken
            0.056

    Nevertheless, depending on the simulator used, more output data is provided. For checking all 
    the information from the simulation as a ``dict``, one can access the attribute 
    :py:attr:`Result.result`.

    If an error occurs at the simulation, an exception will be raised at the pyhton program, 
    :py:exc:`ResultError`.
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

            registers (dict): dictionary specifying the classical registers defined for the circuit. 
            This is neccessary for the correct formating of the counts bit strings.
        """

        self._result = {}
        self._id = circ_id
        self._registers = registers
        
        if result is None or len(result) == 0:
            raise ValueError(f"Empty object passed, result is {None} [{ValueError.__name__}].")
        elif "ERROR" in result:
            message = result["ERROR"]
            raise RuntimeError(f"Error during simulation, please check availability of QPUs, run "
                               f"arguments syntax and circuit syntax: {message}")
        else:
            self._result = result


    # TODO: Use length of counts to justify time_taken (ms) at the end of the line.
    def __str__(self):
        YELLOW = "\033[33m"
        RESET = "\033[0m"   
        GREEN = "\033[32m"
        return (f"{YELLOW}{self._id}:{RESET} {'{'}counts: {self.counts}, \n\t "
               f"time_taken: {GREEN}{self.time_taken} s{RESET}{'}'}\n")


    @property
    def result(self) -> dict:
        """Raw output of the simulation, the ``dict`` format depends on the simulator used."""
        return self._result
    

    @property
    def counts(self) -> dict:
        """
        Counts distribution from the sampling of the simulation, format is 
        ``{"<bit string>":<number of counts as int>}``.
        """
        if "results" in list(self._result.keys()): # aer
            counts = self._result["results"][0]["data"]["counts"]

        elif "counts" in list(self._result.keys()): # munich and cunqa
            counts = self._result["counts"]
        else:
            raise RuntimeError(f"The result format is unknown: no counts in it.")
        
        if len(self._registers) == 1:
            return counts
        
        logger.debug(f"Dividing strings into {len(lengths)} classical registers.")

        lengths = [len(reg) for reg in self._registers.values()]
        if not lengths:
            return counts
        cuts = (0, *accumulate(lengths))
        return {' '.join(bitstring[i:j] for i, j in zip(cuts, cuts[1:])): 
                count for bitstring, count in counts.items()}

    @property
    def time_taken(self) -> str:
        """
        Time that the simulation took in seconds, since it is recieved at the virtual QPU until 
        it is finished.
        """
        if "results" in list(self._result.keys()): # aer
            time = self._result["results"][0]["time_taken"]
            return time

        elif "counts" in list(self._result.keys()): # munich and cunqa
            time = self._result["time_taken"]          
            return time
        else:
            raise RuntimeError(f"The result format is unknown: no time_taken in it.")
        
    

