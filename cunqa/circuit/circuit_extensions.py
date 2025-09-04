"""
Holds dunder methods for the class CunqaCircuit and other functions to extract information from it.
"""
from typing import Union, Optional, Tuple

from cunqa.logger import logger
from cunqa.circuit.circuit import CunqaCircuit, _flatten, SUPPORTED_GATES_DISTRIBUTED, SUPPORTED_GATES_1Q
from cunqa.circuit.converters import qc_to_json

from qiskit import QuantumCircuit
import matplotlib.pyplot as plt #I use this for drawing circuits with LaTeX (quantikz)
from matplotlib import rc

def cunqa_dunder_methods(cls):

    # ================ CIRCUIT MODIFICATION METHODS ==============

    # Horizontal concatenation methods

    def update_other_instances(self, instances_to_change, other_id, comb_id, displace_n = 0): # Change other instances that referenced any of the circuits to reference the combined circuit
        other_instances = self.__class__.access_other_instances() 

        if isinstance(instances_to_change, set): # This one should be used for the sum, where no displacement of the referenced qubits is necessary
            for circuit in instances_to_change: 
                instance = other_instances[circuit]
                for instr in instance.instructions:
                    if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] in [self._id, other_id]):
                        instr["circuits"] == [comb_id]

        elif isinstance(instances_to_change, dict): # This one should be used for the union, where the second circuits' qubits are displaced and the reference needs to be updated
            for circuit, v in instances_to_change.items():
                instance = other_instances[circuit]
                i=0
                for instr in instance.instructions:
                    if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] == other_id):
                        instr["circuits"] == [comb_id]
                        # The dictionary holds for each circuit a list with "control" or "target" on each entry
                        if v[i] == "control": # here we displace the appropriate value by the number of qubits of the first circuit
                            instr["qubits"][0] += displace_n 
                        elif (v[i] == "target" and instr["name"] in ["remote_c_if_cx","remote_c_if_cy","remote_c_if_cz"]): #case of 2-qubit remote gates
                            instr["qubits"][-1] += displace_n 
                            instr["qubits"][-2] += displace_n 
                        else:
                            instr["qubits"][-1] += displace_n 
                        i += 1 # Advance index on the list specifying if we should change the control or target key of the instruction


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
        n = self.num_qubits
        if  n == other_circuit.num_qubits:        
            instances_to_change = set() # If our circuits have distributed gates, as the id changes, we will need to update it on the other circuits that communicate with this one

            if isinstance(other_circuit, CunqaCircuit):
                if not force_execution:
                    if any([(self._id in path and other_circuit._id in path) for path in self.__class__.get_connectivity()]):
                        logger.error(f"Circuits to sum are connected, directly or through a chain of other circuits. This could result in execution waiting forever. If you're sure this won't happen try the syntax sum(circ_1, circ_2, force_execution = True).")
                        raise SystemExit
                    
                other_instr = other_circuit.instructions
                instances_to_change.union({instr["circuits"][0] for instr in other_instr if instr["name"] in SUPPORTED_GATES_DISTRIBUTED})
                other_id = other_circuit._id
                sum_id = self._id + " + " + other_id

            elif isinstance(other_circuit, QuantumCircuit):
                other_instr = qc_to_json(other_circuit)['instructions']
                other_id = "qc"
                sum_id = self._id + " + " + other_id
                
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
            
            self_instr = self.instructions
            instances_to_change.union({instr["circuits"][0] for instr in self_instr if instr["name"] in SUPPORTED_GATES_DISTRIBUTED})

            cl_n=self.num_clbits; cl_m=other_circuit.num_clbits
            summed_circuit = CunqaCircuit(n, max(cl_n,cl_m), id = sum_id) 

            for instruction in list(self_instr + other_instr):
                summed_circuit._add_instruction(instruction)

            self.update_other_instances(instances_to_change, other_id, sum_id)
            
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
        n = self.num_qubits
        if  n == left_circuit.num_qubits:
            if isinstance(left_circuit, CunqaCircuit):
                return left_circuit.__add__(self)

            elif isinstance(left_circuit, QuantumCircuit):
                left_id = "qc"
                sum_id = left_id + " + " + self._id

                cl_n=self.num_clbits; cl_m=left_circuit.num_clbits
                summed_circuit = CunqaCircuit(n, max(cl_n, cl_m), id = sum_id)
                left_instr = qc_to_json(left_circuit)['instructions']
                
                for instruction in list(left_instr + self.instructions):
                    summed_circuit._add_instruction(instruction)

                instances_to_change = {instr["circuits"][0] for instr in self.instructions if instr["name"] in SUPPORTED_GATES_DISTRIBUTED}
                self.update_other_instances(instances_to_change, left_id, sum_id) # Update other circuits that communicate with self to reference the summed_circuit

                return summed_circuit
        
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(left_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
                    
        else:
            logger.error(f"First version only accepts summing circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
            raise SystemExit

    def __iadd__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit], force_execution = False):
        """
        Overloading the "+=" operator to concatenate horizontally the circuit self with other_circuit. This means adding the operations from 
        the other_circuit to self. No return as the modifications are performed locally on self.
        Args
            other_circuit (<class.cunqa.circuit.CunqaCircuit>, <class.qiskit.QuantumCircuit>): circuit to be horizontally concatenated after self.
            force_execution (bool): disallows the check that raises an error if both circuits are distributed (version 1).
        """

        n = self.num_qubits
        if  n == other_circuit.num_qubits:
            cl_m = other_circuit.num_clbits
            if cl_m > self.num_clbits:
                self._add_cl_register(name=other_id, number_clbits=cl_m-self.num_clbits)

            instances_to_change = set()
            if isinstance(other_circuit, CunqaCircuit):
                if not force_execution:
                    if any([(self._id in connection and other_circuit._id in connection) for connection in self.__class__.get_connectivity()]):
                        logger.error(f"Circuits to sum are connected, directly or through a chain of other circuits. This could result in execution waiting forever. If you're sure this won't happen try the syntax sum(circ_1, circ_2, force_execution = True).")
                        raise SystemExit
                    
                other_instr = other_circuit.instructions
                other_id = other_circuit._id
                instances_to_change.union({instr["circuits"][0] for instr in other_instr if instr["name"] in SUPPORTED_GATES_DISTRIBUTED})

            elif isinstance(other_circuit, QuantumCircuit):
                other_instr = qc_to_json(other_circuit)['instructions']
                other_id = "qc" # Avoids an error on update_other_instances execution
                
            else:
                logger.error(f"CunqaCircuits can only be summed with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
                raise SystemExit
            
            
            for instruction in list(other_instr):
                self._add_instruction(instruction)

            if self._id in instances_to_change:
                logger.error("The circuits to be summed contain distributed instructions that reference eachother.")
                raise SystemExit
            self.update_other_instances(instances_to_change, other_id, self._id)
        
        else:
            logger.error(f"Only possible to sum circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
            raise SystemExit
            


    # Vertical concatenation methods

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
        instances_to_change = set()
        if isinstance(other_circuit, CunqaCircuit):
            other_instr = other_circuit.instructions
            other_id = other_circuit._id
            union_id = self._id + " | " + other_id

            instances_to_change.union({instr["circuits"][0] for instr in other_instr if instr["name"] in SUPPORTED_GATES_DISTRIBUTED})

        elif isinstance(other_circuit, QuantumCircuit):
            other_instr = qc_to_json(other_circuit)['instructions']
            other_id = "qc" # Avoids an error on update_other_instances execution
            union_id = self._id + " | " + other_id
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit
        
        n=self.num_qubits; m=other_circuit.num_qubits
        cl_n=self.num_clbits; cl_m=other_circuit.num_clbits
        union_circuit = CunqaCircuit(n+m,cl_n+cl_m, id = union_id)

        for instr in self.instructions:
            # If we find distributed gates referencing self, substitute by a local gate
            if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] == other_id): 
                if instr["name"] == "measure_and_send":
                    continue # These ones will be substituted later
                else:
                    instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                    instr["qubits"][0] = instr["qubits"][0] + n # The control comes from the displaced circuit

            union_circuit._add_instruction(instr)

        instances_to_change_and_displace = {}
        for instrr in other_instr:
            instrr["qubits"] = [qubit + n for qubit in instrr["qubits"]] # displace the qubits of the instructions and then add it to union_circuit
            if "clbits" in instrr:
                instrr["clbits"] = [clbit + cl_n for clbit in instrr["clbits"]]
            union_circuit._add_instruction(instrr)

            if instrr["name"] in SUPPORTED_GATES_DISTRIBUTED: # Gather info on the circuits that reference the other_circuit and wether it controls or is a target
                if instrr["circuits"][0] == self._id: # Susbtitute distr gate by local gates if it refences upper_circuit 
                    if instrr["name"] == "measure_and_send":
                        continue # These ones have been substituted earlier
                    else:
                        instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                        instr["qubits"][-1] = instr["qubits"][-1] + n # The control comes from the displaced circuit
                        if instr["name"] in ["c_if_cx","c_if_cy","c_if_cz"]:
                            instr["qubits"][-2] = instr["qubits"][-2] + n

                if instrr["circuits"][0] in instances_to_change_and_displace:
                    instances_to_change_and_displace[instrr["circuits"][0]].append("control" if instrr["name"] == "measure_and_send" else "target")
                else:
                    instances_to_change_and_displace[instrr["circuits"][0]] = ["control" if instrr["name"] == "measure_and_send" else "target"]


        self.update_other_instances(instances_to_change, other_id, union_id) # Update other circuits that communicate with our input circuits to reference the union_circuit
        self.update_other_instances(instances_to_change_and_displace, other_id, union_id, n)

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

            n=self.num_qubits; m=upper_circuit.num_qubits
            cl_n=self.num_clbits; cl_m=upper_circuit.num_clbits
            union_circuit = CunqaCircuit(n+m,cl_n+cl_m, id = union_id)

            for instr in upper_instr:
                # If we find distributed gates referencing self, substitute by a local gate
                if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] == self._id): 
                    if instr["name"] == "measure_and_send":
                        continue # These ones will be substituted later
                    else:
                        instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                        instr["qubits"][0] = instr["qubits"][0] + n # The control comes from the displaced circuit
                union_circuit._add_instruction(instr)


            instances_to_change_and_displace = {} # Here we will collect info on the circuits that talk to self to make them reference the union instead
            for instrr in self.instructions:
                instrr["qubits"] = [qubit + m for qubit in instrr["qubits"]]
                if "clbits" in instrr:
                    instrr["clbits"] = [clbit + cl_n for clbit in instrr["clbits"]] 

                if instrr["name"] in SUPPORTED_GATES_DISTRIBUTED: # Gather info on the circuits that reference other_circuit and wether it controls or is a target
                    if instrr["circuits"][0] == upper_id: # Susbtitute distr gate by local gates if it refences upper_circuit 
                        if instrr["name"] == "measure_and_send":
                            continue # These ones have been substituted earlier
                        else:
                            instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                            instr["qubits"][-1] = instr["qubits"][-1] + n # The control comes from the displaced circuit
                            if instr["name"] in ["c_if_cx","c_if_cy","c_if_cz"]:
                                instr["qubits"][-2] = instr["qubits"][-2] + n

                    elif instrr["circuits"][0] in instances_to_change_and_displace:
                        instances_to_change_and_displace[instrr["circuits"][0]].append("control" if instrr["name"] == "measure_and_send" else "target")
                    else:
                        instances_to_change_and_displace[instrr["circuits"][0]] = ["control" if instrr["name"] == "measure_and_send" else "target"]

                union_circuit._add_instruction(instrr)

            self.update_other_instances(instances_to_change_and_displace, upper_id, union_id, n)

            return union_circuit
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(upper_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit
        
            
        
    def __ior__(self, other_circuit: Union['CunqaCircuit', QuantumCircuit]):
        if isinstance(other_circuit, CunqaCircuit):
            other_instr = other_circuit.instructions
            other_id = other_circuit._id

        elif isinstance(other_circuit, QuantumCircuit):
            other_instr = qc_to_json(other_circuit)['instructions']
            other_id = "qc"
                
        else:
            logger.error(f"CunqaCircuits can only be unioned with other CunqaCircuits or QuantumCircuits, but {type(other_circuit)} was provided.[{NotImplemented.__name__}].")
            raise SystemExit
        
        

        n=self.num_qubits; cl_n=self.num_clbits; need_to_modify_self = False
        m=other_circuit.num_qubits; cl_m=other_circuit.num_clbits
        self._add_q_register(name=other_id, number_qubits=m); self._add_cl_register(name=other_id, number_clbits=cl_m)

        instances_to_change_and_displace = {} # Here we will collect info on the circuits that talk to other_circuit to make them reference self instead
        for instrr in other_instr:
            instrr["qubits"] = [qubit + n for qubit in instrr["qubits"]]
            if "clbits" in instrr:
                instrr["clbits"] = [clbit + cl_n for clbit in instrr["clbits"]]

            if instrr["name"] in SUPPORTED_GATES_DISTRIBUTED: # Whenever I find a distributed gate extract info of the circuits that need to be updated
                if instrr["circuits"][0] == self._id: # Susbtitute distr gate by local gates if it refences upper_circuit 
                    need_to_modify_self = True
                    if instrr["name"] == "measure_and_send":
                        continue # These ones have been substituted earlier
                    else:
                        instrr["name"] = instrr["name"][6:] # Remove remote_ from the gate name
                        instrr["qubits"][-1] = instrr["qubits"][-1] + n # The control comes from the displaced circuit
                        if instrr["name"] in ["c_if_cx","c_if_cy","c_if_cz"]:
                            instrr["qubits"][-2] = instrr["qubits"][-2] + n

                if instrr["circuits"][0] in instances_to_change_and_displace:
                    instances_to_change_and_displace[instrr["circuits"][0]].append("control" if instrr["name"] == "measure_and_send" else "target")
                else:
                    instances_to_change_and_displace[instrr["circuits"][0]] = ["control" if instrr["name"] == "measure_and_send" else "target"]

            self._add_instruction(instrr)
        
        if need_to_modify_self:
            for instr in self.instructions:
                if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] == other_id):
                    if instr["name"] == "measure_and_send":
                        self.instructions.pop(instr) # Eliminate if from self, it has been substituted by a local gate earlier
                    else:
                        instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                        instr["qubits"][0] = instr["qubits"][0] + n # The control comes from the displaced circuit


        self.update_other_instances(instances_to_change_and_displace, other_id, self._id, n)
        

    # Methods to retrieve information from the circuit

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
                index = [i for i, instr in enumerate(self.instructions) if all(instr[k] == gate[k] for k, _ in gate.items())]
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
            gate (str, dict): gate or sequence of gates to check for presence in the circuit. TODO: accept all of (list[str], str, list[dict], dict)

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
        


    ################ CIRRCUIT DIVIDING METHODS ##################


    def vert_split(self, position: int) -> Tuple["CunqaCircuit", "CunqaCircuit"]:
        """Divides a circuit vertically in two, separating all instructions up to and after a certain layer."""
        n_qubits = self.num_qubits; n_clbits=self.num_clbits

        left_id = self._id + f" left_{position}"
        right_id = self._id + f" right_{position}"

        left_circuit = CunqaCircuit(n_qubits,n_clbits, id=left_id)
        right_circuit = CunqaCircuit(n_qubits,n_clbits, id=right_id)

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
        rest = self.num_qubits - n 
        
        up_id = self._id + f" up_{n}"
        down_id = self._id + f" down_{rest}"

        upper_circuit = CunqaCircuit(n,n, id=up_id)
        lower_circuit = CunqaCircuit(rest, rest, id=down_id)

        up_instrs = []; down_instrs = []
        # Add instructions to the two pieces according to their position
        for instr in self.instructions:
            if instr["name"] in SUPPORTED_GATES_1Q:
                if instr["qubits"][0] > n:
                    down_instrs.append(instr)
                else:
                    up_instrs.append(instr)

            # All subsequent options have more than one qubit        
            elif all(instr["qubits"] > n):
                down_instrs.append(instr)

            elif all(instr["qubits"] < n+1):
                up_instrs.append(instr)

            else: # We have a multiple qubit gate that involves both up and down pieces, needs to be made distributed (classical)

                if instr["qubits"][0] > n: # Down sends to up
                    down_instrs.append({
                        "name": "measure_and_send",
                        "qubits": _flatten(instr["qubits"]),
                        "circuits": [up_id]
                    })
                    up_instrs.append({
                        "name": instr["name"],
                        "qubits": _flatten(instr["qubits"][1:]),
                        "params": instr["params"],
                        "circuits": [down_id]
                    })

                else:                      # Up sends to down
                    up_instrs.append({
                        "name": "measure_and_send",
                        "qubits": _flatten(instr["qubits"]),
                        "circuits": [down_id]
                    })
                    down_instrs.append({
                        "name": instr["name"],
                        "qubits": _flatten(instr["qubits"][1:]),
                        "params": instr["params"],
                        "circuits": [up_id]
                    })

        upper_circuit.from_instructions(up_instrs)
        lower_circuit.from_instructions(down_instrs)

        return upper_circuit, lower_circuit


    ########### DRAWING THE CIRCUIT

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

    ################### ASSIGN THESE METHODS TO CUNQACIRCUIT ###################

    # Update other instances method
    setattr(cls, 'update_other_instances', update_other_instances)

    # Addition methods
    setattr(cls, '__add__', __add__)
    setattr(cls, '__radd__', __radd__)
    setattr(cls, '__iadd__', __iadd__)

    # Union methods
    setattr(cls, '__or__', __or__)
    setattr(cls, '__ror__', __ror__)
    setattr(cls, '__ior__', __ior__)

    # Length method 
    setattr(cls, '__len__', __len__)

    # Additional methods
    setattr(cls, 'param_info', param_info)
    setattr(cls, 'index', index)
    setattr(cls, '__contains__', __contains__)
    setattr(cls, '__getitem__', __getitem__)
    setattr(cls, '__setitem__', __setitem__)

    # Splitting methods
    setattr(cls, 'vert_split', vert_split)
    setattr(cls, 'hor_split', hor_split)

    # Drawing method
    setattr(cls, 'draw', draw)

    return cls

