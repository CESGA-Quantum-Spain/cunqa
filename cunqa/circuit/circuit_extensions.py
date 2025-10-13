"""
Holds dunder methods for the class CunqaCircuit and other functions to extract information from it.
"""
from typing import Union, Optional, Tuple
import copy
from operator 

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

    # # =============== MAKE CUNQACIRCUITS HASHABLE ================
    # def __hash__(self):
    #     # Must return an integer
    #     # Should be consistent with __eq__
    #     return hash((tuple(self.instructions), self._id))
    
    # def __eq__(self, other):
    #     """ Determines when two CunqaCircuits are considered equal. Used in sets of circuits or on dictionary keys."""
    #     if isinstance(other, CunqaCircuit):
    #         return (self.instructions == other.instructions and self._id == other._id)
    #     return False

    # ================ CIRCUIT MODIFICATION METHODS ==============

    def _update_other_instances(self, instances_to_change, other_id, comb_id, displace_n = 0, up_to_instr = 0, instr_name = "expose"): 
        """ 
        Private method called from the __add__ and __or__ methods and its variations. It modifies the instructions of any other CunqaCircuit instance that references
        self or other, the circuits involved on the operation, to reference the combined circuit instead.
        
        Args:
            instances_to_change (set): set with the ids of the circuits referencing self or other, to change the reference to the combined circuit.
            other_id (str): id of the second circuit in the operation
            comb_id (str): id to substitute in on the instructions of circuits referencing the operands
            displace_n (int): specifies the number of qubits of the upper circuit on a union of circuits. It will displace the qubits of the lower circuit by
                              this amount when necessary.
            up_to_instr (int): stops the substitution after having modified up_to_instr communication instructions 
            instr_name (str): admits the options "expose" or "measure_and_send"
        """
        other_instances = self.__class__.access_other_instances() 

        if isinstance(instances_to_change, set):
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
                    if up_to_instr != 0:
                        for instr in instance.instructions:
                            if ("circuits" in instr and instr["circuits"][0] in [self._id, other_id]):
                                instr["circuits"] = [comb_id]

                    else:
                        i = 0 # Count how many instructions we substitute
                        for instr in instance.instructions:
                            if ("circuits" in instr and instr["circuits"][0] in [self._id, other_id]):
                                instr["circuits"] = [comb_id]
                                i += 1
                                if  i > up_to_instr: # if we have substituted up_to_instr instructions we stop
                                    break

        elif isinstance(instance_to_change, dict):
            for circ, name_list in instance_to_change:
                instance = other_instances[circ]

                for instr in instance.instructions:
                    if "circuits" in instr:
                        instr["circuits"] = name_list.pop(0)

            


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

                circs_comm_self.add(instr["circuits"][0])

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
                    
                circs_comm_other.add(instrr["circuits"][0])
            
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

                    circs_comm_self.add(instrr["circuits"][0])

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

                circs_comm_other.add(instrr["circuits"][0])
                
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

        left_instrs = []; circs_comm_left = set()    # circs_comm_left serves to update circuits that communicate with self to point to left 
        right_instrs = []; circs_comm_right = set()

        num_left_comms = 0 # Tracks how many distributed instructions need to happen on the left
        for qubit_layer_info in self.layers.values(): # Go qubit by qubit
            for layer in qubit_layer_info:

                instr = self.instructions[layer[1]]
                if layer[0] > position:

                    right_instrs.append(instr)
                    if "circuits" in instr:
                        circs_comm_right.add(instr["circuits"])

                else:
                    left_instrs.append(instr)
                    if "circuits" in instr:
                        circs_comm_left.add(instr["circuits"])
                        num_left_comms += 1

        left_circuit.from_instructions(left_instrs);      self._update_other_instances(circs_comm_left, self._id, left_id, displace_n=0, up_to_instr = num_left_comms) 
        right_circuit.from_instructions(right_instrs);    self._update_other_instances(circs_comm_right, self._id, right_circuit)

        return left_circuit, right_circuit


    def hor_split(self, n: int) -> Tuple["CunqaCircuit", "CunqaCircuit"]:
        """Divides a circuit horizontally in two, separating the first n qubits from the last num_qubits-n qubits. """
        rest = self.num_qubits - n 
        cl_n = self.num_clbits # TODO: maybe handle cl_bits more gracefully 
        
        up_id = self._id + f" up_{n}"
        down_id = self._id + f" down_{rest}"

        upper_circuit = CunqaCircuit(n, cl_n, id = up_id)          
        lower_circuit = CunqaCircuit(rest, cl_n, id = down_id)      
        self.circs_comm = {circuit: [] for circuit in self.access_other_instances()}

        # Add instructions to the two pieces according to their position
        iterator_instructions = iter(copy.deepcopy(self.instructions))
        for instr in iterator_instructions:
            self.divide_instr(instr, upper_circuit, lower_circuit, n, iterator_instructions)

        self._update_other_instances(self.circs_comm, self._id, "")   
         
        # Clean up instance attributes that shouldn't bloat the instance beyond this method
        del self.circs_comm          

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

    ####################### FUNCTION FOR HOR_SPLIT ########################

    def divide_instr(self, instr: dict, upper_circuit: CunqaCircuit, lower_circuit: CunqaCircuit, n: int, iterator_instructions: 'list_iterator', received: bool = False, recv_remain: int = 0):
        """
        Method to divide an instruction between the upper and lower circuits in hor_split. If given the instructions "recv" or "rcontrol" it calls itself on the next value
        of the iterator or on the first element of instr["instructions"], respectively. This recurrence makes it less readable. Additionally, only instructions that fall 
        squarely on one side are supported on "recv" and "rcontrol" as otherwise they would need a mix of classical and quantum communications or multicontrolled communications
        with control qubits on different circuits.

        Args:
            instr (dict): the dictionary describing the instruction to divide
            upper_circuit (CunqaCircuit):
            lower_circuit (CunqaCircuit):
            n (int): the number of qubits of the upper_circuit (maybe return the divided instructions and remove the circuits from the arguments)
            iterator_instructions (list_iterator): iterator from the list of instructions. Allows the "recv" case to access the next instruction
            received (bool): determines wether 'divide_instr' is being called from within (True), in a received case or from 'hor_split' (False).
            recv_remain (int): determines the lenght of instr["instructions"] in the "rcontrol" case to handle recurrence and avoid an infinte loop. 
        """
        qubits_up        = [q for q in instr["qubits"][0] if q < n+1]
        qubits_down      = [q - (n+1) for q in instr["qubits"][0] if q > n]
        conditional_up   = [q for q in instr["conditional_reg"] if q < n+1]         if ("conditional_reg" in instr) else []
        conditional_down = [q - (n+1) for q in instr["conditional_reg"] if q > n ]  if ("conditional_reg" in instr) else []

        if received and not hasattr(self, rcontrol_up) and not hasattr(self, rcontrol_down):
            self.rcontrol_up = []    # Here we will save the divided received instructions on the corresponding side
            self.rcontrol_down = []

        # ZERO QUBITS: RECV, RCONTROL. We need to check next instruction or the key "instructions"
        if instr["name"] == "recv":
            instr2 = next(iterator_instructions)
            divide_instr(instr2, upper_circuit, lower_circuit, n, iterator_instructions, received = True, recv_remain = 0)

            if self.rcontrol_up and self.rcontrol_down:
                logger.error(f"Dividing instruction {instr} would require mixing classical and quantum communications, which is not supported.")
                raise SystemExit
                # upper_circuit._add_instruction(instr);               lower_circuit._add_instruction(instr)                # recv
                # upper_circuit._add_instruction(*self.rcontrol_up);   lower_circuit._add_instruction(*self.rcontrol_down)

                # TODO: call the function adding another send on the origin circuit 
                # For that we will need to track classical comm instructions processed (for instance with self.num_recv)

            elif self.rcontrol_up:
                upper_circuit._add_instruction(instr) # recv
                upper_circuit._add_instruction(*self.rcontrol_up)
                self.circs_comm[instr["circuits"][0]].append(upper_circuit._id)

            elif self.rcontrol_down:
                lower_circuit._add_instruction(instr) # recv
                lower_circuit._add_instruction(*self.rcontrol_down)
                self.circs_comm[instr["circuits"][0]].append(lower_circuit._id)
            
            del self.rcontrol_up
            del self.rcontrol_down

        elif instr["name"] == "rcontrol":
            total_recv = len(instr["instructions"])
            iter_rcontrol = iter(copy.deepcopy(instr["instructions"]))
            instrr = next(iter_rcontrol)
            
            divide_instr(instrr, upper_circuit, lower_circuit, n, iter_rcontrol, received = True, recv_remain = total_recv)
            
            if self.rcontrol_up and self.rcontrol_down:
                logger.error(f"Dividing instruction {instr} would require quantum communications between 3 circuits, which is not supported (yet?).")
                raise SystemExit
                # instr_copy = copy.deepcopy(instr)
                # instr["instructions"] = self.rcontrol_up
                # instr_copy["instructions"] = self.rcontrol_down
                # upper_circuit._add_instruction(instr);    lower_circuit._add_instruction(instr_copy) # rcontrol
                
                # TODO: call the function adding another send on the origin circuit 
                # For that we will need to track exposes to self on the other circuit processed

            elif self.rcontrol_up:
                instr["instructions"] = self.rcontrol_up
                upper_circuit._add_instruction(instr) # rcontrol
                self.circs_comm[instr["circuits"][0]].append(upper_circuit._id)

            elif self.rcontrol_down:
                instr["instructions"] = self.rcontrol_down
                lower_circuit._add_instruction(instr) # rcontrol
                self.circs_comm[instr["circuits"][0]].append(lower_circuit._id)

            # TODO: modify other instances to point to the correct one
            
            del self.rcontrol_up
            del self.rcontrol_down

                 

        # NO DIVISION CASE
        # Note: "expose", "measure_and_send", "qsend", "qrecv", all SUPPORTED_GATES_1Q fall here
        elif (len(qubits_up) == 0 and len(conditional_up) == 0): 
            instr["qubits"] = qubits_down
            if "conditional_reg" in instr: 
                instr["conditional_reg"] = conditional_down

            if "circuits" in instr:
                self.circs_comm[instr["circuits"][0]].append(lower_circuit._id)

            if received:
                self.rcontrol_down.append(instr)

                if recv_remain > 0:
                    next_rcontrol_instr = next(iterator_instructions)
                    divide_instr(next_rcontrol_instr, upper_circuit, lower_circuit, n, iterator_instructions, received = True, recv_remain = total_recv - 1)
                return

            lower_circuit._add_instruction(instr)            

        elif (len(qubits_down) == 0 and len(conditional_down) == 0):
            if "circuits" in instr:
                self.circs_comm[instr["circuits"][0]].append(upper_circuit._id)

            if received:
                self.rcontrol_up.append(instr)

                if recv_remain > 0:
                    next_rcontrol_instr = next(iterator_instructions)
                    divide_instr(next_rcontrol_instr, upper_circuit, lower_circuit, n, iterator_instructions, received = True, recv_remain = total_recv - 1)
                return

            upper_circuit._add_instruction(instr)

        elif received:
            # If we arrive here it means that the gate will need quantum communication between 3 circuits or a mix of quantum and classical comms
            self.rcontrol_up = ["Error"]; self.rcontrol_down = ["Error"] # An error will be raised as both are non-empty
            return

        # ONLY CLASSICAL COMMUNICATION DIVISION
        elif (len(qubits_up) == 0 and len(conditional_up) != 0 ):
            conditional_reg = conditional_up[0]

            upper_circuit.measure_and_send(qubit=conditional_reg, target_circuit=lower_circuit)
            lower_circuit._add_instruction({
                "name": "recv",
                "qubits":[],
                "remote_conditional_reg":conditional_reg,
                "circuits": [upper_circuit._id]
            })
            instr["remote_conditional_reg"] = [conditional_reg]
            lower_circuit._add_instruction(instr)


        elif (len(qubits_down) == 0 and len(conditional_down) != 0 ):
            conditional_reg = conditional_down[0]

            lower_circuit.measure_and_send(qubit=conditional_reg, target_circuit=upper_circuit)
            upper_circuit._add_instruction({
                "name": "recv",
                "qubits":[],
                "remote_conditional_reg":conditional_reg,
                "circuits": [lower_circuit._id]
            })
            instr["remote_conditional_reg"] = [conditional_reg]
            upper_circuit._add_instruction(instr)

        # QUANTUM COMMUNICATION DIVISION 
        # NOTE: Difficult to maintain (any other gate need to be entered manually)

        elif "conditional_reg" in instr: # Bad luck :(
            logger.error(f"Error: cannot divide instruction {instr} as it would require quantum and classical communications at the same time.")
            raise SystemExit

        elif instr["name"] == "save_state":
            upper_circuit.save_state()
            lower_circuit.save_state() 

        # 2Qubit gates 
        elif instr["name"] == "swap":
            """ Swap decomposition:
                         ┌───┐
            q_0:    ──■──┤ X ├──■──
                    ┌─┴─┐└─┬─┘┌─┴─┐
            q_1:    ┤ X ├──■──┤ X ├
                    └───┘     └───┘
            """
            up_qubit, down_qubit = (instr["qubits"][0], instr["qubits"][1]- n - 1) if instr["qubits"][0] > n else (instr["qubits"][1],  instr["qubits"][0] - n - 1)
            
            with upper_circuit.expose(up_qubit, lower_circuit) as rcontrol:
                lower_circuit.cx(rcontrol, down_qubit)

            with lower_circuit.expose(down_qubit, upper_circuit) as rcontrol:
                upper_circuit.cx(rcontrol, up_qubit)

            with upper_circuit.expose(up_qubit, lower_circuit) as rcontrol:
                lower_circuit.cx(rcontrol, down_qubit)

        elif instr["name"] in ["cu", "cu1", "cu3", "cx", "cy", "cz", "csx", "cp", "crx", "cry", "crz"]:

            name_to_method = {
                "cu":  CunqaCircuit.cu,  "cu1": CunqaCircuit.cu1, "cu3": CunqaCircuit.cu3, 
                "cx":  CunqaCircuit.cx,  "cy":  CunqaCircuit.cy,  "cz":  CunqaCircuit.cz, 
                "crx": CunqaCircuit.crx, "cry": CunqaCircuit.cry, "crz": CunqaCircuit.crz,
                "csx": CunqaCircuit.csx, "cp":  CunqaCircuit.cp}
            if instr["qubits"][0] < instr["qubits"][1]:

                qubit_up = instr["qubits"][0]
                qubit_down = instr["qubits"][1] - n - 1
                method = name_to_method[instr["name"]]
                    
                with upper_circuit.expose(qubit_up, lower_circuit) as rcontrol:
                    method(lower_circuit, *instr["params"], qubit_down)
            else:
                qubit_up = instr["qubits"][1]
                qubit_down = instr["qubits"][0] - n - 1
                    
                with lower_circuit.expose(qubit_down, upper_circuit) as rcontrol:
                    method(upper_circuit, *instr["params"], qubit_up)  

        elif instr["name"] in ["rzz", "ryy", "rxx", "rzx"]:
            """ Rzz gate decomposition:         Ryy gate decomposition:       Rxx gate decomposition:                     Rzx gate decomposition:
                                                   ┌───┐            ┌───┐        ┌─────────┐            ┌──────────┐
            qubit1: ──■─────────────────■──     1: ┤ H ├─■──────────┤ H ├     1: ┤ Rx(π/2) ├─■──────────┤ Rx(-π/2) ├       1: ──────■───────────────
                    ┌─┴─┐┌───────────┐┌─┴─┐        ├───┤ │ZZ(param) ├───┤        ├─────────┤ │ZZ(param) ├──────────┤          ┌───┐ │ZZ(param) ┌───┐
            qubit2: ┤ X ├┤ Rz(param) ├┤ X ├     2: ┤ H ├─■──────────┤ H ├     2: ┤ Rx(π/2) ├─■──────────┤ Rx(-π/2) ├       2: ┤ H ├─■──────────┤ H ├
                    └───┘└───────────┘└───┘        └───┘            └───┘        └─────────┘            └──────────┘          └───┘            └───┘

            """
            distr_rzz_rxx_ryy_rzx(instr, upper_circuit, lower_circuit, n) 

        elif instr["name"] == "ecr":
            """ ECR gate decomposition:
                global phase: 7π/4
                     ┌───┐      ┌───┐
                q_0: ┤ S ├───■──┤ X ├
                     ├───┴┐┌─┴─┐└───┘
                q_1: ┤ √X ├┤ X ├─────
                     └────┘└───┘     
            """
            if instr["qubits"][0] < instr["qubits"][1]:
                qubit_up = instr["qubits"][0]
                qubit_down = instr["qubits"][1] - n - 1
                    
                with upper_circuit.expose(qubit_up, lower_circuit) as rcontrol:
                    upper_circuit.s(qubit_up)
                    lower_circuit.sx(qubit_down)
                    lower_circuit.cx(rcontrol, qubit_down)
                    upper_circuit.x(qubit_up)
            else:
                qubit_up = instr["qubits"][1]
                qubit_down = instr["qubits"][0] - n - 1

                with lower_circuit.expose(qubit_down, upper_circuit) as rcontrol:
                    lower_circuit.s(qubit_down)
                    upper_circuit.sx(qubit_up)
                    upper_circuit.cx(rcontrol, qubit_up)
                    lower_circuit.x(qubit_down)      

        # 3Qubit Gates
        elif instr["name"] in ["ccx", "ccz", "cswap"]:
            """ The restrictions of telegate + the possibility of two controls falling on the same circuit
                force us to use the CCX decomposition:        Cswap decomp. using ccx:     CCZ decomp. using ccx:
                q_0: ────────■─────────────────■────■───      q_0: ───────■───────         q_0: ───────■───────
                           ┌─┴─┐┌─────┐      ┌─┴─┐  │              ┌───┐  │  ┌───┐                     │       
                q_1: ──■───┤ X ├┤ Sdg ├──■───┤ X ├──┼───      q_1: ┤ X ├──■──┤ X ├         q_1: ───────■───────
                     ┌─┴──┐├───┤└─────┘┌─┴──┐├───┤┌─┴──┐           └─┬─┘┌─┴─┐└─┬─┘              ┌───┐┌─┴─┐┌───┐
                q_2: ┤ Sx ├┤ Z ├───────┤ Sx ├┤ Z ├┤ Sx ├      q_2: ──■──┤ X ├──■──         q_2: ┤ H ├┤ X ├┤ H ├
                     └────┘└───┘       └────┘└───┘└────┘                └───┘                   └───┘└───┘└───┘
            """    
            distr_ccx_ccz_cswap(instr, upper_circuit, lower_circuit, n)

        else: # Note that "unitary" falls here (also multicontrolled gates)
            logger.error(f"It is not a priori clear how to divide the provided instruction: {instr}")
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
    setattr(cls, 'divide_instr', divide_instr)

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

    # Drawing method
    setattr(cls, 'draw', draw)

    return cls

