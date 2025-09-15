"""
    Holds Cunqa's custom circuit class and functions to translate its instructions into other formats for circuit definition.

    Building circuits
    =================

    Users can define a circuit using :py:class:`~CunqaCircuit` to then send it to the virtual QPUs. Nevertheless, for the case in which no communications are needed among the circuits sent, other formats are allowed.

    This module also provides global functions that translate form :py:class:`qiskit.QuantumCircuit` [#]_ to a instructions json (:py:meth:`~qc_to_json`) and the other way around (:py:meth:`~from_json_to_qc`).

    For example, if a user wants to transform a :py:class:`qiskit.QuantumCircuit` into a :py:class:`~CunqaCircuit`, one can obtain the instructions and then add them to the :py:class:`~CunqaCircuit` object:

    >>> qc = QuantumCircuit(4)
    >>> ...
    >>> circuit_json = qc_to_json(qc)
    >>> instruction_set = circuit_json["instructions"]
    >>> num_qubits = circuit_json["num_qubits"]
    >>> cunqacirc = CunqaCircuit(num_qubits)
    >>> cunqacirc.from_instructions(instruction_set)

    Be aware that some instructions might not be supported for :py:class:`~CunqaCircuit`, for the list of supported instructions check its documentation.

    References:
    ~~~~~~~~~~~
    .. [#] `qiskit.QuantumCircuit <https://quantum.cloud.ibm.com/docs/es/api/qiskit/qiskit.circuit.QuantumCircuit>`_ documentation.

"""

import numpy as np
import random
import string
from collections import defaultdict
from typing import Union, Optional, Tuple

from cunqa.logger import logger

