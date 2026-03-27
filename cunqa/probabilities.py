import logging
import copy
import math
import numpy as np
from typing import Union
from cunqa.logger import logger
from cunqa.counts_and_probs import counts_to_probs, recombine_probs, marginalize_counts # Implemented on C++ for speed

def probabilities(
        result, 
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
            result (<class cunqa.result.Result>): result object to extract probabilities from
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
            statevecs = result.statevector
            there_is_statevec = True
        except Exception:
            pass

        try:
            there_is_densmat = False
            densmats = result.density_matrix
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
                    for k, prob in probs.items():
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
                if isinstance(densmats, dict):
                    for k, prob in probs.items():
                        probs[k] = recombine_probs(prob, per_qubit, partial, num_qubits)
                else:
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits)
            
            return probs

        # State not available: estimate probabilities from count frequencies
        else: 
            logger.debug(f"Estimating probabilities from the available counts. First ten counts: "
                         f"{ {k: result.counts[k] for k in list(result.counts.keys())[:10]} }")
            
            if len(result._registers) > 1:
                logger.warning(f"Computing probabilities of a circuit with {len(result._registers)} "
                               f"classical registers. Lenght of probabilities may not correspond "
                               f"with 2^num_qubits.")

                if sep_registers:

                    lengths = [len(reg) for reg in reversed(result._registers.values())]
                    registers_probs = copy.deepcopy(result._registers)
                    for counts_reg, register_name in zip(marginalize_counts(result.counts, lengths), reversed(result._registers.keys())):
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
                n = len(next(iter(result.counts.keys())).replace(" ", ""))
                num_bitstrings = 2**n

                if len(result.counts) != num_bitstrings:
                    new_counts = {
                        **{f"{i:0{n}b}": 0 for i in range(num_bitstrings)}, 
                        **{key.strip(): value for key, value in result.counts.items()}
                        }
                else:
                    new_counts = result.counts

                probs = counts_to_probs(new_counts)

                if (per_qubit or partial is not None):
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits = n)

                return probs
            
            # Case with one register
            # If not all bitstrings are present, add them with count 0 (consistent with state vector and density matrix methods)
            num_qubits = len(next(iter(result.counts.keys())))
            num_bitstrings = 2**num_qubits
            if len(result.counts) != num_bitstrings:
                new_counts = {
                    **{f"{i:0{num_qubits}b}": 0 for i in range(num_bitstrings)}, **result.counts
                }

            else:
                new_counts = result.counts

            probs = counts_to_probs(new_counts)

            if (per_qubit or partial is not None):
                    probs = recombine_probs(probs, per_qubit, partial, num_qubits= num_qubits)

            return probs
