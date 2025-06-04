"""
    Holds functions to transform between valid circuit formats and extract circuit information.
"""
from cunqa.logger import logger
import numpy as np
import random
import string
import itertools
import functools
from typing import Tuple, Union, Optional
from qiskit import QuantumCircuit

def generate_id(size=4):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choices(chars, k=size))



SUPPORTED_GATES_1Q = ["id","x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg", "u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "measure_and_send", "remote_c_if_h", "remote_c_if_x","remote_c_if_y","remote_c_if_z","remote_c_if_rx","remote_c_if_ry","remote_c_if_rz"]
SUPPORTED_GATES_2Q = ["swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "crx", "cry", "crz", "ecr", "c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz", "c_if_ecr", "remote_c_if_unitary","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz", "remote_c_if_ecr"]
SUPPORTED_GATES_3Q = [ "ccx","ccy", "ccz","cswap"]
SUPPORTED_GATES_PARAMETRIC_1 = ["u1", "p", "rx", "ry", "rz", "rxx", "ryy", "rzz", "rzx","cp", "crx", "cry", "crz", "cu1","c_if_rx","c_if_ry","c_if_rz", "remote_c_if_rx","remote_c_if_ry","remote_c_if_rz"]
SUPPORTED_GATES_PARAMETRIC_2 = ["u2", "r"]
SUPPORTED_GATES_PARAMETRIC_3 = ["u", "u3", "cu3"]
SUPPORTED_GATES_PARAMETRIC_4 = ["cu"]
SUPPORTED_GATES_CONDITIONAL = ["c_if_unitary","c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz","c_if_cx","c_if_cy","c_if_cz", "c_if_ecr"]
SUPPORTED_GATES_DISTRIBUTED = ["measure_and_send", "remote_c_if_unitary", "remote_c_if_h", "remote_c_if_x","remote_c_if_y","remote_c_if_z","remote_c_if_rx","remote_c_if_ry","remote_c_if_rz","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz", "remote_c_if_ecr"]

class CunqaCircuitError(Exception):
    """Exception for error during circuit desing in CunqaCircuit."""
    pass

class InstanceTracker:
    """Decorator that records all created instances and allows any of them to access the other ones."""
    def __init__(self, cls):
        functools.update_wrapper(self, cls) # Ensures that the docstring of the wrapped class is shown and not that of the wrapper
        self._cls = cls
        self._instances = {}

        # Override the __init__ method to track instance creation
        self._original_init = cls.__init__
        cls.__init__ = self._new_init

    def _new_init(self, *args, **kwargs):
        instance = object.__new__(self._cls)
        self._original_init(instance, *args, **kwargs) # Call the original __init__ method
        
        self._instances[instance._id] = instance # Store reference to instance on the key with its id

    def access_other_instances(self):
        return self._instances

    def __call__(self, *args, **kwargs):
        return self._cls(*args, **kwargs)

@InstanceTracker
class CunqaCircuit:
    """
    Class to define a quantum circuit for the `cunqa` api.

    TODO: Indicate supported gates, supported gates dor send() and recv(),... etc

    *** Indicate supported gates ***
    """
    
    id: str
    is_parametric: bool 
    is_distributed: bool
    instructions: "list[dict]"
    quantum_regs: dict
    classical_regs: dict
    sending_to: "list[str]"


    def __init__(self, num_qubits: int, num_clbits: Optional[int] = None, id: Optional[str] = None):

        self.is_parametric = False
        self.is_distributed = False
        self.instructions = []
        self.quantum_regs = {'q0':[q for q in range(num_qubits)]}
        self.classical_regs = {}
        self.sending_to = []

        if not isinstance(num_qubits, int):
            logger.error(f"num_qubits must be an int, but a {type(num_qubits)} was provided [TypeError].")
            raise SystemExit
        
        self.is_parametric = False 

        if id is None:
            self._id = "cunqacircuit_" + generate_id()
        elif isinstance(id, str):
            self._id = id
        else:
            logger.error(f"id must be a str, but a {type(id)} was provided [TypeError].")
            raise SystemExit
        
        if num_clbits is None:
            self.classical_regs = {}
        
        elif isinstance(num_clbits, int):
            self.classical_regs = {'c0':[c for c in range(num_clbits)]}


    @property
    def num_qubits(self) -> int:
        return len(flatten([[q for q in qr] for qr in self.quantum_regs.values()]))
    
    @property
    def info(self) -> dict:
        return {"id":self._id, "instructions":self.instructions, "num_qubits": self.num_qubits,"num_clbits": self.num_clbits,"classical_registers": self.classical_regs,"quantum_registers": self.quantum_regs, "is_distributed":self.is_distributed, "is_parametric":self.is_parametric, "sending_to":self.sending_to}

    @property
    def num_clbits(self):
        return len(flatten([[c for c in cr] for cr in self.classical_regs.values()]))

    """ @property
    def idd(self) -> str:
        return self._id
    
    @idd.setter
    def idd(self, id):
        if self._id is None or not hasattr(self, "_id"):
            self._id = id
        else:
            raise ValueError("Circuit id can only be set once.") """

    def from_instructions(self, instructions):
        for instruction in instructions:
            self._add_instruction(instruction)
        return self


    def _add_instruction(self, instruction):
        """
        Class method to add an instruction to the CunqaCircuit.

        Args:
        --------
        instruction (dict): instruction to be added.
        """
        try:
            self._check_instruction(instruction)
            self.instructions.append(instruction)

        except Exception as error:
            logger.error(f"Error during processing of instruction {instruction} [{CunqaCircuitError.__name__}] [{type(error).__name__}].")
            raise error


    def _check_instruction(self, instruction):
        """
        Class method to check format for circuit instruction. If method finds some inconsistency, raises an error that must be captured avobe.
        
        If format is correct, no error is raise and nothing is returned.

        Args:
        ----------
        instruction (dict): instruction to be checked.
        """

        mandatory_keys = {"name", "qubits"}

        instructions_with_clbits = {"measure"}

        if isinstance(instruction, dict):
        # check if the given instruction has the mandatory keys
            if mandatory_keys.issubset(instruction):
                
                # checking name
                if not isinstance(instruction["name"], str):
                    logger.error(f"instruction name must be str, but {type(instruction['name'])} was provided.")
                    raise TypeError # I capture this at _add_instruction method
                
                if (instruction["name"] in SUPPORTED_GATES_1Q):
                    gate_qubits = 1
                elif (instruction["name"] in SUPPORTED_GATES_2Q) or (instruction["name"] in SUPPORTED_GATES_DISTRIBUTED):
                    # we include as 2 qubit gates the distributed gates
                    gate_qubits = 2
                elif (instruction["name"] in SUPPORTED_GATES_3Q):
                    gate_qubits = 3

                elif any([instruction["name"] == u for u in ["unitary", "c_if_unitary", "remote_c_if_unitary"]]) and ("params" in instruction):
                    # in previous method, format of the matrix is checked, a list must be passed with the correct length given the number of qubits
                    gate_qubits = int(np.log2(len(instruction["params"][0])))
                    if not instruction["name"] == "unitary":
                        gate_qubits += 1 # adding the control qubit

                elif (instruction["name"] in instructions_with_clbits) and ({"qubits", "clbits"}.issubset(instruction)):
                    gate_qubits = 1

                else:
                    logger.error(f"instruction is not supported.")
                    raise ValueError # I capture this at _add_instruction method

                # checking qubits
                if isinstance(instruction["qubits"], list):
                    if not all([isinstance(q, int) for q in instruction["qubits"]]):
                        logger.error(f"instruction qubits must be a list of ints, but a list of {[type(q) for q in instruction['qubits'] if not isinstance(q,int)]} was provided.")
                        raise TypeError
                    elif (len(set(instruction["qubits"])) != len(instruction["qubits"])):
                        logger.error(f"qubits provided for instruction cannot be repeated.")
                        raise ValueError
                else:
                    logger.error(f"instruction qubits must be a list of ints, but {type(instruction['qubits'])} was provided.")
                    raise TypeError # I capture this at _add_instruction method
                
                if not (len(instruction["qubits"]) == gate_qubits):
                    logger.error(f"instruction number of qubits ({gate_qubits}) is not cosistent with qubits provided ({len(instruction['qubits'])}).")
                    raise ValueError # I capture this at _add_instruction method

                if not all([q in flatten([qr for qr in self.quantum_regs.values()]) for q in instruction["qubits"]]):
                    logger.error(f"instruction qubits out of range: {instruction['qubits']} not in {flatten([qr for qr in self.quantum_regs.values()])}.")
                    raise ValueError # I capture this at _add_instruction method


                # checking clibits
                if ("clbits" in instruction) and (instruction["name"] in instructions_with_clbits):

                    if isinstance(instruction["clbits"], list):
                        if not all([isinstance(c, int) for c in instruction["clbits"]]):
                            logger.error(f"instruction clbits must be a list of ints, but a list of {[type(c) for c in instruction['clbits'] if not isinstance(c,int)]} was provided.")
                            raise TypeError
                    else:
                        logger.error(f"instruction clbits must be a list of ints, but {type(instruction['clbits'])} was provided.")
                        raise TypeError # I capture this at _add_instruction method
                    
                    if not all([c in flatten([cr for cr in self.classical_regs.values()]) for c in instruction["clbits"]]):
                        logger.error(f"instruction clbits out of range: {instruction['clbits']} not in {flatten([cr for cr in self.classical_regs.values()])}.")
                        raise ValueError
                    
                elif ("clbits" in instruction) and not (instruction["name"] in instructions_with_clbits):
                    logger.error(f"instruction {instruction['name']} does not support clbits.")
                    raise ValueError
                
                # checking params
                if ("params" in instruction) and (not instruction["name"] in {"unitary", "c_if_unitary", "remote_c_if_unitary"}) and (len(instruction["params"]) != 0):
                    self.is_parametric = True

                    if (instruction["name"] in SUPPORTED_GATES_PARAMETRIC_1):
                        gate_params = 1
                    elif (instruction["name"] in SUPPORTED_GATES_PARAMETRIC_2):
                        gate_params = 2
                    elif (instruction["name"] in SUPPORTED_GATES_PARAMETRIC_3):
                        gate_params = 3
                    elif (instruction["name"] in SUPPORTED_GATES_PARAMETRIC_4):
                        gate_params = 4
                    else:
                        logger.error(f"instruction {instruction['name']} is not parametric, therefore does not accept params.")
                        raise ValueError
                    
                    if not all([(isinstance(p,float) or isinstance(p,int)) for p in instruction["params"]]):
                        logger.error(f"instruction params must be int or float, but {type(instruction['params'])} was provided.")
                        raise TypeError
                    
                    if not len(instruction["params"]) == gate_params:
                        logger.error(f"instruction number of params ({gate_params}) is not consistent with params provided ({len(instruction['params'])}).")
                        raise ValueError
                elif (not ("params" in instruction)) and (instruction["name"] in flatten([SUPPORTED_GATES_PARAMETRIC_1, SUPPORTED_GATES_PARAMETRIC_2, SUPPORTED_GATES_PARAMETRIC_3, SUPPORTED_GATES_PARAMETRIC_4])):
                    logger.error("instruction is parametric, therefore requires params.")
                    raise ValueError
                    
    def _add_q_register(self, name, number_qubits):

        if name in self.quantum_regs:
            i = 0
            new_name = name
            while new_name in self.quantum_regs:
                new_name = new_name + "_" + str(i); i += 1

            logger.warning(f"{name} for quantum register in use, renaming to {new_name}.")
        
        else:
            new_name = name

        self.quantum_regs[new_name] = [(self.num_qubits + 1 + i) for i in range(number_qubits)]

        return new_name
    
    
    def _add_cl_register(self, name, number_clbits):

        if name in self.classical_regs:
            i = 0
            new_name = name
            while new_name in self.classical_regs:
                new_name = new_name + "_" + str(i); i += 1

            logger.warning(f"{name} for classcial register in use, renaaming to {new_name}.")
        
        else:
            new_name = name

        self.classical_regs[new_name] = [(self.num_clbits + i) for i in range(number_clbits)]

        return new_name
    
    # ================ CIRCUIT MODIFICATION METHODS ==============

    # Horizontal concatenation methods

    def update_other_instances(self, instances_to_change, other_id, comb_id, displace_n = 0): # Change other instances that referenced any of the circuits to reference the combined circuit
        other_instances = self.access_other_instances 

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
                    if all([self.is_distributed, other_circuit.is_distributed]): # Here we will have the connectivity check when this function is implemented
                        logger.error(f"Both circuits are distributed. If they reference eachother or are connected through a chain of other circuits execution could wait forever. If you're sure this won't happen try the syntax sum(circ_1, circ_2, force_execution = True).")
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
            if (other_id in instances_to_change or self._id in instances_to_change):
                logger.error("The circuits to be summed contain distributed instructions that reference eachother.")
                raise SystemExit

            summed_circuit = CunqaCircuit(n, n, id = sum_id) 

            for instruction in list(self_instr + other_instr):
                summed_circuit._add_instruction(instruction)

            self.update_other_instances(self, instances_to_change, other_id, sum_id)
            
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
                summed_circuit = CunqaCircuit(n, n, id = sum_id)
                left_instr = qc_to_json(left_circuit)['instructions']
                
                for instruction in list(left_instr + self.instructions):
                    summed_circuit._add_instruction(instruction)

                instances_to_change = {instr["circuits"][0] for instr in self.instructions if instr["name"] in SUPPORTED_GATES_DISTRIBUTED}
                self.update_other_instances(self, instances_to_change, left_id, sum_id) # Update other circuits that communicate with self to reference the summed_circuit

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

            instances_to_change = set()
            if isinstance(other_circuit, CunqaCircuit):
                if not force_execution:
                    if all([self.is_distributed, other_circuit.is_distributed]): # Here we will have the connectivity check when this function is implemented
                        logger.error(f"Both circuits are distributed. If they reference eachother or are connected through a chain of other circuits execution could wait forever. If you're sure this won't happen try the syntax sum(circ_1, circ_2, force_execution = True).")
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
            self.update_other_instances(self, instances_to_change, other_id, self._id)
        
        else:
            logger.error(f"First version only accepts summing circuits with the same number of qubits. Try vertically concatenating (using | ) with an empty circuit to fill the missing qubits {[NotImplemented.__name__]}.")
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
            other_id = other_circuit.id
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
        union_circuit = CunqaCircuit(n+m,n+m, id = union_id)

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


        self.update_other_instances(self, instances_to_change, other_id, union_id) # Update other circuits that communicate with our input circuits to reference the union_circuit
        self.update_other_instances(self, instances_to_change_and_displace, other_id, union_id, n)

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
            union_circuit = CunqaCircuit(n+m,n+m, id = union_id)

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

                if instrr["name"] in SUPPORTED_GATES_DISTRIBUTED: # Gather info on the circuits that reference the other_circuit and wether it controls or is a target
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

            self.update_other_instances(self, instances_to_change_and_displace, upper_id, union_id, n)

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
        
        n=self.num_qubits;     need_to_modify_self = False
        instances_to_change_and_displace = {} # Here we will collect info on the circuits that talk to other_circuit to make them reference self instead
        for instrr in other_instr:
            instrr["qubits"] = [qubit + n for qubit in instrr["qubits"]]
            self._add_instruction(instrr)

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
        
        if need_to_modify_self:
            for instr in self.instructions:
                if (instr["name"] in SUPPORTED_GATES_DISTRIBUTED and instr["circuits"][0] == other_id):
                    if instr["name"] == "measure_and_send":
                        self.instructions.pop(instr) # Eliminate if from self, it has been substituted by a local gate earlier
                    else:
                        instr["name"] = instr["name"][6:] # Remove remote_ from the gate name
                        instr["qubits"][0] = instr["qubits"][0] + n # The control comes from the displaced circuit


        self.update_other_instances(self, instances_to_change_and_displace, other_id, self._id, n)
    

    # Methods to retrieve information from the circuit

    def __len__(self): # TODO: substitute this for circuit depth, ie number of layers, once they are implemented
        """Returns the number of gates on a circuit."""
        return len(self.instructions)
    
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
        
        # Filter through the input format possibilities. Strings could be used to find gate structures while dicts, with more information to match,
        # could be used to find a particular instance of a gate structure.
        # if isinstance(gate_seq, str):
        #     pass
        # if isinstance(gate_seq, dict):
        #     pass
        # if isinstance(gate_seq, list):
        #     if all(isinstance(gate, str) for gate in gate_seq):
        #         pass
        #     if all(isinstance(gate, dict) for gate in gate_seq):
        #         pass

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
        
    # TODO: create circuit dividing methods

    def vert_split(self, position):
        pass

    def hor_split(self, position):
        pass
    
    # =============== INSTRUCTIONS ===============
    
    # Methods for implementing non parametric single-qubit gates

    def id(self, qubit: int) -> None:
        """
        Class method to apply id gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"id",
            "qubits":[qubit]
        })
    
    def x(self, qubit: int) -> None:
        """
        Class method to apply x gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"x",
            "qubits":[qubit]
        })
    
    def y(self, qubit: int) -> None:
        """
        Class method to apply y gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"y",
            "qubits":[qubit]
        })

    def z(self, qubit: int) -> None:
        """
        Class method to apply z gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"z",
            "qubits":[qubit]
        })
    
    def h(self, qubit: int) -> None:
        """
        Class method to apply h gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"h",
            "qubits":[qubit]
        })

    def s(self, qubit: int) -> None:
        """
        Class method to apply s gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"y",
            "qubits":[qubit]
        })

    def sdg(self, qubit: int) -> None:
        """
        Class method to apply sdg gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sdg",
            "qubits":[qubit]
        })

    def sx(self, qubit: int) -> None:
        """
        Class method to apply sx gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sx",
            "qubits":[qubit]
        })
    
    def sxdg(self, qubit: int) -> None:
        """
        Class method to apply sxdg gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sxdg",
            "qubits":[qubit]
        })
    
    def t(self, qubit: int) -> None:
        """
        Class method to apply t gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"t",
            "qubits":[qubit]
        })
    
    def tdg(self, qubit: int) -> None:
        """
        Class method to apply tdg gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"tdg",
            "qubits":[qubit]
        })

    # methods for non parametric two-qubit gates

    def swap(self, *qubits: int) -> None:
        """
        Class method to apply swap gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"swap",
            "qubits":[*qubits]
        })

    def ecr(self, *qubits: int) -> None:
        """
        Class method to apply ecr gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"ecr",
            "qubits":[*qubits]
        })

    def cx(self, *qubits: int) -> None:
        """
        Class method to apply cx gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cx",
            "qubits":[*qubits]
        })
    
    def cy(self, *qubits: int) -> None:
        """
        Class method to apply cy gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cy",
            "qubits":[*qubits]
        })

    def cz(self, *qubits: int) -> None:
        """
        Class method to apply cz gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cz",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits: int) -> None:
        """
        Class method to apply csx gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"csx",
            "qubits":[*qubits]
        })

    # methods for non parametric three-qubit gates

    def ccx(self, *qubits: int) -> None:
        """
        Class method to apply ccx gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits: int) -> None:
        """
        Class method to apply ccy gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccy",
            "qubits":[*qubits]
        })

    def ccz(self, *qubits: int) -> None:
        """
        Class method to apply ccz gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits: int) -> None:
        """
        Class method to apply cswap gate to the given qubits.

        Args:
        --------
        *qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"cswap",
            "qubits":[*qubits]
        })

    
    # methods for parametric single-qubit gates

    def u1(self, param: float, qubit: int) -> None:
        """
        Class method to apply u1 gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"u1",
            "qubits":[qubit],
            "params":[param]
        })
    
    def u2(self, theta: float, phi: float, qubit: int) -> None:
        """
        Class method to apply u2 gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        *params (float or int): parameters for the parametric gate.
        """
        self._add_instruction({
            "name":"u2",
            "qubits":[qubit],
            "params":[theta,phi]
        })

    def u(self, theta: float, phi: float, lam: float, qubit: int) -> None:
        """
        Class method to apply u gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        *params (float or int): parameters for the parametric gate.
        """
        self._add_instruction({
            "name":"u",
            "qubits":[qubit],
            "params":[theta,phi,lam]
        })

    def u3(self, theta: float, phi: float, lam: float, qubit: int) -> None:
        """
        Class method to apply u3 gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        *params (float or int): parameters for the parametric gate.
        """
        self._add_instruction({
            "name":"u3",
            "qubits":[qubit],
            "params":[theta,phi,lam]
        })

    def p(self, param: float, qubit: int) -> None:
        """
        Class method to apply p gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"p",
            "qubits":[qubit],
            "params":[param]
        })

    def r(self, theta: float, phi: float, qubit: int) -> None:
        """
        Class method to apply r gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        *params (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"r",
            "qubits":[qubit],
            "params":[theta, phi]
        })

    def rx(self, param: float, qubit: int) -> None:
        """
        Class method to apply rx gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"rx",
            "qubits":[qubit],
            "params":[param]
        })

    def ry(self, param: float, qubit: int) -> None:
        """
        Class method to apply ry gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"ry",
            "qubits":[qubit],
            "params":[param]
        })
    
    def rz(self, param: float, qubit: int) -> None:
        """
        Class method to apply rz gate to the given qubit.

        Args:
        --------
        qubit (int): qubit in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"rz",
            "qubits":[qubit],
            "params":[param]
        })

    # methods for parametric two-qubit gates

    def rxx(self, param: float, *qubits: int) -> None:
        """
        Class method to apply rxx gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"rxx",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def ryy(self, param: float, *qubits: int) -> None:
        """
        Class method to apply ryy gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"ryy",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzz(self, param: float, *qubits: int) -> None:
        """
        Class method to apply rzz gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"rzz",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzx(self, param: float, *qubits: int) -> None:
        """
        Class method to apply rzx gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"rzx",
            "qubits":[*qubits],
            "params":[param]
        })

    def crx(self, param: float, *qubits: int) -> None:
        """
        Class method to apply crx gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"crx",
            "qubits":[*qubits],
            "params":[param]
        })

    def cry(self, param: float, *qubits: int) -> None:
        """
        Class method to apply cry gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"cry",
            "qubits":[*qubits],
            "params":[param]
        })

    def crz(self, param: float, *qubits: int) -> None:
        """
        Class method to apply crz gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"crz",
            "qubits":[*qubits],
            "params":[param]
        })

    def cp(self, param: float, *qubits: int) -> None:
        """
        Class method to apply cp gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"cp",
            "qubits":[*qubits],
            "params":[param]
        })

    def cu1(self, param: float, *qubits: int) -> None:
        """
        Class method to apply cu1 gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        param (float or int): parameter for the parametric gate.
        """
        self._add_instruction({
            "name":"cu1",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def cu3(self, theta: float, phi: float, lam: float, *qubits: int) -> None: # three parameters
        """
        Class method to apply cu3 gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

        *params (float or int): parameters for the parametric gate.
        """
        self._add_instruction({
            "name":"cu3",
            "qubits":[*qubits],
            "params":[theta,phi,lam]
        })
    
    def cu(self, theta: float, phi: float, lam: float, gamma: float, *qubits: int) -> None: # four parameters
        """
        Class method to apply cu gate to the given qubits.

        Args:
        --------
        qubits (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.

       *params (float or int): parameters for the parametric gate.
        """
        self._add_instruction({
            "name":"cu",
            "qubits":[*qubits],
            "params":[theta, phi, lam, gamma]
        })
    

    # methods for implementing conditional LOCAL gates
    def unitary(self, matrix: "list[list[list[complex]]]", *qubits: int) -> None:
        """
        Class method to apply a unitary gate created from an unitary matrix provided.

        Args:
        -------
        matrix (list or <class 'numpy.ndarray'>): unitary operator in matrix form to be applied to the given qubits.

        *qubits (int): qubits to which the unitary operator will be applied.

        """
        if isinstance(matrix, np.ndarray) and (matrix.shape[0] == matrix.shape[1]) and (matrix.shape[0]%2 == 0):
            matrix = list(matrix)
        elif isinstance(matrix, list) and isinstance(matrix[0], list) and all([len(matrix) == len(m) for m in matrix]) and (len(matrix)%2 == 0):
            matrix = matrix
        else:
            logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape (2^n,2^n) [TypeError].")
            raise SystemExit # User's level
        
        matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]


        self._add_instruction({
            "name":"unitary",
            "qubits":[*qubits],
            "params":[matrix]
        })
        

    def measure(self, qubits: Union[int, "list[int]"], clbits: Union[int, "list[int]"]) -> None:
        """
        Class method to add a measurement of a qubit or a list of qubits and to register that measurement in the given classical bits.

        Args:
        --------
        qubits (list[int] or int): qubits to measure.

        clbits (list[int] or int): clasical bits where the measurement will be registered.
        """
        if not (isinstance(qubits, list) and isinstance(clbits, list)):
            list_qubits = [qubits]; list_clbits = [clbits]
        else:
            list_qubits = qubits; list_clbits = clbits
        
        for q,c in zip(list_qubits, list_clbits):
            self._add_instruction({
                "name":"measure",
                "qubits":[q],
                "clbits":[c]
            })

    def measure_all(self) -> None:
        """
        Class to apply a global measurement of all of the qubits of the circuit. An additional classcial register will be added and labeled as "measure".
        """
        new_clreg = "measure"

        new_clreg = self._add_cl_register(new_clreg, self.num_qubits)

        for q in range(self.num_qubits):

            self._add_instruction({
                "name":"measure",
                "qubits":[q],
                "clbits":[self.classical_regs[new_clreg][q]]
            })


    def c_if(self, gate: str, control_qubit: int, target_qubit: int, param: Optional[float] = None, matrix: Optional["list[list[list[complex]]]"] = None) -> None:
        """
        Method for implementing a gate contiioned to a classical measurement. The control qubit provided is measured, if it's 1 the gate provided is applied to the given qubits.

        For parametric gates, only one-parameter gates are supported, therefore only one parameter must be passed.

        Args:
        --------
        gate (str): gate to be applied. Has to be supported by CunqaCircuit.

        control (int): control qubit whose classical measurement will control the execution of the gate.

        target (list[int], int): list of qubits or qubit to which the gate is intended to be applied.

        param (float or int): parameter for the case parametric gate is provided.
        """
        
        if isinstance(gate, str):
            name = "c_if_" + gate
        else:
            logger.error(f"gate specification must be str, but {type(gate)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(control_qubit, int):
            list_control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubit, int):
            list_target_qubit = [target_qubit]
        elif isinstance(target_qubit, list):
            list_target_qubit = target_qubit
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(target_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if (gate == "unitary") and (matrix is not None):

            if isinstance(matrix, np.ndarray) and (matrix.shape[0] == matrix.shape[1]) and (matrix.shape[0]%2 == 0):
                matrix = list(matrix)
            elif isinstance(matrix, list) and isinstance(matrix[0], list) and all([len(m) == len(matrix) for m in matrix]) and (len(matrix)%2 == 0):
                matrix = matrix
            else:
                logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape (2^n,2^n) [TypeError].")
                raise SystemExit # User's level

            matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]

            self._add_instruction({
                "name": name,
                "qubits": flatten([list_control_qubit, list_target_qubit]),
                "params":[matrix]
            })
            # we have to exit here
            return

        elif (gate == "unitary") and (matrix is None):
            logger.error(f"For unitary gate a matrix must be provided [ValueError].")
            raise SystemExit # User's level
        
        elif (gate != "unitary") and (matrix is not None):
            logger.error(f"instruction {gate} does not suppor matrix.")
            raise SystemExit

        
        if gate in SUPPORTED_GATES_PARAMETRIC_1:
            if param is None:
                logger.error(f"Since a parametric gate was provided ({gate}) a parameter should be passed [ValueError].")
                raise SystemExit
            elif isinstance(param, float) or isinstance(param, int):
                list_param = [param]
            else:
                logger.error(f"param must be int or float, but {type(param)} was provided [TypeError].")
                raise SystemExit
        else:
            if param is not None:
                logger.warning("A parameter was provided but gate is not parametric, therefore it will be ignored.")
            list_param = []


        
        if name in SUPPORTED_GATES_CONDITIONAL:

            self._add_instruction({
                "name": name,
                "qubits": flatten([list_control_qubit, list_target_qubit]),
                "params":list_param
            })

        else:
            logger.error(f"Gate {gate} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing 
        

    def measure_and_send(self, control_qubit: Optional[int] = None, target_circuit: Optional[Union[str, 'CunqaCircuit']] = None) -> None:
        """
        Class method to measure and send a bit from the current circuit to a remote one.
        
        Args:
        -------

        control_qubit (int): control qubit from self.

        target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit to which we will send the gate or the circuit itself.

        """

        self.is_distributed = True

        if isinstance(control_qubit, int):
            list_control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} was provided [TypeError].")
            raise SystemExit

        if target_circuit is None:
            logger.error("target_circuit not provided.")
            raise SystemExit
        
        elif isinstance(target_circuit, str):
            target_circuit_id = target_circuit

        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit.id
        else:
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        

        self._add_instruction({
            "name": "measure_and_send",
            "qubits": flatten([list_control_qubit]),
            "circuits": [target_circuit_id]
        })

        self.sending_to.append(target_circuit_id)


    def remote_c_if(self, gate: str, target_qubits: Union[int, "list[int]"], param: float, control_circuit: Optional[Union[str, 'CunqaCircuit']] = None)-> None:
        """
        Class method to apply a distributed instruction as a gate condioned by a non local classical measurement from a remote circuit and applied locally.
        
        Args:
        -------
        gate (str): gate to be applied. Has to be supported by CunqaCircuit.

        param (float or int): parameter in case the gate provided is parametric.

        control_qubit (int): control qubit from self.

        target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit to which we will send the gate or the circuit itself.

        target_qubit (int): qubit where the gate will be conditionally applied.       
        """

        self.is_distributed = True

        if isinstance(gate, str):
            name = "remote_c_if_" + gate
        else:
            logger.error(f"gate specification must be str, but {type(gate)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubits, int):
            target_qubits = [target_qubits]
        elif isinstance(target_qubits, list):
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(target_qubits)} was provided [TypeError].")
            raise SystemExit
        
        if param is not None:
            params = [param]
        else:
            params = []

        if control_circuit is None:
            logger.error("target_circuit not provided.")
            raise SystemExit
        
        elif isinstance(control_circuit, str):
            control_circuit = control_circuit

        elif isinstance(control_circuit, CunqaCircuit):
            control_circuit = control_circuit.id
        else:
            logger.error(f"control_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(control_circuit)} was provided [TypeError].")
            raise SystemExit
        
        if name in SUPPORTED_GATES_DISTRIBUTED:

            self._add_instruction({
                "name": name,
                "qubits": flatten([target_qubits]),
                "params":params,
                "circuits": [control_circuit]
            })
        else:
            logger.error(f"Gate {name} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing
                

def flatten(lists: "list[list]"):
    return [element for sublist in lists for element in sublist]


###################################################################################
######################### INTEGRATION WITH QISKIT BLOCK ###########################
###################################################################################
from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit

def qc_to_json(qc: QuantumCircuit) -> dict:
    """
    Transforms a QuantumCircuit to json dict.

    Args:
    ---------
    qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): circuit to transform to json.

    Return:
    ---------
    Json dict with the circuit information.
    """
    # Check validity of the provided quantum circuit
    if isinstance(qc, dict):
        logger.warning(f"Circuit provided is already a dict.")
        return qc
    elif isinstance(qc, QuantumCircuit):
        pass
    else:
        logger.error(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or dict, but {type(qc)} was provided [{TypeError.__name__}].")
        raise TypeError # this error should not be raised bacause in QPU we already check type of the circuit

    # Actual translation
    try:
        
        quantum_registers, classical_registers = registers_dict(qc)
        
        json_data = {
            "id": "",
            "is_parametric": _is_parametric(qc),
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }
        for i in range(len(qc.data)):
            if qc.data[i].name == "barrier":
                pass
            elif qc.data[i].name == "unitary":
                qreg = [r._register.name for r in qc.data[i].qubits]
                qubit = [q._index for q in qc.data[i].qubits]

                json_data["instructions"].append({"name":qc.data[i].name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg,qubit)],
                                                "params":[[list(map(lambda z: [z.real, z.imag], row)) for row in qc.data[i].params[0].tolist()]] #only difference, it ensures that the matrix appears as a list, and converts a+bj to (a,b)
                                                })
            elif qc.data[i].name != "measure":

                qreg = [r._register.name for r in qc.data[i].qubits]
                qubit = [q._index for q in qc.data[i].qubits]

                json_data["instructions"].append({"name":qc.data[i].name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg,qubit)],
                                                "params":qc.data[i].params
                                                })
            else:
                qreg = [r._register.name for r in qc.data[i].qubits]
                qubit = [q._index for q in qc.data[i].qubits]
                
                creg = [r._register.name for r in qc.data[i].clbits]
                bit = [b._index for b in qc.data[i].clbits]

                json_data["instructions"].append({"name":qc.data[i].name,
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg,qubit)],
                                                "clbits":[classical_registers[k][b] for k,b in zip(creg,bit)]
                                                })
                    

        return json_data
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from QuantumCircuit to json dict [{type(error).__name__}].")
        raise error


def from_json_to_qc(circuit_dict: dict) -> 'QuantumCircuit':
    """
    Function to transform a circuit in json dict format to <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.

    Args:
    ----------
    circuit_dict (dict): circuit to be transformed to QuantumCircuit.

    Return:
    -----------
    QuantumCircuit with the given instructions.

    """
    # Checking validity of the provided circuit
    if isinstance(circuit_dict, QuantumCircuit):
        logger.warning("Circuit provided is already <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.")
        return circuit_dict

    elif isinstance(circuit_dict, dict):
        circuit = circuit_dict
    else:
        logger.error(f"circuit_dict must be dict, but {type(circuit_dict)} was provided [{TypeError.__name__}]")
        raise TypeError

    #Extract key information from the json
    try:
        instructions = circuit['instructions']
        num_qubits = circuit['num_qubits']
        classical_registers = circuit['classical_registers']

    except KeyError as error:
        logger.error(f"Circuit json not correct, requiered keys must be: 'instructions', 'num_qubits', 'num_clbits', 'quantum_resgisters' and 'classical_registers' [{type(error).__name__}].")
        raise error
        
    # Proceed with translation
    try:
    
        qc = QuantumCircuit(num_qubits)

        bits = []
        for cr, lista in classical_registers.items():
            for i in lista: 
                bits.append(i)
            qc.add_register(ClassicalRegister(len(lista), cr))


        for instruction in instructions:
            if instruction['name'] != 'measure':
                if 'params' in instruction:
                    params = instruction['params']
                else:
                    params = []
                inst = CircuitInstruction( 
                    operation = Instruction(name = instruction['name'],
                                            num_qubits = len(instruction['qubits']),
                                            num_clbits = 0,
                                            params = params
                                            ),
                    qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                    clbits = ()
                    )
                qc.append(inst)
            elif instruction['name'] == 'measure':
                bit = instruction['clbits'][0]
                if bit in bits: # checking that the bit referenced in the instruction it actually belongs to a register
                    for k,v in classical_registers.items():
                        if bit in v:
                            reg = k
                            l = len(v)
                            clbit = v.index(bit)
                            inst = CircuitInstruction(
                                operation = Instruction(name = instruction['name'],
                                                        num_qubits = 1,
                                                        num_clbits = 1,
                                                        params = []
                                                        ),
                                qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                                clbits = (Clbit(ClassicalRegister(l, reg), clbit),)
                                )
                            qc.append(inst)
                else:
                    logger.error(f"Bit {bit} not found in {bits}, please check the format of the circuit json.")
                    raise IndexError
        return qc
    
    except KeyError as error:
        logger.error(f"Some error with the keys of `instructions` occured, please check the format [{type(error).__name__}].")
        raise error
    
    except TypeError as error:
        logger.error(f"Error when reading instructions, check that the given elements have the correct type [{type(error).__name__}].")
        raise TypeError
    
    except IndexError as error:
        logger.error(f"Error with format for classical_registers [{type(error).__name__}].")
        raise error

    except Exception as error:
        logger.error(f"Error when converting json dict to QuantumCircuit [{type(error).__name__}].")
        raise error


def registers_dict(qc: 'QuantumCircuit') -> "list[dict]":
    """
    Extracts the number of classical and quantum registers from a QuantumCircuit.

    Args
    -------
     qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): quantum circuit whose number of registers we want to know

    Return:
    --------
    Two element list with quantum and classical registers, in that order.
    """

    quantum_registers = {}
    for qr in qc.qregs:
        quantum_registers[qr.name] = qr.size

    countsq = []

    valuesq = list(quantum_registers.values())

    for i, v in enumerate(valuesq):
        if i == 0:
            countsq.append(list(range(0, v)))
        else:
            countsq.append(list(range(sum(valuesq[:i]), sum(valuesq[:i])+v)))

    for i,k in enumerate(quantum_registers.keys()):
        quantum_registers[k] = countsq[i]

    classical_registers = {}
    for cr in qc.cregs:
        classical_registers[cr.name] = cr.size

    counts = []

    values = list(classical_registers.values())

    for i, v in enumerate(values):
        if i == 0:
            counts.append(list(range(0, v)))
        else:
            counts.append(list(range(sum(values[:i]), sum(values[:i])+v)))

    for i,k in enumerate(classical_registers.keys()):
        classical_registers[k] = counts[i]

    return [quantum_registers, classical_registers]


def _is_parametric(circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit']) -> bool:
    """
    Function to determine weather a cirucit has gates that accept parameters, not necesarily parametric <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.
    For example, a circuit that is composed by hadamard and cnot gates is not a parametric circuit; but if a circuit has any of the gates defined in `parametric_gates` we
    consider it a parametric circuit for our purposes.

    Args:
    -------
    circuit (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, dict or str): the circuit from which we want to find out if it's parametric.

    Return:
    -------
    True if the circuit is considered parametric, False if it's not.
    """
    parametric_gates = ["u", "u1", "u2", "u3", "rx", "ry", "rz", "crx", "cry", "crz", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "cp", "cswap", "ccx", "crz", "cu"]
    if isinstance(circuit, QuantumCircuit):
        for instruction in circuit.data:
            if instruction.operation.name in parametric_gates:
                return True
        return False
    elif isinstance(circuit, dict):
        for instruction in circuit['instructions']:
            if instruction['name'] in parametric_gates:
                return True
        return False
    elif isinstance(circuit, list):
        for instruction in circuit:
            if instruction['name'] in parametric_gates:
                return True
        return False
    elif isinstance(circuit, CunqaCircuit):
        return circuit.is_parametric
    elif isinstance(circuit, str):
        lines = circuit.splitlines()
        for line in lines:
            line = line.strip()
            if any(line.startswith(gate) for gate in parametric_gates):
                return True
        return False

def all_suborder_preserving_shuffles(*lists):
    """Randomly combines lists while preserving each or their internal order.
    
    Args:
        *lists (list[list]): list of lists to be combined.
    Returns:
        permutations (list[list]): all possible ways to combine the input lists such that the resulting list preserves the order of each input sublist
    """   
    lst_to_combine = list(*lists)
    list_ids = [i for i, _ in enumerate(lst_to_combine) for _ in range(len(lists[i]))]
    permutations = list(itertools.permutations(list_ids))

    pointers = [0] * len(lists) # list with elements that track in which position of each of the lists we are: [i, j, k] going from all zeros to [len(list_1), len(list_2), len(list_3)]
    
    for perm in permutations:
        for i, list_id in enumerate(perm):
            perm[i] = lst_to_combine[list_id][pointers[list_id]]
            pointers[list_id] += 1

    return permutations

