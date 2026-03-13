"""
    Contains the :py:class:`~cunqa.result.Result` class, which holds the output of the executions.

    Once we have submmited a :py:class:`~cunqa.qjob.QJob`, for obtaining its results we call for its 
    property :py:attr:`~cunqa.qjob.QJob.result`:

        >>> qjob.result
        <cunqa.result.Result object at XXXX>
    
    This object has two main attributes of interest: the counts distribution from the 
    simulation and the time that the simulation took in seconds:

        >>> result = qjob.result
        >>> result.counts
        {'000':34, '111':66}
        >>> result.time_taken
        0.056
"""
import logging
import copy
import math
import numpy as np

from typing import  Union
from cunqa.logger import logger
from cunqa.counts_and_probs import counts_to_probs, recombine_probs, marginalize_counts # Implemented on C++ for speed
from itertools import accumulate

class Result:
    """
    Class to describe the result of a simulation. 

    There are two main attributes, :py:attr:`Result.counts` and :py:attr:`Result.time_taken`, common
    to every simulator available on the backends.Nevertheless, depending on the simulator used, more
    output data is provided. For checking all the information from the simulation as a ``dict``, 
    one can access the attribute :py:attr:`Result.result`.

    .. autoattribute:: counts
    .. autoattribute:: time_taken
    .. autoattribute:: result
    """
    _result: dict
    id: str
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
        self.id = circ_id
        self._registers = registers
        
        if result is None or len(result) == 0:
            raise ValueError(f"Empty object passed, result is {None}.")
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
        return (f"{YELLOW}{self.id}:{RESET} {'{'}counts: {self.counts}, \n\t "
               f"time_taken: {GREEN}{self.time_taken} s{RESET}{'}'}\n")


    @property
    def result(self) -> dict:
        """
        Dictionary with the whole output of the simulation. This output is presented as the raw 
        product of the simulation, and so the ``dict`` format depends on the simulator used.
        """
        return self._result
    

    @property
    def counts(self) -> dict:
        """
        Counts distribution from the sampling of the simulation, format is 
        ``{"<bit string>":<number of counts as int>}``.

                >>> result.counts
                {'000':34, '111':66}

        .. note::
            If the circuit sent has more than one classical register, bit strings corresponding to 
            each one of them will be separated by blank spaces in the order they were added:

                >>> result.counts
                {'001 11':23, '110 10':77}
        """
        if "qmio_results" in list(self._result.keys()): 
            counts = self._result["qmio_results"]["c"] #TODO: More registers? 
        elif "results" in list(self._result.keys()): # aer
            counts = self._result["results"][0]["data"]["counts"]
        elif "counts" in list(self._result.keys()): # munich and cunqa
            counts = self._result["counts"]
        else:
            raise RuntimeError(f"The result format is unknown: no counts in it.")
        
        if len(self._registers) == 1:
            return counts

        # reversed to keep the order of counts keys
        lengths = [len(reg) for reg in reversed(self._registers.values())]
        if not lengths:
            return counts
        cuts = (0, *accumulate(lengths))
        return {' '.join(bitstring[i:j] for i, j in zip(cuts, cuts[1:])): 
                count for bitstring, count in counts.items()}

    @property
    def time_taken(self) -> str:
        """
        Time that the simulation took in seconds, since it is received at the virtual QPU until 
        it is finished.

            >>> result.time_taken
            0.056
        """
        if "qmio_results" in list(self._result.keys()):
            time = self._result["time_taken"]
            return time
        elif "results" in list(self._result.keys()): # aer
            time = self._result["results"][0]["time_taken"]
            return time
        elif "counts" in list(self._result.keys()): # munich and cunqa
            time = self._result["time_taken"]          
            return time
        else:
            raise RuntimeError(f"The result format is unknown: no time_taken in it.")
        
    @property
    def statevector(self) -> Union[dict[np.array], np.array]:
        """
        Statevector or dictionary of statevectors captured at the moment that `.save_state()` was 
        performed on the circuit.
        """
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
                raise RuntimeError(f"Statevector not found, try using circuit.save_state() at some "
                                   f"point before executing.")
                

        except Exception as error:
            raise RuntimeError(f"Some error occured with Statevector "
                               f"[{type(error).__name__}]: {error}.")
        
        return statevector


    @property
    def density_matrix(self) -> Union[dict[np.array], np.array]:
        """
        Density matrix or dictionary of density matrices captured at the moment that `.save_state()` 
        was performed on a circuit runned with `method = density_matrix` on AER.
        """
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
                raise RuntimeError(f"Density Matrix not found, try using circuit.save_state() "
                                   f"before executing with Aer.")

        except Exception as error:
            raise RuntimeError(f"Some error occured with Density Matrix "
                               f"[{type(error).__name__}]: {error}.")
        
        return density_matrix
    
    def probabilities(
        self, 
        per_qubit: bool = False, 
        partial: list[int] = None,
        sep_registers: bool = False,
        estimate: bool = False
    ) -> Union[dict[np.ndarray],  np.ndarray]:
        """
        Extracts probabilities from result information. If we have statevector or density matrix 
        exact probabilities are obtained, otherwise frequencies are calculated from counts.

        .. note:: 
            
            If several states (statevector or density matrix) are saved, a dict with keys the
            state labels and values the probability arrays is returned.

        Args:
            per_qubit (bool): selects probabilities per bitstring (False) or per qubit (True).
                              If True, an 2D (num_qubits x 2)-array with the probability of 
                              "0" and "1" for each qubit as rows, where the qubit index 
                              decreases with row index, is returned. Default: False.
            partial (list[int]): list of indices of significant qubits. If given, probabilities are
                                 marginalized w/ respect to those qubits. Good for excluding ancillae.
            sep_registers (bool): if True, a separate probs array is returned for each cl_register
                                  on the estimation from counts mode. If additionally the partial 
                                  option is desired, it should be of type list[list[int]] 
                                  or behave analogously. Default: False
            estimate (bool): if True, probabilities are estimated from counts regardless of the 
                             presence of statevector or density matrix to pull them from. Default: False

        Returns:
            probs (dict, np.ndarray): probabilities per bitstring or per qubit. Order is as follows:
                                      - Per bitstring: probability on each position is associated to
                                        the corresponding bitstring in ascending binary order, ie
                                        for 2 qubits ["00", "01", "10", "11"].
                                      - Per qubit: probability of "0" and "1" for each qubit 
                                        where qubit 0 is the last row and the qubit index increases 
                                        as row index decreases.
                                      Partial preserves these orders in both cases.  
                                     
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
        
        # Logging re-enabled 
        logging.disable(logging.NOTSET)

        # Statevector
        if (there_is_statevec and not estimate):
            logger.debug("Extracting probabilities from statevector.")
            if isinstance(statevecs, dict):
                probs={}
                for k, statevec in statevecs.items():
                    probs[k] = np.reshape(np.power(np.abs(statevec), 2), np.shape(statevec)[0])
                # Extract number of qubits from the lenght of one of the sets of probs
                num_qubits= int(math.log2(next(iter(probs.values())).size))

            else:
                probs = np.reshape(np.power(np.abs(statevecs), 2), np.shape(statevecs)[0])
                num_qubits= int(math.log2(probs.size))

            if (per_qubit or partial is not None):
                if isinstance(statevecs, dict):
                    for k, prob in probs.values():
                        probs[k] = recombine_probs(prob, per_qubit, partial, num_qubits)
                else:
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits)

            return probs

        # Density matrix
        elif (there_is_densmat and not estimate):
            logger.debug("Extracting probabilities from density_matrix.")
            if isinstance(densmats, dict):
                probs = {}
                for k, densmat in densmats.items():
                    probs[k] = np.diagonal(densmat, axis1=0).real[0].copy()
                # Extract number of qubits from the lenght of one of the sets of probs
                num_qubits = int(math.log2(next(iter(probs.values())).size)) 

            else:
                probs =  np.diagonal(densmats, axis1=0).real[0].copy()
                num_qubits = int(math.log2(probs.size))

            if (per_qubit or partial is not None):
                if isinstance(statevecs, dict):
                    for k, prob in probs.values():
                        probs[k] = recombine_probs(prob, per_qubit, partial, num_qubits)
                else:
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits)
            
            return probs

        # State not available: estimate probabilities from count frequencies
        else: 
            logger.debug(f"Estimating probabilities from the available counts. First ten counts: "
                         f"{ {k: self.counts[k] for k in list(self.counts.keys())[:10]} }")
            
            if len(self._registers) > 1:
                logger.warning(f"Computing probabilities of a circuit with {len(self._registers)} "
                               f"classical registers. Lenght of probabilities may not correspond "
                               f"with 2^num_qubits.")

                if sep_registers:

                    lengths = [len(reg) for reg in reversed(self._registers.values())]
                    registers_probs = copy.deepcopy(self._registers)
                    for counts_reg, register_name in zip(marginalize_counts(self.counts, lengths), reversed(self._registers.keys())):
                        n = len(next(iter(counts_reg.keys())))
                        # Add zero counts if necessary
                        if len(counts_reg) != 2**n:
                            counts_reg = {**{f"{i:0{n}b}": 0 for i in range(2**n)}, **counts_reg}

                        registers_probs[register_name] = counts_to_probs(counts_reg)

                    if (per_qubit or partial is not None):
                        for key, value, partial_i in zip(registers_probs.items(), partial):
                            registers_probs[key] = recombine_probs(value, per_qubit, partial_i, num_qubits= math.log2(value.size()))

                    return registers_probs

                # All registers together
                n = len(next(iter(self.counts.keys())).replace(" ", ""))
                num_bitstrings = 2**n

                if len(self.counts) != num_bitstrings:
                    new_counts = {
                        **{f"{i:0{n}b}": 0 for i in range(num_bitstrings)}, 
                        **{key.strip(): value for key, value in self.counts.items()}
                        }
                else:
                    new_counts = self.counts

                probs = counts_to_probs(new_counts)

                if (per_qubit or partial is not None):
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits = n)

                return probs
            
            # Case with one register
            # If not all bitstrings are present, add them with count 0 (consistent with state vector and density matrix methods)
            num_qubits = len(next(iter(self.counts.keys())))
            num_bitstrings = 2**num_qubits
            if len(self.counts) != num_bitstrings:
                new_counts = {
                    **{f"{i:0{num_qubits}b}": 0 for i in range(num_bitstrings)}, **self.counts
                }

            else:
                new_counts = self.counts

            probs = counts_to_probs(new_counts)

            if (per_qubit or partial is not None):
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits= num_qubits)

            return probs