def _generate_id(size: int = 4) -> str:
    """Returns a random alphanumeric identifier.

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
    """Exception for error during circuit desing at :py:class:`~cunqa.circuit.CunqaCircuit`."""
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
    Class to define a quantum circuit for the :py:mod:`~cunqa` api.

    This class serves as a tool for the user to describe not only simple circuits, but also to describe classical and quantum operations between circuits.
    On its initialization, it takes as mandatory the number of qubits for the circuit *num_qubits*), also number of classical bits (*num_clbits*) and a personalized id (*id*), which by default would be randomly generated.
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

    Once the circuit is ready, it is ready to be sent to a QPU by the method :py:meth:`~cunqa.qpu.QPU.run`.

    Other methods to manupulate the class are:

    .. list-table::
       :header-rows: 1
       :widths: 20 80

       * - Method
         - 
       * - :py:meth:`~CunqaCircuit.from_instructions`
         - Class method to add operations to the circuit from a list of dict-type instructions.
    

    Classical communications among circuits
    ---------------------------------------

    The strong part of CunqaCircuit is that it allows to define communication directives between circuits.
    We can define the sending of a classical bit from one circuit to another by:

        >>> circuit_1 = CunqaCircuit(2)
        >>> circuit_2 = CunqaCircuit(2)
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, circuit_2) # qubit 0 is measured and the outcome is sent to circuit_2
        >>> circuit_2.remote_c_if("x", 0, circuit_1) # the outcome is recived to perform a classicaly controlled operation
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Then, circuits can be sent to QPUs that support classical communications using the :py:meth:`cunqa.mappers.run_distributed` function.

    Circuits can also be referend to through their *id* string. When a CunqaCircuit is created, by default a random *id* is assigned, but it can also be personalized:

        >>> circuit_1 = CunqaCircuit(2, id = "1")
        >>> circuit_2 = CunqaCircuit(2, id = "2")
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, "2") # qubit 0 is measured and the outcome is sent to circuit_2
        >>> circuit_2.remote_c_if("x", 0, "1") # the outcome is recived to perform a classicaly controlled operation
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
    """
    
    _id: str #: Circuit identificator.
    is_parametric: bool  #: Weather the circuit contains parametric gates.
    has_cc: bool #: Weather the circuit contains classical communications with other circuit.
    is_dynamic: bool #: Weather the circuit has local non-unitary operations.
    instructions: "list[dict]" #: Set of operations applied to the circuit.
    quantum_regs: dict  #: Dictionary of quantum registers as ``{"name": [assigned qubits]}``.
    classical_regs: dict #: Dictionary of classical registers of the circuit as ``{"name": [assigned clbits]}``.
    sending_to: "list[str]" #: List of circuit ids to which the current circuit is sending measurement outcomes or qubits. 
    current_params: "list[Union[int, float]]" #: List of the parameters that the circuit currently has
    param_labels: "list[str]" #: List of labels assigned to parametric gates to be able to update them separately and conveniently. Same lenght as current_params


    def __init__(self, num_qubits: int, num_clbits: Optional[int] = None, id: Optional[str] = None):

        """
        Class constructor to create a CunqaCirucit. Only the ``num_qubits`` argument is mandatory, also ``num_clbits`` can be provided if there is intention to incorporate intermediate measurements.
        If no ``id`` is provided, one is generated randomly, then it can be accessed through the class attribute :py:attr:`~CunqaCircuit._id`.

        Args:
            num_qubits (int): Number of qubits of the circuit.

            num_clbits (int): Numeber of classical bits for the circuit. A classical register is initially added.

            id (str): Label for identifying the circuit. This id is then used for refering to the circuit in classical and quantum communications methods.
        """

        self.is_parametric = False
        self.has_cc = False
        self.is_dynamic = False
        self.instructions = []
        self.quantum_regs = {'q0':[q for q in range(num_qubits)]}
        self.classical_regs = {}
        self.sending_to = []

        self.param_labels = []
        self.current_params = []

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
        info = {"id":self._id, "instructions":self.instructions, "num_qubits": self.num_qubits,"num_clbits": self.num_clbits,"classical_registers": self.classical_regs,"quantum_registers": self.quantum_regs, "has_cc":self.has_cc, "is_dynamic":self.is_dynamic, "sending_to":self.sending_to, "is_parametric": self.is_parametric}
        if self.is_parametric:
            info += {"param_labels": self.param_labels,"current_params": self.current_params}

        return info

    @property
    def num_qubits(self) -> int:
        """
        Number of qubits of the circuit.
        """
        return len(_flatten([[q for q in qr] for qr in self.quantum_regs.values()]))
    
    @property
    def num_clbits(self) -> int:
        """
        Number of classical bits of the circuit.
        """
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


    def from_instructions(self, instructions: list[dict]):
        """
        Class method to add operations to the circuit from a list of dict-type instructions.
        
        Each instruction must have as mandatory keys ``"name"`` and ``"qubits"``, while other keys are accepted: ``"clbits"``, ``"params"``, ``"circuits"`` or ``"remote_conditional_reg"``.

        Args:
            instructions (list[dict]): list gathering all the each instruction as a dict.
        """
        for instruction in instructions:
            self._add_instruction(instruction)
        return self


    def _add_instruction(self, instruction):
        """
        Class method to add an instruction to the CunqaCircuit.

        Each instruction must have as mandatory keys "name" and "qubits", while other keys are accepted: "clbits", "params", "circuits" or "remote_conditional_reg".

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

        Each instruction must have as mandatory keys `` "name":str `` and ``"qubits":`[int]``, while other keys are accepted: ``"clbits":[int]``, ``"params":[int or float]``, ``"circuits":[str]`` or ``"remote_conditional_reg":[int]``.

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
                    
                    if not all([(isinstance(p,float) or isinstance(p,int) or isinstance(p, str)) for p in instruction["params"]]):
                        logger.error(f"Instruction params must be int, float or str (for labels), but {type(instruction['params'])} was provided.")
                        raise TypeError
                    
                    self.current_params += instruction["params"]
                    self.param_labels += [p if isinstance(p, str) else "no_name" for p in instruction["params"]]

                    if not len(instruction["params"]) == gate_params:
                        logger.error(f"instruction number of params ({gate_params}) is not consistent with params provided ({len(instruction['params'])}).")
                        raise ValueError
                elif (not ("params" in instruction)) and (instruction["name"] in _flatten([SUPPORTED_GATES_PARAMETRIC_1, SUPPORTED_GATES_PARAMETRIC_2, SUPPORTED_GATES_PARAMETRIC_3, SUPPORTED_GATES_PARAMETRIC_4])):
                    logger.error("instruction is parametric, therefore requires params.")
                    raise ValueError
                    
    def _add_q_register(self, name, number_qubits):
        """
        Class method to add a quantum register to the circuit. A quantum register is understood as a group of qubits with a label.

        Args:
            name (str): label for the quantum register.

            number_qubits (int): number of qubits.
        """

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
        """
        Class method to add a classical register to the circuit. A classical register is understood as a group of classical bits with a label.

        Args:
            name (str): label for the quantum register.

            number_clbits (int): number of classical bits.
        """

        if name in self.classical_regs:
            i = 0
            new_name = name
            while new_name in self.classical_regs:
                new_name = new_name + "_" + str(i); i += 1

            logger.warning(f"{name} for classical register in use, renaming to {new_name}.")
        
        else:
            new_name = name

        self.classical_regs[new_name] = [(self.num_clbits + i) for i in range(number_clbits)]

        return new_name

    
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
            qubits (list[int]): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"swap",
            "qubits":[*qubits]
        })

    def ecr(self, *qubits: int) -> None:
        """
        Class method to apply ecr gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"ecr",
            "qubits":[*qubits]
        })

    def cx(self, *qubits: int) -> None:
        """
        Class method to apply cx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cx",
            "qubits":[*qubits]
        })
    
    def cy(self, *qubits: int) -> None:
        """
        Class method to apply cy gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cy",
            "qubits":[*qubits]
        })

    def cz(self, *qubits: int) -> None:
        """
        Class method to apply cz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cz",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits: int) -> None:
        """
        Class method to apply csx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
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
            qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits: int) -> None:
        """
        Class method to apply ccy gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccy",
            "qubits":[*qubits]
        })

    def ccz(self, *qubits: int) -> None:
        """
        Class method to apply ccz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits: int) -> None:
        """
        Class method to apply cswap gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"cswap",
            "qubits":[*qubits]
        })

    
    # methods for parametric single-qubit gates

    def u1(self, param: Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply u1 gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate. String identifies a variable parameter (needs to be assigned) with the string label.

             qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"u1",
            "qubits":[qubit],
            "params":[param]
        })
    
    def u2(self, theta:  Union[float,int, str], phi:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply u2 gate to the given qubit.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"u2",
            "qubits":[qubit],
            "params":[theta,phi]
        })

    def u(self, theta:  Union[float,int, str], phi:  Union[float,int, str], lam:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply u gate to the given qubit.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"u",
            "qubits":[qubit],
            "params":[theta,phi,lam]
        })

    def u3(self, theta:  Union[float,int, str], phi:  Union[float,int, str], lam:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply u3 gate to the given qubit.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"u3",
            "qubits":[qubit],
            "params":[theta,phi,lam]
        })

    def p(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply p gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"p",
            "qubits":[qubit],
            "params":[param]
        })

    def r(self, theta:  Union[float,int, str], phi:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply r gate to the given qubit.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"r",
            "qubits":[qubit],
            "params":[theta, phi]
        })

    def rx(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rx gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"rx",
            "qubits":[qubit],
            "params":[param]
        })

    def ry(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply ry gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"ry",
            "qubits":[qubit],
            "params":[param]
        })
    
    def rz(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rz gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"rz",
            "qubits":[qubit],
            "params":[param]
        })

    # methods for parametric two-qubit gates

    def rxx(self, param: Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rxx gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"rxx",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def ryy(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply ryy gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"ryy",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzz(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rzz gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"rzz",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzx(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rzx gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"rzx",
            "qubits":[*qubits],
            "params":[param]
        })

    def crx(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply crx gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"crx",
            "qubits":[*qubits],
            "params":[param]
        })

    def cry(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cry gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cry",
            "qubits":[*qubits],
            "params":[param]
        })

    def crz(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply crz gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"crz",
            "qubits":[*qubits],
            "params":[param]
        })

    def cp(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cp gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cp",
            "qubits":[*qubits],
            "params":[param]
        })

    def cu1(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cu1 gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu1",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def cu3(self, theta:  Union[float,int, str], phi:  Union[float,int, str], lam:  Union[float,int, str], *qubits: int) -> None: # three parameters
        """
        Class method to apply cu3 gate to the given qubits.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu3",
            "qubits":[*qubits],
            "params":[theta,phi,lam]
        })
    
    def cu(self, theta:  Union[float,int, str], phi:  Union[float,int, str], lam:  Union[float,int, str], gamma:  Union[float,int, str], *qubits: int) -> None: # four parameters
        """
        Class method to apply cu gate to the given qubits.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            gamma (float | int): angle.
            qubits (int | list[int]): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
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
            matrix (list | numpy.ndarray): unitary operator in matrix form to be applied to the given qubits.

            qubits (int): qubits to which the unitary operator will be applied.

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
            qubits (int | list[int]): qubits to measure.

            clbits (int | list[int]): clasical bits where the measurement will be registered.
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

        The gates supported by the method are the following: h, x, y, z, rx, ry, rz, cx, cy, cz, unitary.

        To implement the conditioned uniraty gate, the corresponding matrix should be passed by the `matrix` argument.

        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            control_qubit (int): control qubit whose classical measurement will control the execution of the gate.

            target_qubit (int | list[int]): list of qubits or qubit to which the gate is intended to be applied.

            param (float | int): parameter for the case parametric gate is provided.

            matrix (list | numpy.ndarray): unitary operator in matrix form to be applied to the given qubits.

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
                logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape (2^n,2^n), n=1,2 [TypeError].")
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

    # TODO: check if simulators accept reset instruction as native
    def reset(self, *qubits: int):
        """
        Class method to add reset to zero instruction to a qubit or list of qubits (use after measure).

        Args:
            qubits (int): qubits to which the reset operation is applied.
        
        """

        if isinstance(qubits, list):
            for q in qubits:
                self.instructions.append({'name': 'c_if_x', 'qubits': [q, q], 'conditional_reg': [q], 'params': []})

        elif isinstance(qubits, int):
            self.instructions.append({'name': 'c_if_x', 'qubits': [qubits, qubits], 'conditional_reg': [qubits], 'params': []})

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

            qubit (int): qubit to be measured and sent.

            target_circuit (str | CunqaCircuit): id of the circuit or circuit to which the result of the measurement is sent.

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
    

    def remote_c_if(self, gate: str, qubits: Union[int, "list[int]"], control_circuit: Union[str, 'CunqaCircuit'], param: Optional[float] = None, matrix: Optional["list[list[list[complex]]]"] = None) -> None:
        """
        Class method to apply a distributed instruction as a gate condioned by a non local classical measurement from a remote circuit and applied locally.

        The gates supported by the method are the following: h, x, y, z, rx, ry, rz, cx, cy, cz, unitary.

        To implement the conditioned uniraty gate, the corresponding matrix should be passed by the `param` argument.
        
        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            target_qubits (int | list[int]): qubit or qubits to which the gate is conditionally applied.

            param (float | int): parameter in case the gate provided is parametric.

            control_circuit (str | CunqaCircuit): id of the circuit or circuit from which the condition is sent.

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
        
        if (gate == "unitary") and (matrix is not None):

            if isinstance(matrix, np.ndarray) and (matrix.shape[0] == matrix.shape[1]) and (matrix.shape[0]%2 == 0):
                matrix = list(matrix)
            elif isinstance(matrix, list) and isinstance(matrix[0], list) and all([len(m) == len(matrix) for m in matrix]) and (len(matrix)%2 == 0):
                matrix = matrix
            else:
                logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape (2^n,2^n), n=1,2 [TypeError].")
                raise SystemExit # User's level

            params = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]
            return

        elif (gate == "unitary") and (matrix is None):
            logger.error(f"For unitary gate a matrix must be provided [ValueError].")
            raise SystemExit # User's level
        
        elif (gate != "unitary") and (matrix is not None):
            logger.error(f"instruction {gate} does not suppor matrix.")
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

    def assign_parameters(self, **marked_params):
        """
        Plugs values into the intructions of parametric gates marked with a parameter name.

        Args:
            marked_params (dict[list | float | int]): dict with keys the labels of the variable parameters 
            and associated values the new parameters to assign to them. The values are entered with the syntax theta_1 = 3.14, theta_2 = [2, 7], theta_3 = 9
        """
        try:
            param_index = 0
            for instruction in self.instructions:
                if (("params" in instruction) and (not instruction["name"] in {"unitary", "c_if_unitary", "remote_c_if_unitary"}) and (len(instruction["params"]) != 0)):
                    for i in range(len(instruction["params"])):
                        param = self.param_labels[param_index + i]

                        if param in marked_params:
                            if isinstance(marked_params[param], (int, float)):
                                instruction["params"][i] = marked_params[param]
                                self.current_params[param_index + i] = marked_params[param] # update the saved current values, too

                            elif isinstance(marked_params[param], list):
                                next_value = marked_params[param].pop(0)
                                if not isinstance(next_value, (int, float)):
                                    logger.error(f"Parameters inside the list must be int or float but {type(next_value)} was given.")
                                    raise SystemExit
                                
                                instruction["params"][i] = next_value
                                self.current_params[param_index + i] = next_value # update the saved current values, too

                            else:
                                logger.error(f"Parameters must be list[int, float], int or float but {type(marked_params[param])} was given.")
                                raise SystemExit
                        
                    param_index += len(instruction["params"])
                                
        except Exception as error:
            logger.error(f"Error while assigning parameters, try checking that the provided params are of the correct lenght. \n {error}")
            raise SystemExit
        
        if not all([len(value)==0 for value in marked_params.values() if isinstance(value, list)]):
            logger.warning(f"Some of the given parameters list were not exhausted, check name or lenght of at least the following keys: {[key for key, value in marked_params.items() if (isinstance(value, list) and len(value)!=0)]}. \n Use circuit.param_info to obtain the names and numbers of the variable parameters.")



                
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





#################################################################################
#################### PURELY UTILITARIAN/MATHEMATICAL METHODS ####################
#################################################################################

def _flatten(lists: list[list]):
    """
    Takes the elements of the lists supplied and creates a single list gathering all of them.
    """
    return [element for sublist in lists for element in sublist]
   

def transitive_combinations(pairs: set) -> set:
    """
    Method used to find deep connectivity between circuits from first order connections. Used on CunqaCircuit's metaclass.
    """
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