"""
    Holds Cunqa's custom circuit class and functions to translate its instructions into other formats for circuit definition.
"""
import numpy as np
import random
import string
import itertools
import functools
from collections import defaultdict
from typing import Tuple, Union, Optional
from qiskit import QuantumCircuit
from cunqa.logger import logger

import matplotlib.pyplot as plt #I use this for drawing circuits with LaTeX (quantikz)
from matplotlib import rc

def _generate_id(size: int = 4) -> str:
    """Return a random alphanumeric identifier.

    Args:
        size (int): Desired length of the identifier.  
                    Defaults to 4 and must be a positive integer.

    Returns:
        A string of uppercase/lowercase letters and digits.

    Return type:
        str

    Raises:
        ValueError: If *size* is smaller than 1.
    """
    if size < 1:
        raise ValueError("size must be >= 1")
    
    chars = string.ascii_letters + string.digits
    return ''.join(random.choices(chars, k=size))



SUPPORTED_GATES_1Q = ["id","x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg", "u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz", "measure_and_send", "remote_c_if_h", "remote_c_if_x","remote_c_if_y","remote_c_if_z","remote_c_if_rx","remote_c_if_ry","remote_c_if_rz", "remote_c_if_ecr"]
SUPPORTED_GATES_2Q = ["swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "crx", "cry", "crz", "ecr", "c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz", "c_if_ecr", "remote_c_if_unitary","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz"]
SUPPORTED_GATES_3Q = [ "ccx","ccy", "ccz","cswap"]
SUPPORTED_GATES_PARAMETRIC_1 = ["u1", "p", "rx", "ry", "rz", "rxx", "ryy", "rzz", "rzx","cp", "crx", "cry", "crz", "cu1","c_if_rx","c_if_ry","c_if_rz"]
SUPPORTED_GATES_PARAMETRIC_2 = ["u2", "r"]
SUPPORTED_GATES_PARAMETRIC_3 = ["u", "u3", "cu3"]
SUPPORTED_GATES_PARAMETRIC_4 = ["cu"]
SUPPORTED_GATES_CONDITIONAL = ["c_if_unitary","c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz","c_if_cx","c_if_cy","c_if_cz", "c_if_ecr"]
SUPPORTED_GATES_DISTRIBUTED = ["measure_and_send", "remote_c_if_unitary", "remote_c_if_h", "remote_c_if_x","remote_c_if_y","remote_c_if_z","remote_c_if_rx","remote_c_if_ry","remote_c_if_rz","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz", "remote_c_if_ecr"]
SUPPORTED_GATES_QDISTRIBUTED = ["qsend", "distr_unitary", "distr_h", "distr_x", "distr_y", "distr_z", "distr_rx", "distr_ry", "distr_rz", "distr_cx","distr_cy","distr_cz","distr_ecr"]

class CunqaCircuitError(Exception):
    """Exception for error during circuit desing in ``CunqaCircuit``."""
    pass

class InstanceTrackerMeta(type):
    """Metaclass to track instances of CunqaCircuits and extract the connections between them."""
    _instances = {}
    _connected = None
    _new_inst = False

    def __call__(cls, *args, **kwargs):
        # Create the instance using the original __call__ method
        instance = super().__call__(*args, **kwargs)
        
        # Store the instance 
        cls._instances[instance._id] = instance
        
        return instance

    def access_other_instances(cls):
        return cls._instances
    
    def get_connectivity(cls):
            if (cls._connected is None or cls._new_inst):
                # sending_to has the circuits where you send but not the received ones, the graph is directed. We use sets to undirect the graph
                first_connections = {frozenset({idd, sent}) for idd, circuit in cls.access_other_instances().items() for sent in circuit.sending_to}
                # adds (a,c) if (a,b) and (b,c) are in the set. Does this recursively until the highest level transitivity is addressed
                cls._connected = transitive_combinations(first_connections) 
                cls._new_inst = False

            return cls._connected

class CunqaCircuit(metaclass=InstanceTrackerMeta):
    # TODO: look for other alternatives for describing the documentation that do not requiere such long docstrings, maybe gatehring everything in another file and using decorators, as in ther APIs.
    """
    Class to define a quantum circuit for the ``cunqa`` api.

    This class serves as a tool for the user to describe not only simple circuits, but also to describe classical and quantum operations between circuits.
    On its initialization, it takes as mandatory the number of qubits for the circuit (*num_qubits*), also number of classical bits (*num_clbits*) and a personalized id (*id*), which by default would be randomly generated.
    Once the object is created, class methods canbe used to add instructions to the circuit such as single-qubit and two-qubits gates, measurements, conditional operations,... but also operations that allow to send measurement outcomes or qubits to other circuits.
    This sending operations require that the virtual QPUs to which the circuits are sent support classical or quantum communications with the desired connectivity.
    
    Supported operations
    ----------------------

    **Single-qubit gates:**
    :py:meth:`~CunqaCircuit.id`, :py:meth:`~CunqaCircuit.x`, :py:meth:`~CunqaCircuit.y`, :py:meth:`~CunqaCircuit.z`, :py:meth:`~CunqaCircuit.h`,  :py:meth:`~CunqaCircuit.s`,
    :py:meth:`~CunqaCircuit.sdg`, :py:meth:`~CunqaCircuit.sx`, :py:meth:`~CunqaCircuit.sxdg`, :py:meth:`~CunqaCircuit.t`, :py:meth:`~CunqaCircuit.tdg`, :py:meth:`~CunqaCircuit.u1`,
    :py:meth:`~CunqaCircuit.u2`, :py:meth:`~CunqaCircuit.u3`, :py:meth:`~CunqaCircuit.u`, :py:meth:`~CunqaCircuit.p`, :py:meth:`~CunqaCircuit.r`, :py:meth:`~CunqaCircuit.rx`,
    :py:meth:`~CunqaCircuit.ry`, :py:meth:`~CunqaCircuit.rz`.

    **Two-qubits gates:**
    :py:meth:`~CunqaCircuit.swap`, :py:meth:`~CunqaCircuit.cx`, :py:meth:`~CunqaCircuit.cy`, :py:meth:`~CunqaCircuit.cz`, :py:meth:`~CunqaCircuit.csx`, :py:meth:`~CunqaCircuit.cp`,
    :py:meth:`~CunqaCircuit.cu`, :py:meth:`~CunqaCircuit.cu1`, :py:meth:`~CunqaCircuit.cu3`, :py:meth:`~CunqaCircuit.rxx`, :py:meth:`~CunqaCircuit.ryy`, :py:meth:`~CunqaCircuit.rzz`,
    :py:meth:`~CunqaCircuit.rzx`, :py:meth:`~CunqaCircuit.crx`, :py:meth:`~CunqaCircuit.cry`, :py:meth:`~CunqaCircuit.crz`, :py:meth:`~CunqaCircuit.ecr`,

    **Three-qubits gates:**
    :py:meth:`~CunqaCircuit.ccx`, :py:meth:`~CunqaCircuit.ccy`, :py:meth:`~CunqaCircuit.ccz`, :py:meth:`~CunqaCircuit.cswap`.

    **n-qubits gates:**
    :py:meth:`~CunqaCircuit.unitary`.

    **Non-unitary local operations:**
    :py:meth:`~CunqaCircuit.c_if`, :py:meth:`~CunqaCircuit.measure`, :py:meth:`~CunqaCircuit.measure_all`, :py:meth:`~CunqaCircuit.reset`.

    **Remote operations for classical communications:**
    :py:meth:`~CunqaCircuit.measure_and_send`, :py:meth:`~CunqaCircuit.remote_c_if`.

    **Remote operations for quantum comminications:**
    :py:meth:`~CunqaCircuit.qsend`, :py:meth:`~CunqaCircuit.qrecv`.

    Creating your first CunqaCircuit
    ---------------------------------

    Start by instantiating the class providing the desired number of qubits:

        >>> circuit = CunqaCircuit(2)

    Then, gates can be added through the mentioned methods. Let's add a Hadamard gate and CNOT gates to create a Bell state:

        >>> circuit.h(0) # adding hadamard to qubit 0
        >>> circuit.cx(0,1)

    Finally, qubits are measured by:

        >>> circuit.measure_all()

    Once the circuit is ready, it is ready to be sent to a QPU by the method :py:meth:`cunqa.qpu.run`.

    Classical communications among circuits
    ---------------------------------------

    The strong part of CunqaCircuit is that it allows to define communication directives between circuits.
    We can define the sending of a classical bit from one circuit to another by:

        >>> circuit_1 = CunqaCircuit(2)
        >>> circuit_2 = CunqaCircuit(2)
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, circuit_2) # qubit 0 is measured and the outcome is sent to circuit_2
        >>> circuit_2.remote_c_if("x", circuit_1) # the outcome is recived to perform a classicaly controlled operation
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Then, circuits can be sent to QPUs that support classical communications using the :py:meth:`cunqa.mappers.run_distributed` function.

    Circuits can also be referend to through their *id* string. When a CunqaCircuit is created, by default a random *id* is assigned, but it can also be personalized:

        >>> circuit_1 = CunqaCircuit(2, id = "1")
        >>> circuit_2 = CunqaCircuit(2, id = "2")
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, "2") # qubit 0 is measured and the outcome is sent to circuit_2
        >>> circuit_2.remote_c_if("x", "1") # the outcome is recived to perform a classicaly controlled operation
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Sending qubits between circuits
    --------------------------------

    When quantum communications among the QPUs utilized are available, a qubit from one circuit can be sent to another.
    In this scheme, generally an acilla qubit would be neccesary to perform the communication. Let's see an example for the creation of a Bell pair remotely:

        >>> circuit_1 = CunqaCircuit(2, 1, id = "1")
        >>> circuit_2 = CunqaCircuit(2, 2, id = "2")
        >>>
        >>> circuit_1.h(0); circuit_1.cx(0,1)
        >>> circuit_1.qsend(1, "1") # sending qubit 1 to circuit with id "2"
        >>> circuit_1.measure(0,0)
        >>>
        >>> circuit_2.qrecv(0, "2") # reciving qubit from circuit with id "1" and assigning it to qubit 0
        >>> circuit_2.cx(0,1)
        >>> circuit_2.measure_all()

    It is important to note that the qubit used for the communication, the one send, after the operation it is reset, so in a general basis it wouldn't need to be measured.
    If we want to send more qubits afer, we can use it since it is reset to zero.

    Properties:
    -----------
        quantum_regs: dict
            Dictionary of quantum registers of the circuit as {"name" : <list of qubits assigned>}.

        classical_regs: dict
            Dictionary of classical registers of the circuit as {"name" : <list of clbits assigned>}

        instructions: list
            Set of operations applied to the circuit.

        is_parametric: bool
            Weather the circuit contains parametric gates.

        has_cc: bool
            Weather the circuit contains classical communications with other circuit.

        is_dynamic: bool
            Weather the circuit has local non-unitary operations.

        sending_to: list[str]
            List of circuit ids to which the current circuit is sending measurement outcomes or qubits.

    """
    
    id: str
    is_parametric: bool 
    has_cc: bool 
    is_dynamic: bool
    instructions: "list[dict]"
    quantum_regs: dict
    classical_regs: dict
    sending_to: "list[str]"


    def __init__(self, num_qubits: int, num_clbits: Optional[int] = None, id: Optional[str] = None):

        self.is_parametric = False
        self.has_cc = False
        self.is_dynamic = False
        self.instructions = []
        self.quantum_regs = {'q0':[q for q in range(num_qubits)]}
        self.classical_regs = {}
        self.sending_to = []

        if not isinstance(num_qubits, int):
            logger.error(f"num_qubits must be an int, but a {type(num_qubits)} was provided [TypeError].")
            raise SystemExit
        
        self.is_parametric = False 

        if id is None:
            self._id = "CunqaCircuit_" + _generate_id()
        elif isinstance(id, str):
            other_circuits = self.__class__.access_other_instances()
            if id in other_circuits.keys():
                logger.error(f"Id {id} was already used for another circuit.")
                raise SystemExit
            else:
                self._id = id
        else:
            logger.error(f"id must be a str, but a {type(id)} was provided [TypeError].")
            raise SystemExit

        if num_clbits is None:
            self.classical_regs = {}
        
        elif isinstance(num_clbits, int):
            self.classical_regs = {'c0':[c for c in range(num_clbits)]}



    @property
    def info(self) -> dict:
        """
        Information about the main class attributes given as a dictinary.
        """
        return {"id":self._id, "instructions":self.instructions, "num_qubits": self.num_qubits,"num_clbits": self.num_clbits,"classical_registers": self.classical_regs,"quantum_registers": self.quantum_regs, "has_cc":self.has_cc, "is_dynamic":self.is_dynamic, "sending_to":self.sending_to}

    @property
    def num_qubits(self) -> int:
        return len(_flatten([[q for q in qr] for qr in self.quantum_regs.values()]))
    
    @property
    def info(self) -> dict:
        return {"id":self._id, "instructions":self.instructions, "num_qubits": self.num_qubits,"num_clbits": self.num_clbits,"classical_registers": self.classical_regs,"quantum_registers": self.quantum_regs, "is_distributed":self.is_distributed, "is_parametric":self.is_parametric, "sending_to":self.sending_to}

    @property
    def num_clbits(self):
        return len(_flatten([[c for c in cr] for cr in self.classical_regs.values()]))
    
    @property
    def layers(self):
        layer_dict = {f"{i}": [] for i in range(self.num_qubits)} 
        self.last_active_layer = [0 for _ in range(self.num_qubits)]

        for i, instr in enumerate(self.instructions):
            if instr["name"] in SUPPORTED_GATES_1Q:
                self.last_active_layer[instr["qubits"][0]]+=1
                layer_dict[str(instr["qubits"][0])].append([self.last_active_layer[instr["qubits"][0]], i, instr["name"]])

            elif instr["name"] in SUPPORTED_GATES_2Q + SUPPORTED_GATES_3Q:
                max_layer_qubit = max([self.last_active_layer[j] for j in instr["qubits"]])
                for j in instr["qubits"]:
                    self.last_active_layer[j] = max_layer_qubit + 1
                    layer_dict[str(j)].append([self.last_active_layer[j], i, instr["name"]])

        return layer_dict

    def from_instructions(self, instructions):
        """
        Class method to 
        """
        for instruction in instructions:
            self._add_instruction(instruction)
        return self


    def _add_instruction(self, instruction):
        """
        Class method to add an instruction to the CunqaCircuit.

        Args:
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
                elif (instruction["name"] in SUPPORTED_GATES_2Q):
                    gate_qubits = 2
                elif (instruction["name"] in SUPPORTED_GATES_3Q):
                    gate_qubits = 3
                elif (instruction["name"] == "measure_and_send"):
                    gate_qubits = 2
                elif (instruction["name"] == "recv"):
                    gate_qubits = 0

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

                if not all([q in _flatten([qr for qr in self.quantum_regs.values()]) for q in instruction["qubits"]]):
                    logger.error(f"instruction qubits out of range: {instruction['qubits']} not in {_flatten([qr for qr in self.quantum_regs.values()])}.")
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
                    
                    if not all([c in _flatten([cr for cr in self.classical_regs.values()]) for c in instruction["clbits"]]):
                        logger.error(f"instruction clbits out of range: {instruction['clbits']} not in {_flatten([cr for cr in self.classical_regs.values()])}.")
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
                elif (not ("params" in instruction)) and (instruction["name"] in _flatten([SUPPORTED_GATES_PARAMETRIC_1, SUPPORTED_GATES_PARAMETRIC_2, SUPPORTED_GATES_PARAMETRIC_3, SUPPORTED_GATES_PARAMETRIC_4])):
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

            summed_circuit = CunqaCircuit(n, n, id = sum_id) 

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
                summed_circuit = CunqaCircuit(n, n, id = sum_id)
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


        self.update_other_instances(instances_to_change_and_displace, other_id, self._id, n)
    

    # Methods to retrieve information from the circuit

    def __len__(self): 
        """Returns the layer depth of the circuit."""
        self.layers
        return max(self.last_active_layer)
    
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
    
    # =============== INSTRUCTIONS ===============
    
    # Methods for implementing non parametric single-qubit gates

    def id(self, qubit: int) -> None:
        """
        Class method to apply id gate to the given qubit.

        Args:
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
            ``*qubits`` (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"swap",
            "qubits":[*qubits]
        })

    def ecr(self, *qubits: int) -> None:
        """
        Class method to apply ecr gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"ecr",
            "qubits":[*qubits]
        })

    def cx(self, *qubits: int) -> None:
        """
        Class method to apply cx gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cx",
            "qubits":[*qubits]
        })
    
    def cy(self, *qubits: int) -> None:
        """
        Class method to apply cy gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cy",
            "qubits":[*qubits]
        })

    def cz(self, *qubits: int) -> None:
        """
        Class method to apply cz gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cz",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits: int) -> None:
        """
        Class method to apply csx gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
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
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits: int) -> None:
        """
        Class method to apply ccy gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccy",
            "qubits":[*qubits]
        })

    def ccz(self, *qubits: int) -> None:
        """
        Class method to apply ccz gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits: int) -> None:
        """
        Class method to apply cswap gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
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
            theta (float): angle.
            phi (float): angle.
            qubit (int): qubit in which the gate is applied.
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
            theta (float): angle.
            phi (float): angle.
            lam (float): angle.
            qubit (int): qubit in which the gate is applied.
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
            theta (float): angle.
            phi (float): angle.
            lam (float): angle.
            qubit (int): qubit in which the gate is applied.
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
            theta (float): angle.
            phi (float): angle.
            qubit (int): qubit in which the gate is applied.
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
            theta (float): angle.
            phi (float): angle.
            lam (float): angle.
            ``*qubits`` (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
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
            theta (float): angle.
            phi (float): angle.
            lam (float): angle.
            gamma (float): angle.
            ``*qubits`` (list[int, int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
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
            matrix (list or <class 'numpy.ndarray'>): unitary operator in matrix form to be applied to the given qubits.

            ``*qubits`` (int): qubits to which the unitary operator will be applied.

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
                "clbits":[c],
                "clreg":[]
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
                "clbits":[self.classical_regs[new_clreg][q]],
                "clreg":[]
            })

    def c_if(self, gate: str, control_qubit: int, target_qubit: int, param: Optional[float] = None, matrix: Optional["list[list[list[complex]]]"] = None) -> None:
        """
        Method for implementing a gate contiioned to a classical measurement. The control qubit provided is measured, if it's 1 the gate provided is applied to the given qubits.

        For parametric gates, only one-parameter gates are supported, therefore only one parameter must be passed.

        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            control (int): control qubit whose classical measurement will control the execution of the gate.

            target (list[int], int): list of qubits or qubit to which the gate is intended to be applied.

            param (float or int): parameter for the case parametric gate is provided.
        """

        self.is_dynamic = True
        
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

            self.measure(list_control_qubit[0], list_control_qubit[0])
            self._add_instruction({
                "name": name,
                "qubits": _flatten([list_target_qubit, list_control_qubit]),
                "registers":_flatten([list_control_qubit]),
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

            self.measure(list_control_qubit[0], list_control_qubit[0])
            self._add_instruction({
                "name": name,
                "qubits": _flatten([list_target_qubit, list_control_qubit]),
                "conditional_reg":_flatten([list_control_qubit]),
                "params":list_param
            })

        else:
            logger.error(f"Gate {gate} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing 

    def reset(self, qubits: Union[list[int], int]):
        """
        To be applied after a measure. It ensure that the qubit returns to the zero state.

        Args:
            qubits (int, list[int]): qubit or list of qubits to reset
        """
        if isinstance(qubits, list):
            for q in qubits:
                self.c_if("x", q, q)

        elif isinstance(qubits, int):
            self.c_if("x", qubits, qubits)

        else:
            logger.error(f"Argument for reset must be list or int, but {type(qubits)} was provided.")

    def save_state(self, pershot: bool = False, label: str = "_method_"):
        self.instructions.append({
            "name": "save_state",
            "qubits": list(range(self.num_qubits)),
            "snapshot_type": "list" if pershot else "single",
            "label": label
        })

    def measure_and_send(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to measure and send a bit from the current circuit to a remote one.
        
        Args:
        -------

            qubit (int): qubit to be measured and sent.

            target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit or circuit to which the result of the measurement is sent.

        """
        self.is_dynamic = True
        self.has_cc = True
        
        # TODO: accept a list of qubits to be measured and sent to one circuit or to a list of them

        if isinstance(qubit, int):
            qubits = [qubit]
        else:
            logger.error(f"control qubit must be int, but {type(qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_circuit, str):
            target_circuit_id = target_circuit

        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit.id
        else:
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        

        self._add_instruction({
            "name": "measure_and_send",
            "qubits": qubits,
            "circuits": [target_circuit_id]
        })


        self.sending_to.append(target_circuit_id)
    
    def qsend(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a qubit from the current circuit to another one.
        
        Args:
        -------

        qubit (int): qubit to be sent.

        target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit or circuit to which the qubit is sent.
        """
        self.is_dynamic = True
        
        if isinstance(qubit, int):
            list_control_qubit = [qubit]
        else:
            logger.error(f"control qubit must be int, but {type(qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_circuit, str):
            target_circuit_id = target_circuit

        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit._id
        else:
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        

        self._add_instruction({
            "name": "qsend",
            "qubits": list_control_qubit,
            "circuits": [target_circuit_id]
        })

    def qrecv(self, qubit: int, control_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a qubit from the current circuit to a remote one.
        
        Args:
        -------
            qubit (int): ancilla to which the received qubit is assigned.

            control_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit from which the qubit is received.
        """
        self.is_dynamic = True
        
        if isinstance(qubit, int):
            qubits = [qubit]
        else:
            logger.error(f"control qubit must be int, but {type(qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(control_circuit, str):
            control_circuit_id = control_circuit

        elif isinstance(control_circuit, CunqaCircuit):
            control_circuit_id = control_circuit._id
        else:
            logger.error(f"control_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(control_circuit)} was provided [TypeError].")
            raise SystemExit
        
        self._add_instruction({
            "name": "qrecv",
            "qubits": qubits,
            "circuits": [control_circuit_id]
        })

    def remote_c_if(self, gate: str, qubits: Union[int, list[int]], param: Optional[float], control_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to apply a distributed instruction as a gate conditioned by a non local classical measurement from a remote circuit and applied locally.
        
        Args:
        -------
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

        target_qubits (int): target qubits from self.

        param (float or int): parameter in case the gate provided is parametric.

        control_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit (or the circuit itself) from which we receive the gate.      
        """

        self.is_dynamic = True
        self.has_cc = True
        
        if isinstance(qubits, int):
            qubits = [qubits]
        elif isinstance(qubits, list):
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(qubits)} was provided [TypeError].")
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

        self._add_instruction({
            "name": "recv",
            "qubits":[],
            "remote_conditional_reg":qubits,
            "circuits": [control_circuit]
        })

        self._add_instruction({
            "name": gate,
            "qubits": qubits,
            "remote_conditional_reg":qubits,
            "params":params,
        })

                
    def qsend(self, control_qubit: Optional[int] = None, target_circuit: Optional[Union[str, 'CunqaCircuit']] = None) -> None:
            """
            Class method to send a qubit from the current circuit to a remote one.
            
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
                "name": "qsend",
                "qubits": _flatten([list_control_qubit]),
                "circuits": [target_circuit_id]
            })

            self.sending_to.append(target_circuit_id)

    def qrecv(self, gate: str, target_qubits: Union[int, "list[int]"], param: float, control_circuit: Optional[Union[str, 'CunqaCircuit']] = None)-> None:
        """
        Class method to apply a distributed instruction as a gate controlled by a non-local qubit (from a remote circuit).
        
        Args:
        -------
        gate (str): gate to be applied. Has to be supported by CunqaCircuit.

        target_qubits (int): target qubits from self.

        param (float or int): parameter in case the gate provided is parametric.

        control_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit (or circuit itself) from which we will receive a qubit.       
        """

        self.is_distributed = True

        if isinstance(gate, str):
            name = "distr_" + gate
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
                "qubits": _flatten([target_qubits]),
                "params":params,
                "circuits": [control_circuit]
            })
        else:
            logger.error(f"Gate {name} is not supported for quantum communications.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing

    def draw(self, include_comm = False):
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




def _flatten(lists: list[list]):
    return [element for sublist in lists for element in sublist]


###################################################################################
######################### INTEGRATION WITH QISKIT BLOCK ###########################
###################################################################################
from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit

def qc_to_json(qc: Union['QuantumCircuit', 'CunqaCircuit', str]) -> Tuple[dict, bool]:
    """
    Transforms a QuantumCircuit to json dict.

    Args:
        qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): circuit to transform to json.

    Return:
        Json dict with the circuit information.
    """
    is_dynamic = False
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
        
        quantum_registers, classical_registers = _registers_dict(qc)
        
        json_data = {
            "id": "",
            "is_parametric": _is_parametric(qc),
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }
        for instruction in qc.data:
            qreg = [r._register.name for r in instruction.qubits]
            qubit = [q._index for q in instruction.qubits]
            
            creg = [r._register.name for r in instruction.clbits]
            bit = [b._index for b in instruction.clbits]

            if instruction.name == "barrier":
                pass
            elif instruction.name == "unitary":

                json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":[[list(map(lambda z: [z.real, z.imag], row)) for row in instruction.params[0].tolist()]] #only difference, it ensures that the matrix appears as a list, and converts a+bj to (a,b)
                                                })
            elif instruction.name != "measure":

                if (instruction.operation._condition != None):
                    is_dynamic = True
                    json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":instruction.params,
                                                "conditional_reg":[instruction.operation._condition[0]._index]
                                                })
                else:
                    json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":instruction.params
                                                })
            else:
                clreg_name = [r._register.name for r in instruction.clbits]
                clreg = []
                if clreg_name[0] != 'meas':
                    clreg = bit

                json_data["instructions"].append({"name":instruction.name,
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "clbits":[classical_registers[k][b] for k,b in zip(clreg_name, bit)],
                                                "clreg":[classical_registers[k][b] for k,b in zip(clreg_name, clreg)]
                                                })
                    

        return json_data, is_dynamic 
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from QuantumCircuit to json dict [{type(error).__name__}].")
        raise error


def from_json_to_qc(circuit_dict: dict) -> 'QuantumCircuit':
    """
    Function to transform a circuit in json dict format to <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.

    Args:
        circuit_dict (dict): circuit to be transformed to QuantumCircuit.

    Return:
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

def _registers_dict(qc: 'QuantumCircuit') -> "list[dict]":
    """
    Extracts the number of classical and quantum registers from a QuantumCircuit.

    Args
        qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): quantum circuit whose number of registers we want to know

    Return:
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
        circuit (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, dict or str): the circuit from which we want to find out if it's parametric.

    Return:
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



########## PURELY MATHEMATICAL METHODS ####################

def transitive_combinations(pairs: set) -> set:
    # Step 1: Create a graph representation of the given pairs
    graph = defaultdict(set)
    for pair in pairs:
        for node in pair:
            graph[node].update(pair)

    # Step 2: Perform a breadth-first search to find all transitive combinations
    transitive_combinations = pairs.copy()

    while True:
        new_combinations = set()
        for combo in transitive_combinations:
            for node in combo:
                for neighbor in graph[node]:
                    if neighbor not in combo:
                        new_combo = frozenset({*combo, neighbor})
                        if new_combo not in transitive_combinations:
                            new_combinations.add(new_combo)
        if not new_combinations:
            break
        transitive_combinations.update(new_combinations)

    return transitive_combinations