############################### HELPERS FOR HOR_SPLIT ###########

def distr_rzz_rxx_ryy_rzx(instr: dict, upper_circuit: CunqaCircuit, lower_circuit: CunqaCircuit, n: int) -> None:
    """
    Performs either a rzz, or rxx, or ryy, or rzx gate distributed between two circuits. 
    The rzz gate decomposition is                                       
                                        q_1: ──■─────────────────■──         
                                             ┌─┴─┐┌───────────┐┌─┴─┐         
                                        q_2: ┤ X ├┤ Rz(param) ├┤ X ├         
                                             └───┘└───────────┘└───┘    

    Rxx:                               Ryy:                                    Rzx:                 
         ┌───┐            ┌───┐        ┌─────────┐            ┌──────────┐
      1: ┤ H ├─■──────────┤ H ├     1: ┤ Rx(π/2) ├─■──────────┤ Rx(-π/2) ├       1: ──────■───────────────
         ├───┤ │ZZ(param) ├───┤        ├─────────┤ │ZZ(param) ├──────────┤          ┌───┐ │ZZ(param) ┌───┐
      2: ┤ H ├─■──────────┤ H ├     2: ┤ Rx(π/2) ├─■──────────┤ Rx(-π/2) ├       2: ┤ H ├─■──────────┤ H ├
         └───┘            └───┘        └─────────┘            └──────────┘          └───┘            └───┘
    Args:
        instr (dict): the instruction to divide 
        upper_circuit (CunqaCircuit): circuit with the first n qubits of the original circuit
        lower_circuit (CunqaCircuit): circuit with the last num_qubits - n qubits of the original circuit
        n (int): number of qubits of the upper_circuit
    """
    if instr["qubits"][0] < instr["qubits"][1]:
        qubit_1 = instr["qubits"][0];             circ1 = upper_circuit
        qubit_2 = instr["qubits"][1] - n - 1;     circ2 = lower_circuit

    else:
        qubit_1 = instr["qubits"][1];             circ1 = lower_circuit
        qubit_2 = instr["qubits"][0] - n - 1;     circ2 = upper_circuit

    if instr["name"] == "ryy":
        circ1.rx(np.pi/2, qubit_1)
        circ2.rx(np.pi/2, qubit_2)
    elif intr["name"] == "rxx":
        circ1.h(qubit_1)
        circ2.h(qubit_2)
    elif instr["name"] == "rzx":
        circ2.h(qubit2)

    ############## Body of rzz ##################
    with circ1.expose(qubit1, circ2) as rcontrol:
        circ2.cx(rcontrol, qubit2)
        circ2.rz(*instr["params"], qubit2)
        circ2.cx(rcontrol, qubit2)
    ############## End of rzz ##################

    if instr["name"] == "ryy":
        circ1.rx(np.pi/2, qubit_1)
        circ2.rx(np.pi/2, qubit_2)
    elif intr["name"] == "rxx":
        circ1.h(qubit_1)
        circ2.h(qubit_2)
    elif instr["name"] == "rzx":
        circ2.h(qubit2)

