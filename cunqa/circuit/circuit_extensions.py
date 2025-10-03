"""
Holds dunder methods for the class CunqaCircuit and other functions to extract information from it.
"""
from typing import Union, Optional, Tuple
import copy

from cunqa.logger import logger
from cunqa.circuit.circuit import CunqaCircuit, _flatten, SUPPORTED_GATES_DISTRIBUTED, SUPPORTED_GATES_1Q
from cunqa.circuit.converters import _qc_to_json

from qiskit import QuantumCircuit
import matplotlib.pyplot as plt #I use this for drawing circuits with LaTeX (quantikz)
from matplotlib import rc

class HorizontalDivisionError(Exception):
    """ Signals that a gate for which we haven't defined a division was found while trying to horizontally divide a circuit."""
    pass

def cunqa_dunder_methods(cls):

    # ================ CIRCUIT MODIFICATION METHODS ==============

    def _update_other_instances(self, instances_to_change, other_id, comb_id, displace_n = 0): 
        """ 
        Private method called from the __add__ and __or__ methods and its variations. It modifies the instructions of any other CunqaCircuit instance that references
        self or other, the circuits involved on the operation, to reference the combined circuit instead.
        
        Args:
            instances_to_change (set): set with the ids of the circuits referencing self or other, to change the reference to the combined circuit.
            other_id (str): id of the second circuit in the operation
            comb_id (str): id to substitute in on the instructions of circuits referencing the operands
            displace_n (int): specifies the number of qubits of the upper circuit on a union of circuits. It will displace the qubits of the lower circuit by
                              this amount when necessary.
        """
        other_instances = self.__class__.access_other_instances() 

        for circuit in instances_to_change: 
            instance = other_instances[circuit]

            if displace_n != 0:
                for instr in (iterator_instrs := iter(instance.instructions)):
                    if ("circuits" in instr and instr["circuits"][0] == other_id): # Notice here we only check for other_id to not displace those of self
                        instr["circuits"] = [comb_id]

                        if instr["name"] == "recv":
                            instr["remote_conditional_reg"][0] += displace_n
                            instr2 = next(iterator_instrs)
                            instr2["remote_conditional_reg"] += displace_n
                
            else:
                for instr in instance.instructions:
                    if ("circuits" in instr and instr["circuits"][0] in [self._id, other_id]):
                        instr["circuits"] = [comb_id]
    

    ######################## SUM ########################

    def __add__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit], force_execution = False) -> 'CunqaCircuit':
        """
        Overloading the "+" operator to perform horizontal concatenation. This means that summing two CunqaCircuits will return a circuit that
        applies the operations of the first circuit and then those of the second circuit. Not a commutative operation.

        Args
            other_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be horizontally concatenated after self.
            force_execution (bool): disallows the check that raises an error if both circuits are distributed (version 1).
        Returns
            summed_circuit (<class.cunqa.circuit.CunqaCircuit>): circuit with instructions from both summands.
        """
        if other_circuit == 0:
            return copy.deepcopy(self)  
        
        if (n := self.num_qubits) == other_circuit.num_qubits:        
            circs_comm_summands = set() # If our circuits have distributed gates, as the id changes, we will need to update it on the other circuits that communicate with this one

            if isinstance(other_circuit, CunqaCircuit):
                if not force_execution:
                    if any([(self._id in path and other_circuit._id in path) for path in self.__class__.get_connectivity()]):
                        logger.error(f"Circuits to sum are connected, directly or through a chain of other circuits. This could result in execution waiting forever.\n If you're sure this won't happen try the syntax CunqaCircuit.sum(circ_1, circ_2, force_execution = True).")
                        raise SystemExit
                    
                other_instr = other_circuit.instructions
                other_id = other_circuit._id  
                circs_comm_summands.union({instr["circuits"][0] for instr in other_instr if "circuits" in instr})   

            elif isinstance(other_circuit, QuantumCircuit):
                other_instr = qc_to_json(other_circuit)['instructions']
                other_id = "qc"
                
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
            
            self_instr = self.instructions
            circs_comm_summands.union({instr["circuits"][0] for instr in self_instr if "circuits" in instr})

            cl_n = self.num_clbits
            cl_m = other_circuit.num_clbits
            sum_id = self._id + " + " + other_id
            summed_circuit = CunqaCircuit(n, max(cl_n, cl_m), id = sum_id) 

            summed_circuit.quantum_regs = self.quantum_regs     # The registers of the first circuit are kept
            summed_circuit.classical_regs = self.classical_regs

            for instruction in list(self_instr + other_instr):
                summed_circuit._add_instruction(instruction)

            self._update_other_instances(circs_comm_summands, other_id, sum_id)
            
            return summed_circuit
        
        else:
            logger.error(f"First version only accepts summing circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
            raise SystemExit

    def __radd__(self, left_circuit: Union['CunqaCircuit', QuantumCircuit])-> 'CunqaCircuit':
        """
        Overloading the "+" operator to perform horizontal concatenation. In this case circ_1 + circ_2 is interpreted as circ_2.__radd__(circ_1). 
        Implementing it ensures that the order QuantumCircuit + CunqaCircuit also works, as their QuantumCircuit.__add__() only accepts QuantumCircuits.

        Args
            left_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be horizontally concatenated before self.
        Returns
            summed_circuit (<class.cunqa.circuit.CunqaCircuit>): circuit with instructions from both summands. 
        """
        if left_circuit == 0:
            return copy.deepcopy(self)

        
        if  (n := self.num_qubits) == left_circuit.num_qubits:
            if isinstance(left_circuit, CunqaCircuit):
                return left_circuit.__add__(self)

            elif isinstance(left_circuit, QuantumCircuit):
                left_id = "qc"
                sum_id = left_id + " + " + self._id

                cl_n=self.num_clbits; cl_m=left_circuit.num_clbits
                summed_circuit = CunqaCircuit(n, max(cl_n, cl_m), id = sum_id)

                summed_circuit.quantum_regs = self.quantum_regs     # The registers of the CunqaCircuit are kept (can be changed later)
                summed_circuit.classical_regs = self.classical_regs   

                left_instr = qc_to_json(left_circuit)['instructions']
                
                for instruction in list(left_instr + self.instructions):
                    summed_circuit._add_instruction(instruction)

                circs_comm_self = {instr["circuits"][0] for instr in self.instructions if "circuits" in instr}
                self._update_other_instances(circs_comm_self, left_id, sum_id) # Update other circuits that communicate with self to reference the summed_circuit

                return summed_circuit
        
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(left_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
                    
        else:
            logger.error(f"Only possible to sum circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
            raise SystemExit

    def __iadd__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit], force_execution = False):
        """
        Overloading the "+=" operator to concatenate horizontally the circuit self with other_circuit. This means adding the operations from 
        the other_circuit to self. No return as the modifications are performed locally on self.
        Args
            other_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be horizontally concatenated after self.
            force_execution (bool): disallows the check that raises an error if both circuits are distributed (version 1).
        """
        if other_circuit == 0:
            return self

        if  self.num_qubits == other_circuit.num_qubits:
            if (cl_m := other_circuit.num_clbits) > self.num_clbits:

                self._add_cl_register(name = other_id, number_clbits = cl_m - self.num_clbits)

            circs_comm_summands = set()
            if isinstance(other_circuit, CunqaCircuit):
                if not force_execution:
                    if any([(self._id in connection and other_circuit._id in connection) for connection in self.__class__.get_connectivity()]):
                        logger.error(f"Circuits to sum are connected, directly or through a chain of other circuits. This could result in execution waiting forever.\n If you're sure this won't happen try the syntax CunqaCircuit.sum(circ_1, circ_2, force_execution = True).")
                        raise SystemExit
                    
                other_instr = other_circuit.instructions
                other_id = other_circuit._id
                circs_comm_summands.union({instr["circuits"][0] for instr in other_instr if "circuits" in instr})

            elif isinstance(other_circuit, QuantumCircuit):
                other_instr = qc_to_json(other_circuit)['instructions']
                other_id = "qc" # Avoids an error on _update_other_instances execution
                
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
            
            
            for instruction in list(other_instr):
                self._add_instruction(instruction)

            if self._id in circs_comm_summands:
                logger.error("The circuits to be summed contain distributed instructions that reference eachother.")
                raise SystemExit
            self._update_other_instances(circs_comm_summands, other_id, self._id)
        
        else:
            logger.error(f"Only possible to sum circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
            raise SystemExit

        return self
            
    @classmethod
    def sum(cls, iterable, start = CunqaCircuit(0, id = "Empty"), force_execution = False):
        """
        Custom sum function that supports additional arguments
        
        Args:
            iterable: Circuits to sum
            start: Starting value (default CunqaCircuit(0, id = "Empty"))
            force_execution: Optional argument to pass to __add__()
        """
        it = iter(iterable)
        try:
            total = next(it)
        except StopIteration:
            return start
        
        for element in it:
            total = total.__add__(element, force_execution=force_execution)
        
        return total


    ######################## UNION ########################

    def __or__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit])-> 'CunqaCircuit':
        """
        Overloading the "|" operator to perform vertical concatenation. This means that taking the union of two CunqaCircuits with n and m qubits
        will return a circuit with n + m qubits where the operations of the first circuit are applied to the first n and those of the second circuit
        will be applied to the last m. Not a commutative operation.

        Args
            other_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be vertically concatenated next to self.
        Returns
            union_circuit (<class.cunqa.circuit.CunqaCircuit>): circuit with both input circuits one above the other.
        """

        if isinstance(other_circuit, CunqaCircuit):
            other_instr = other_circuit.instructions
            other_id = other_circuit._id

        elif isinstance(other_circuit, QuantumCircuit):
            other_instr = qc_to_json(other_circuit)['instructions']
            other_id = "qc" 
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit
        
        n    = self.num_qubits;           m = other_circuit.num_qubits
        cl_n = self.num_clbits;        cl_m = other_circuit.num_clbits
        union_id = self._id + " | " + other_id
        union_circuit = CunqaCircuit(n + m, cl_n + cl_m, id = union_id)

        # Traverse input circuits instructions to change distributed gates between them for local gates and record circuits that communicate with them to modify the reference to point to the union circuit
        circs_comm_self = set() 
        for instr in (iter_self_instructions := iter(copy.deepcopy(self.instructions))):
            # If we find distributed gates referencing self, substitute by a local gate
            if ("circuits" in instr and instr["circuits"][0] == other_id): 
                if instr["name"] == "measure_and_send":
                    continue # These ones will be substituted later

                elif instr["name"] == "recv":
                    instr2 = next(iter_self_instructions) # This retrieves the next gate and skips the recv
                    print(instr2)
                    instr2["conditional_reg"] = instr2["remote_conditional_reg"] + n
                    del instr2["remote_conditional_reg"]

                    union_circuit._add_instruction(instr2)
                    continue

                else: # ["qsend", "qrecv", "expose", "rcontrol"]
                    # TODO: support these
                    pass

                circs_comm_self.append(instr["circuits"][0])

            union_circuit._add_instruction(instr)


        circs_comm_other = set()
        for instrr in (iter_other_instr := iter(copy.deepcopy(other_instr))):

            instrr["qubits"] = [qubit + n for qubit in instrr["qubits"]] # displace the qubits of the instructions and then add it to union_circuit
            if "clbits" in instrr:
                instrr["clbits"] = [clbit + cl_n for clbit in instrr["clbits"]]

            if "conditional_reg" in instrr:
                instrr["conditional_reg"][0] += cl_m

            if "registers" in instrr:
                instrr["registers"] = [qubit + m for qubit in instrr["registers"]]

            if "circuits" in instrr: # Gather info on the circuits that reference the other_circuit and wether it controls or is a target

                if instrr["circuits"][0] == self._id: # Susbtitute distr gate by local gates if communicates with upper_circuit 
                    if instrr["name"] == "measure_and_send":
                        continue # These ones have been substituted earlier

                    elif instrr["name"] == "recv":
                        instr2 = next(iter_other_instr) # This retrieves the next gate and skips the recv
                        
                        instr2["qubits"] = [q + n for q in instr2["qubits"]] 
                        instr2["conditional_reg"] = instr2["remote_conditional_reg"]
                        del instr2["remote_conditional_reg"]

                        union_circuit._add_instruction(instr2)
                        continue

                    else: # ["qsend", "qrecv", "expose", "rcontrol"]
                        # TODO: support these
                        pass
                    
                circs_comm_other.append(instrr["circuits"][0])
            
            union_circuit._add_instruction(instrr)

        # Update other circuits that communicate with our input circuits to reference the union_circuit
        self._update_other_instances(circs_comm_self, self._id, union_id) 
        self._update_other_instances(circs_comm_other, other_id, union_id, n)

        return union_circuit
        
    def __ror__(self, upper_circuit: Union['CunqaCircuit', QuantumCircuit])-> 'CunqaCircuit':
        """
        Overloading the "|" operator to perform vertical concatenation. In this case circ_1 | circ_2 is interpreted as circ_2.__ror__(circ_1). 
        Implementing it ensures that the order QuantumCircuit | CunqaCircuit also works.

        Args
            upper_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be vertically concatenated above self.
        Returns
            union_circuit (<class.cunqa.circuit.CunqaCircuit>): circuit with both input circuits one above the other. 
        """
        if isinstance(upper_circuit, CunqaCircuit):
            return upper_circuit.__or__(self)

        elif isinstance(upper_circuit, QuantumCircuit):
            upper_instr = qc_to_json(upper_circuit)['instructions']
            upper_id ="qc"
            union_id = self._id + " | " + upper_id

            n    = self.num_qubits;          m = upper_circuit.num_qubits
            cl_n = self.num_clbits;       cl_m = upper_circuit.num_clbits
            union_circuit = CunqaCircuit(n+m, cl_n+cl_m, id = union_id)

            for instr in upper_instr: # QuantumCircuits don't have distributed instructions, we're in the simple case
                union_circuit._add_instruction(instr)


            circs_comm_self = set() # Here we will collect info on the circuits that talk to self to make them reference the union instead
            for instrr in copy.deepcopy(self.instructions):

                instrr["qubits"] = [qubit + m for qubit in instrr["qubits"]]
                if "clbits" in instrr:
                    instrr["clbits"] = [clbit + cl_m for clbit in instrr["clbits"]] 

                if "conditional_reg" in instrr:
                    instrr["conditional_reg"][0] += cl_m

                if "registers" in instrr:
                    instrr["registers"] = [qubit + m for qubit in instrr["registers"]]

                if 'circuits' in instrr: # Gather info on the circuits that reference upper_circuit and wether it controls or is a target

                    circs_comm_self.append(instrr["circuits"][0])

                union_circuit._add_instruction(instrr)

            self._update_other_instances(circs_comm_self, self._id, union_id, n)

            return union_circuit
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(upper_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit
        
            
        
    def __ior__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit]):
        """
        Overloading the "|=" operator to perform vertical concatenation. This means adding the qubits and their instructions from 
        other_circuit to self. No return as the modifications are performed locally on self.

        Args
            upper_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be vertically concatenated below self.
        Returns
            union_circuit (<class.cunqa.circuit.CunqaCircuit>): circuit with both input circuits one above the other. 
        """
        if isinstance(other_circuit, CunqaCircuit):
            other_instr = other_circuit.instructions
            other_id = other_circuit._id

        elif isinstance(other_circuit, QuantumCircuit):
            other_instr = qc_to_json(other_circuit)['instructions']
            other_id = "qc"
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit

        n = self.num_qubits;             cl_n = self.num_clbits;             need_to_modify_self = False
        m = other_circuit.num_qubits;    cl_m = other_circuit.num_clbits

        self._add_q_register(name = other_id, number_qubits = m)
        self._add_cl_register(name = other_id, number_clbits = cl_m)

        circs_comm_other = set() # Here we will collect info on the circuits that talk to other_circuit to make them reference self instead
        for instrr in (iter_other_instr := iter(copy.deepcopy(other_instr))):

            instrr["qubits"] = [qubit + n for qubit in instrr["qubits"]]
            if "clbits" in instrr:
                instrr["clbits"] = [clbit + cl_n for clbit in instrr["clbits"]]

            if "conditional_reg" in instrr:
                instrr["conditional_reg"][0] += cl_m

            if "registers" in instrr:
                instrr["registers"] = [qubit + m for qubit in instrr["registers"]]

            if "circuits" in instrr: # Treat distributed gates
                if instrr["circuits"][0] == self._id: # Susbtitute distr gate by local gates if it refences upper_circuit 
                    need_to_modify_self = True
                    if instrr["name"] == "measure_and_send":
                        continue # These ones will be substituted later

                    elif instrr["name"] == "recv":
                        instr2 = next(iter_other_instr) # This retrieves the next gate and skips the recv
                        instr2["qubits"] = [q + n for q in instr2["qubits"]] 
                        instr2["conditional_reg"] = instr2["remote_conditional_reg"]
                        del instr2["remote_conditional_reg"]

                        self._add_instruction(instr2)
                        continue

                    else: # ["qsend", "qrecv", "expose", "rcontrol"]
                        # TODO: support these
                        pass

                circs_comm_other.append(instrr["circuits"][0])
                
            self._add_instruction(instrr)
        
        if need_to_modify_self:
            for instr in (iter_self_instructions := iter(self.instructions)):
                if ("circuits" in instr and instr["circuits"][0] == other_id):
                    if instr["name"] in ["measure_and_send", "expose"]:
                        self.instructions.pop(instr) # Eliminate if from self, it has been substituted by a local gate earlier

                    elif instr["name"] == "recv":
                        instr2 = next(iter_other_instr) # This retrieves the next gate and skips the recv
                        instr2["conditional_reg"] = instr2["remote_conditional_reg"]
                        del instr2["remote_conditional_reg"]

                        self._add_instruction(instr2)
                        continue
                    
                    else: # ["qsend", "qrecv", "expose", "rcontrol"]
                        # TODO: support these
                        pass


        self._update_other_instances(circs_comm_other, other_id, self._id, n)

        return self
        

    @classmethod
    def union(cls, instances):
        # Implements a chainable union across multiple instances
        if not instances:
            return cls(0) # Return empty instance if an empty iterator is given
        
        # Start with the first instance
        result = instances[0]
        
        # Chain through remaining instances using __or__
        for instance in instances[1:]:
            result = result | instance
        
        return result

    ################ CIRRCUIT DIVIDING METHODS ##################


    def vert_split(self, position: int) -> Tuple["CunqaCircuit", "CunqaCircuit"]:
        """Divides a circuit vertically in two, separating all instructions up to and after a certain layer."""
        n_qubits = self.num_qubits; n_clbits=self.num_clbits

        left_id = self._id + f" left of {position}"
        right_id = self._id + f" right of {position}"

        left_circuit = CunqaCircuit(n_qubits, n_clbits, id=left_id)
        right_circuit = CunqaCircuit(n_qubits, n_clbits, id=right_id)

        left_instrs = []; right_instrs = []
        for qubit_instrs in self.layers.values():
            for instr in qubit_instrs:
                if instr[0] > position:
                    right_instrs.append(self.instructions[instr[1]])
                else:
                    left_instrs.append(self.instructions[instr[1]])

        left_circuit.from_instructions(left_instrs)
        right_circuit.from_instructions(right_instrs)

        return left_circuit, right_circuit

    def hor_split(self, n: int) -> Tuple["CunqaCircuit", "CunqaCircuit"]:
        """Divides a circuit horizontally in two, separating the first n qubits from the last num_qubits-n qubits. """
        rest = self.num_qubits - n - 1
        cl_n = self.num_clbits
        # TODO: maybe handle cl_bits more gracefully (will be complex and annoying)
        
        up_id = self._id + f" up_{n}"
        down_id = self._id + f" down_{rest}"

        upper_circuit = CunqaCircuit(n + 1, cl_n, id=up_id)
        lower_circuit = CunqaCircuit(rest, cl_n, id=down_id)

        # Add instructions to the two pieces according to their position
        for instr in copy.deepcopy(self.instructions):
            # ONE QUBIT GATES
            if instr["name"] in SUPPORTED_GATES_1Q:
                if instr["qubits"][0] > n:
                    instr["qubits"][0] -= (n+1) # Note that due to 0-indexing the number of qubits of the upper_circuit will be n+1 
                    lower_circuit._add_instruction(instr)
                else:
                    upper_circuit._add_instruction(instr)

            # MORE THAN ONE QUBIT, ALL IN ONE SIDE   
            elif all([q > n for q in instr["qubits"]]):
                instr["qubits"] = [q - n - 1 for q in instr["qubits"]]
                lower_circuit._add_instruction(instr)

            elif all(q < n+1 for q in instr["qubits"]):
                upper_circuit._add_instruction(instr)

            # MORE THAN ONE QUBIT, PARTITIONED. Gates here need to be made distributed (classical)
            else: 
            # TODO: consider case where more than one qubit fall on both sides
                if instr["name"] == "swap":
                    up_qubit, down_qubit = (instr["qubits"][0], instr["qubits"][1]- n - 1) if instr["qubits"][0] > n else (instr["qubits"][1],  instr["qubits"][0] - n - 1)
                    # Swap decomposes as three cnots, make them telegate
                    with upper_circuit.expose(up_qubit, lower_circuit) as rcontrol:
                        lower_circuit.cx(rcontrol, down_qubit)

                    with lower_circuit.expose(down_qubit, upper_circuit) as rcontrol:
                        upper_circuit.cx(rcontrol, up_qubit)

                    with upper_circuit.expose(up_qubit, lower_circuit) as rcontrol:
                        lower_circuit.cx(rcontrol, down_qubit)

                elif instr["name"] == "cswap":
                
                elif instr["name"] == ["unitary"]:
                    logger.error(f"It is not a priori clear how to divide the provided instruction: {instr}")
                    raise SystemExit

                elif instr["name"] == "save_state":
                    upper_circuit.save_state()
                    lower_circuit.save_state()

                elif instr["name"] == "rzz":
                    if instr["qubits"][0] < instr["qubits"][1]:

                    else:
                    up_param, donw_param = (instr["params"], -instr["params"]) if qb[0] < qb[1] else (-instr["params"], instr["params"])
                    upper_circuit.rz()
                    lower_circuit.rz()

                elif instr["name"] == "rxx":

                elif instr["name"] == "ryy":

                elif instr["name"] == "rzx":

                elif instr["name"] == "ecr":

                elif instr["name"] == "cu":

                elif instr["name"] == "cu1":

                elif instr["name"] ==  "cu3":


                # if instr["qubits"][0] > n: # Down sends to up
                #     # TODO: some qubits need to be displaced back by n
                #     down_instrs.append({
                #         "name": "measure_and_send",
                #         "qubits": _flatten(instr["qubits"]),
                #         "circuits": [up_id]
                #     })
                #     up_instrs.append({
                #         "name": instr["name"],
                #         "qubits": _flatten(instr["qubits"][1:]),
                #         "params": instr["params"],
                #         "circuits": [down_id]
                #     })

                # else:                      # Up sends to down
                #     up_instrs.append({
                #         "name": "measure_and_send",
                #         "qubits": _flatten(instr["qubits"]),
                #         "circuits": [down_id]
                #     })
                #     down_instrs.append({
                #         "name": instr["name"],
                #         "qubits": _flatten(instr["qubits"][1:]),
                #         "params": instr["params"],
                #         "circuits": [up_id]
                #     })

        return upper_circuit, lower_circuit


    ######################## CIRCUIT INFO ########################

    def __len__(self): 
        """Returns the layer depth of the circuit."""
        self.layers
        return max(self.last_active_layer)

    def param_info(self):
        """
        Provides information about variable parametric gates: their labels and multiplicity.

        Returns:
            lenghts (dict[int]): dictionary with keys the names of the parameters of the circuit and the number of times they appear as values.
        """
        labels = self.param_labels
        lenghts = {param: labels.count(param) for param in set(labels)}

        return lenghts

    def index(self, gate, multiple = False):
        """
        Method to determine the position of a certain gate.

        Args
            gate (str, dict): gate to be found 
            multiple (bool): option to return the position of all instances of the gate on the circuit instead of just the first one.
        Returns
            index (int, list[int]): position of the gate (index on the list self.instructions)
        """

        if isinstance(gate, str):
            if multiple:
                index = [i for i, instr in enumerate(self.instructions) if instr["name"] == gate]
                if len(index) != 0:
                    return index
                else:
                    raise ValueError
            else:
                for instr in self.instructions:
                    if instr["name"] == gate:
                        return self.instructions.index(instr)
                
        elif isinstance(gate, dict):
            self._check_instruction(gate)
            if multiple:
                index = [i for i, instr in enumerate(self.instructions) if all([instr[k] == gate[k] for k in gate.keys()])]
                if len(index) != 0:
                    return index
                else:
                    raise ValueError
            else:
                for instr in self.instructions:
                    if all(instr[k] == gate[k] for k, _ in gate.items()):
                        return self.instructions.index(instr)
                    
        else:
            logger.error(f"Gate should be str (its name) or dict, but {type(gate)} was provided [{TypeError.__name__}].")
            raise SystemExit



    def __contains__(self, gate):
        """Method to check if a certain gate or sequence of gates is present in the circuit instructions.
            Implemented by overloading the "in" operator.

        Args:
            gate (str, dict): gate or sequence of gates to check for presence in the circuit.

        Returns
            (bool): True if gate is on the circuit, False otherwise
        """
        try:
            self.index(gate)
            return True
        
        except ValueError:
            return False
        
        except Exception as e:
            logger.error(repr(e))
            raise SystemExit

        return False
        
    ######### TODO: change getitem and setitem to work with layers

    def __getitem__(self, indexes):
        """
        Returns the gates on the positions given by the input indexes. Overloads the "[ ]" operator. The circuit is interpreted as a list of instructions on a certain order.

        Args
            indexes (int, list[int]): positions to be queried. The circuit is 0-indexed, as usual.
        Returns
            gates (dict, list[dict]): objects found on the input indexes of the circuit. 
        """
        if isinstance(indexes, list):
            gates = []
            for index in indexes:
                if isinstance(index, int):
                    gates.append(self.instructions[index])
                else:
                    logger.error(f"Indexes must be ints, but a {type(index)} was provided [{TypeError.__name__}].")
                    raise SystemExit
            return gates
        
        elif isinstance(indexes, int):
            return self.instructions[indexes]
        
        else:
            logger.error(f"Indexes must be list[int] or int, but {type(indexes)} was provided [{TypeError.__name__}].")
            raise SystemExit

    def __setitem__(self, indexes, gate_seq):
        """
        Assigns a new gates to the positions given by the input indexes. Called by the "=" operator on commands like "cunqa_circuit[3] = {"name":"x", "qubits":[0]}".

        Args
            indexes (int, list[int]): positions of the circuit to be modified.
            gate_seq (dict, list(dict)): gates to be assigned in order on the input positions.
        """
        if (isinstance(indexes, list) and isinstance(gate_seq, list)):
            if len(indexes) == len(gate_seq):
                for index, gate in zip(indexes, gate_seq):
                    self._check_instruction(gate)
                    self.instructions[index] = gate
            else:
                logger.error(f"The indexes and gate_seq lists must have equal lenght [{ValueError.__name__}]")
                raise SystemExit
        elif (isinstance(indexes, int) and isinstance(gate_seq, dict)):
            self._check_instruction(gate_seq)
            self.instructions[indexes] = gate_seq
        else:
            logger.error(f"Both the indexes and gate_seq should be lists, or an int and a dict respectively. Instead a {type(indexes)} indexes and a {type(gate_seq)} gate_seq was provided [{TypeError.__name__}]")
            raise SystemExit


    ######################## DRAWING THE CIRCUIT ########################

    def draw(self, include_comm = False):
        """
        Method for plotting a circuit using Latex. CURRENTLY BEING DEVELOPED
        """
        rc('text', usetex=True)

        #Create the Tikz code from the circuit (use r""" """ for a raw string literal that preserves special characters)
        tikz_code = r"""\begin{quantikz}
        """
        # Iterate throught the layers to convert the gates to quantikz instructions
        for intructions in self.layers.values():
            new_line = r"""&"""
            for layer, instr_index in intructions:
                new_line += r"""\gate[]{} """# Write the gates 
            new_line+r"""
            """
            tikz_code+=new_line

        tikz_code + r"""\end{quantikz}"""
        
        # Create the figure and save it to a file
        fig, ax = plt.subplots(figsize=(8, 6))
        ax.text(0.5, 0.5, tikz_code, ha='center', va='center', transform=ax.transAxes)
        ax.set_xlim(0, 2)
        ax.set_ylim(0, 2)
        ax.set_axis_off()
        plt.savefig('quantikz_diagram.png')

        # Display the saved image within the matplotlib figure
        plt.figure(figsize=(8, 6))
        img = plt.imread('quantikz_diagram.png')
        plt.imshow(img)
        plt.axis('off')
        plt.show() 

    ################### ASSIGN METHODS TO CUNQACIRCUIT ###################

    # Update other instances method
    setattr(cls, '_update_other_instances', _update_other_instances)

    # Addition methods
    setattr(cls, '__add__', __add__)
    setattr(cls, '__radd__', __radd__)
    setattr(cls, '__iadd__', __iadd__)
    setattr(cls, 'sum', sum)

    # Union methods
    setattr(cls, '__or__', __or__)
    setattr(cls, '__ror__', __ror__)
    setattr(cls, '__ior__', __ior__)
    setattr(cls, 'union', union)

    # Splitting methods
    setattr(cls, 'vert_split', vert_split)
    setattr(cls, 'hor_split', hor_split)

    # Length method 
    setattr(cls, '__len__', __len__)

    # Additional methods
    setattr(cls, 'param_info', param_info)
    setattr(cls, 'index', index)
    setattr(cls, '__contains__', __contains__)
    setattr(cls, '__getitem__', __getitem__)
    setattr(cls, '__setitem__', __setitem__)

    # Drawing method
    setattr(cls, 'draw', draw)

    return cls


