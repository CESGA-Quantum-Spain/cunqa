"""Contains the Result class, which deals with the output of QJobs using any simulator."""
from cunqa.logger import logger

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
            self._result = result
        
        #logger.debug("Results correctly loaded.")


    # TODO: Use length of counts to justify time_taken (ms) at the end of the line.
    def __str__(self):
        RED = "\033[31m"
        YELLOW = "\033[33m"
        RESET = "\033[0m"   
        GREEN = "\033[32m"
        return f"{YELLOW}{self._id}:{RESET} {'{'}counts: {self.counts}, \n\t time_taken: {GREEN}{float(self.time_taken)/1e3} ms{RESET}{'}'}\n"


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
            
            #counts = convert_counts(counts, self._registers)   #TODO

        except Exception as error:
            logger.error(f"Some error occured with counts [{type(error).__name__}]: {error}.")
            raise error
        return counts


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
        
        # counting number of classical bits
        num_clbits = sum([len(i) for i in registers.values()])
        # getting lenghts of bits for the different registers
        lengths = []
        for v in registers.values():
            lengths.append(len(v))
    else:
        logger.error(f"Error when converting `counts` strings.")
        raise ResultError # I capture this error in QJob.result()

    if isinstance(counts, dict):
        res_counts = {}
        for k,v in counts.items():
            if k.startswith('0x'): # converting to binary string and dividing in bit strings
                new_counts[divide(format( int(k, 16), '0'+str(num_clbits)+'b' ), lengths)]= v
            else: # just dividing the bit stings
                new_counts[divide(k, lengths)] = v
    elif isinstance(counts, list):
        for count in counts:
            new_counts[divide(format(count[0],'0'+str(num_clbits)+'b')[::-1], lengths)] = count[1] 
    return new_counts