def distr_ccx_ccz_cswap(instr: dict, upper_circuit: CunqaCircuit, lower_circuit: CunqaCircuit, n: int) -> None:
    """
    Performs a either a Toffoli (ccx), or ccz, or cswap gate distributed between two circuits. 

    Cswap decomp. using ccx:     CCZ decomp. using ccx:
    q_0: ───────■───────         q_0: ───────■───────
         ┌───┐  │  ┌───┐                     │       
    q_1: ┤ X ├──■──┤ X ├         q_1: ───────■───────
         └─┬─┘┌─┴─┐└─┬─┘              ┌───┐┌─┴─┐┌───┐
    q_2: ──■──┤ X ├──■──         q_2: ┤ H ├┤ X ├┤ H ├
              └───┘                   └───┘└───┘└───┘

    Args:
        instr (dict): the instruction to divide 
        upper_circuit (CunqaCircuit): circuit with the first n qubits of the original circuit
        lower_circuit (CunqaCircuit): circuit with the last num_qubits - n qubits of the original circuit
        n (int): number of qubits of the upper_circuit
    """
    # Process instruction to decide the case
    circ1, circ2, less_than_or_greater_equal = (upper_circuit, lower_circuit, operator.lt) if instr["qubits"][0] < n+1 else (lower_circuit, upper_circuit, operator.ge)
    qubits1 = {}; qubits2 = {}
    for i, q in enumerate(instr["qubits"]):

        if less_than_or_greater_equal(q, n+1): # Apply the corresponding comparation q < n+1 or q >= n+1 depending on which circuit is circ1
            qubits1[str(i+1)] = q
        else:
            qubits2[str(i+1)] = q - n - 1

    if ("1" in qubits1 and ("2" in qubits2 and "3" in qubits2)):
        """ First qubit in circ1, second and third qubits in circ2, 
        corresponds to the partition of the decomposition of CCX given by:
        1:   ────────■─────────────────■────■───
                     │                 │    │   
        -------------│-----------------│----│-------   
                   ┌─┴─┐┌─────┐      ┌─┴─┐  │               CCX
        2:   ──■───┤ X ├┤ Sdg ├──■───┤ X ├──┼───
             ┌─┴──┐├───┤└─────┘┌─┴──┐├───┤┌─┴──┐
        3:   ┤ Sx ├┤ Z ├───────┤ Sx ├┤ Z ├┤ Sx ├
             └────┘└───┘       └────┘└───┘└────┘
         """
        q1 = qubits1["1"]
        q2 = qubits2["2"]; q3 = qubits2["3"]

        if instr["name"] == "ccz":
            circ2.h(q3)
        elif instr["name"] == "cswap":
            circ2.cx(q3, q2)

        ############# Body of ccx ##############
        with circ1.expose(q1, circ2) as rcontrol: 
            circ2.csx(q2, q3)
            circ2.cx(rcontrol, q2)
            circ2.z(q3)
            circ2.sdg(q2)
            circ2.csx(q2, q3)
            circ2.cx(rcontrol, q2)
            circ2.z(q3)
            circ2.csx(rcontrol, q3)  
        ############### ccx ends ################          

        if instr["name"] == "ccz":
            circ2.h(q3)
        elif instr["name"] == "cswap":
            circ2.cx(q3, q2)

    elif (("1" in qubits1 and "2" in qubits1) and "3" in qubits2):
        """ First and second qubit in circ1, third qubit in circ2, corresponds to the partition of the decomposition of ccx given by:
        1:   ────────■─────────────────■────■───
                   ┌─┴─┐┌─────┐      ┌─┴─┐  │   
        2:   ──■───┤ X ├┤ Sdg ├──■───┤ X ├──┼───
               │   └───┘└─────┘  │   └───┘  │                   CCX
        -------│-----------------│----------│--------
             ┌─┴──┐┌───┐       ┌─┴──┐┌───┐┌─┴──┐
        3:   ┤ Sx ├┤ Z ├───────┤ Sx ├┤ Z ├┤ Sx ├
             └────┘└───┘       └────┘└───┘└────┘
        """
        q1 = qubits1["1"]; q2 = qubits1["2"]
        q3 = qubits2["3"]

        if instr["name"] == "ccz":
            circ2.h(q3)
        elif instr["name"] == "cswap":
            with circ2.expose(q3, circ1) as rcontrol:
                circ1.cx(rcontrol, q2)

        ############# Body of ccx ##############
        with circ1.expose(q2, circ2) as rcontrol:       
            circ2.csx(rcontrol, q3);  circ1.cx(q1, q2)
            circ2.z(q3);              circ1.sdg(q2)
            circ2.csx(rcontrol, q3);  circ1.cx(q1, q2)
            circ2.z(q3)

        with circ1.expose(q1, circ2) as rcontrol:
            circ2.csx(rcontrol, q3)
        ############### ccx ends ################

        if instr["name"] == "ccz":
            circ2.h(q3)
        elif instr["name"] == "cswap":
            with circ2.expose(q3, circ1) as rcontrol:
                circ1.cx(rcontrol, q2)

    elif (("1" in qubits1 and "3" in qubits1) and "2" in qubits2):
        """ First and third qubits in circ1, second qubit in circ2, corresponds to the partition of the decomposition of ccx given by:
        1:   ────────■─────────────────■─────────■───
             ┌────┐  │   ┌───┐ ┌────┐  │  ┌───┐┌─┴──┐
        3:   ┤ Sx ├──┼───┤ Z ├─┤ Sx ├──┼──┤ Z ├┤ Sx ├
             └─┬──┘  │   └───┘ └─┬──┘  │  └───┘└────┘                   CCX
               │     │           │     │
        -------│-----│-----------│-----│----------------
               │   ┌─┴─┐┌─────┐  │   ┌─┴─┐
        2:   ──■───┤ X ├┤ Sdg ├──■───┤ X ├───────────
                   └───┘└─────┘      └───┘           
        """
        q1 = qubits1["1"]; q3 = qubits1["3"]
        q2 = qubits2["2"]

        if instr["name"] == "ccz":
            circ1.h(q3)
        elif instr["name"] == "cswap":
            with circ1.expose(q3, circ2) as rcontrol:
                circ2.cx(rcontrol, q2)

        ############# Body of ccx ##############
        with circ2.expose(q2, circ1) as rcontrol:
            circ1.csx(rcontrol, q3)
        with circ1.expose(q1, circ2) as rcontrol:
            circ2.cx(rcontrol, q2)

        circ1.z(q3); circ2.sdg(q2)
        with circ2.expose(q2, circ1) as rcontrol:
            circ1.csx(rcontrol, q3)
        with circ1.expose(q1, circ2) as rcontrol:
            circ2.cx(rcontrol, q2)
        
        circ1.z(q3)
        circ1.csx(q1, q3)
        ############### ccx ends ################

        if instr["name"] == "ccz":
            circ1.h(q3)
        elif instr["name"] == "cswap":
            with circ1.expose(q3, circ2) as rcontrol:
                circ2.cx(rcontrol, q2)

    else:
        # Any other valid configuration is complementary to these ones
        logger.error(f"Too many qubits were sent to distr_ccx: {len(circ_and_qubits1) + len(circ_and_qubits2) - 2}")
        raise IndexError



