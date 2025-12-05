"""
    Holds CUNQA's custom circuit class and functions to translate its instructions into other 
    formats for circuit definition.

    Building circuits
    =================

    Users can define a circuit using :py:class:`~CunqaCircuit` to then send it to the virtual QPUs.
    Nevertheless, for the case in which no communications are needed among the circuits sent,  
    :py:class:`qiskit.QuantumCircuit` [#]_ is also allowed. Be aware that some instructions might 
    not be supported for :py:class:`~CunqaCircuit`, for the list of supported instructions check 
    its documentation. Module py:mod:`cunqa.converters` contains functions to transform between 
    circuit formats.

    Circuits by json ``dict`` format
    ================================

    A low level way of representing a circuit is by a json ``dict`` with specefic fields that gather 
    the information needed by the simulator in order to run the circuit.

    This structe is presented below:

    .. code-block:: python

       {"id":str, # circuit identificator
        "is_parametric":bool, # weather if the circuit has parametric instructions that can be 
        updated
        "is_dynamic":bool, # weather if the circuit has intermediate measurements or conditioned 
        operations
        "instructions":list[dict], # list of instructions of the circuit in dict format
        "num_qubits":int, # number of qubits of the circuit
        "num_clbits":int, # number of classical bits of the circuit
        "quantum_registers":dict, # dict specifying the grouping of the qubits in registers
        "classical_registers":dict # dict specifying the grouping of the classical bits in registers
        }

    On the other hand, instructions have some mandatory and optional keys:

    .. code-block:: python

        {"name":str, # MANDATORY, name of the instruction, has to be accepted by the simulator
         "qubits":list[int], # MANDATORY, qubits on which the instruction acts
         "params":list[int|float] | list[list[[int|float]]], # OPTIONAL, only required for 
         parametric gates and for \'unitary\' instruction.
         "clbits":list[int], # OPTINAL, any classical bits used in the instruction
        }

    For classical and quantum communications among circuits, we do not recomend working at such low 
    level format, users rather describe this operations through the 
    :py:class:`~cunqa.circuit.CunqaCircuit` class. If curious, you can always create the 
    :py:class:`~cunqa.circuit.CunqaCircuit` and obtain its intructions by its attribute 
    :py:attr:`~cunqa.circuit.CunqaCircuit.instructions`, or you can convert it to the json `dict` 
    format by the :py:func:`~cunqa.converters.convert` function.

    References:
    ~~~~~~~~~~~
    .. [#] `qiskit.QuantumCircuit 
    <https://quantum.cloud.ibm.com/docs/es/api/qiskit/qiskit.circuit.QuantumCircuit>`_ 
    documentation.

"""
from __future__ import annotations
import numpy as np
import random
import string

from collections import defaultdict
from typing import Union, Optional

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

class CunqaCircuit():
    # TODO: look for other alternatives for describing the documentation that do not requiere such 
    # long docstrings, maybe gatehring everything in another file and using decorators, as in ther 
    # APIs.
    """
    Class to define a quantum circuit for the :py:mod:`~cunqa` api.

    This class serves as a tool for the user to describe not only simple circuits, but also to 
    describe classical and quantum operations between circuits. On its initialization, it takes as 
    mandatory the number of qubits for the circuit *num_qubits*), also number of classical 
    bits (*num_clbits*) and a personalized id (*id*), which by default would be randomly generated.
    Once the object is created, class methods canbe used to add instructions to the circuit such as 
    single-qubit and two-qubits gates, measurements, conditional operations,... but also operations 
    that allow to send measurement outcomes or qubits to other circuits. This sending operations 
    require that the virtual QPUs to which the circuits are sent support classical or quantum 
    communications with the desired connectivity.
    
    Supported operations
    ----------------------

    **Single-qubit gates:**
    :py:meth:`~CunqaCircuit.id`, :py:meth:`~CunqaCircuit.x`, :py:meth:`~CunqaCircuit.y`, 
    :py:meth:`~CunqaCircuit.z`, :py:meth:`~CunqaCircuit.h`,  :py:meth:`~CunqaCircuit.s`,
    :py:meth:`~CunqaCircuit.sdg`, :py:meth:`~CunqaCircuit.sx`, :py:meth:`~CunqaCircuit.sxdg`, 
    :py:meth:`~CunqaCircuit.t`, :py:meth:`~CunqaCircuit.tdg`, :py:meth:`~CunqaCircuit.u1`,
    :py:meth:`~CunqaCircuit.u2`, :py:meth:`~CunqaCircuit.u3`, :py:meth:`~CunqaCircuit.u`, 
    :py:meth:`~CunqaCircuit.p`, :py:meth:`~CunqaCircuit.r`, :py:meth:`~CunqaCircuit.rx`,
    :py:meth:`~CunqaCircuit.ry`, :py:meth:`~CunqaCircuit.rz`.

    **Two-qubits gates:**
    :py:meth:`~CunqaCircuit.swap`, :py:meth:`~CunqaCircuit.cx`, :py:meth:`~CunqaCircuit.cy`, 
    :py:meth:`~CunqaCircuit.cz`, :py:meth:`~CunqaCircuit.csx`, :py:meth:`~CunqaCircuit.cp`,
    :py:meth:`~CunqaCircuit.cu`, :py:meth:`~CunqaCircuit.cu1`, :py:meth:`~CunqaCircuit.cu3`, 
    :py:meth:`~CunqaCircuit.rxx`, :py:meth:`~CunqaCircuit.ryy`, :py:meth:`~CunqaCircuit.rzz`,
    :py:meth:`~CunqaCircuit.rzx`, :py:meth:`~CunqaCircuit.crx`, :py:meth:`~CunqaCircuit.cry`, 
    :py:meth:`~CunqaCircuit.crz`, :py:meth:`~CunqaCircuit.ecr`,

    **Three-qubits gates:**
    :py:meth:`~CunqaCircuit.ccx`, :py:meth:`~CunqaCircuit.ccy`, :py:meth:`~CunqaCircuit.ccz`, 
    :py:meth:`~CunqaCircuit.cswap`.

    **n-qubits gates:**
    :py:meth:`~CunqaCircuit.unitary`.

    **Non-unitary local operations:**
    :py:meth:`~CunqaCircuit.c_if`, :py:meth:`~CunqaCircuit.measure`, 
    :py:meth:`~CunqaCircuit.measure_all`, :py:meth:`~CunqaCircuit.reset`.

    **Remote operations for classical communications:**
    :py:meth:`~CunqaCircuit.measure_and_send`, :py:meth:`~CunqaCircuit.remote_c_if`.

    **Remote operations for quantum comminications:**
    :py:meth:`~CunqaCircuit.qsend`, :py:meth:`~CunqaCircuit.qrecv`, :py:meth:`~CunqaCircuit.expose`, 
    :py:meth:`~CunqaCircuit.rcontrol`.

    Creating your first CunqaCircuit
    ---------------------------------

    Start by instantiating the class providing the desired number of qubits:

        >>> circuit = CunqaCircuit(2)

    Then, gates can be added through the mentioned methods. Let's add a Hadamard gate and CNOT gates 
    to create a Bell state:

        >>> circuit.h(0) # adding hadamard to qubit 0
        >>> circuit.cx(0,1)

    Finally, qubits are measured by:

        >>> circuit.measure_all()

    Once the circuit is ready, it is ready to be sent to a QPU by the method 
    :py:meth:`~cunqa.qpu.QPU.run`.

    Other methods to manupulate the class are:

    .. list-table::
       :header-rows: 1
       :widths: 20 80

       * - Method
         - 
       * - :py:meth:`~CunqaCircuit.add_instructions`
         - Class method to add operations to the circuit from a list of dict-type instructions.
       * - :py:meth:`~CunqaCircuit.assign_parameters`
         - Plugs values into the intructions of parametric gates marked with a parameter name.
    

    Classical communications among circuits
    ---------------------------------------

    The strong part of :py:class:`CunqaCircuit` is that it allows to define communication 
    directives between circuits. We can define the sending of a classical bit from one circuit to 
    another by:

        >>> circuit_1 = CunqaCircuit(2)
        >>> circuit_2 = CunqaCircuit(2)
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, circuit_2) # qubit 0 is measured and the outcome is sent 
        to circuit_2
        >>> circuit_2.remote_c_if("x", 0, circuit_1) # the outcome is recived to perform a 
        classically controlled operation
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Then, circuits can be sent to QPUs that support classical communications using the 
    :py:meth:`~cunqa.mappers.run_distributed` function.

    Circuits can also be referend to through their *id* string. When a CunqaCircuit is created, by 
    default a random *id* is assigned, but it can also be personalized:

        >>> circuit_1 = CunqaCircuit(2, id = "1")
        >>> circuit_2 = CunqaCircuit(2, id = "2")
        >>> circuit_1.h(0)
        >>> circuit_1.measure_and_send(0, "2") # qubit 0 is measured and the outcome is sent to 
        circuit_2
        >>> circuit_2.remote_c_if("x", 0, "1") # the outcome is recived to perform a classicaly 
        controlled operation
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Teledata protocol
    -----------------
    .. image:: /_static/teledata.png
        :align: center
        :width: 150
        :height: 300px

    The teledata protocol consists on the reconstruction of an unknown quantum state of a given 
    physical system at a different location without actually transmitting the system [#]_. Within 
    :py:mod:`cunqa`, when quantum communications among the virtual QPUs utilized are available, a 
    qubit from one circuit can be sent to another, the teledata protocol is implemented at a lower 
    level so there is no need for the user to implement it. In this scheme, generally an acilla 
    qubit would be neccesary to recieve the quantum state. Let's see an example for the creation of 
    a Bell pair remotely:

        >>> circuit_1 = CunqaCircuit(2, 1, id = "1")
        >>> circuit_2 = CunqaCircuit(2, 2, id = "2")
        >>>
        >>> circuit_1.h(0); circuit_1.cx(0,1)
        >>> circuit_1.qsend(1, "1") # sending qubit 1 to circuit with id "2"
        >>> circuit_1.measure(0,0)
        >>>
        >>> circuit_2.qrecv(0, "2") # reciving qubit from circuit with id "1" and assigning it 
        to qubit 0
        >>> circuit_2.cx(0,1)
        >>> circuit_2.measure_all()

    It is important to note that the qubit used for the communication, the one send, after the 
    operation it is reset, so in a general basis it wouldn't need to be measured. If we want to send 
    more qubits afer, we can use it since it is reset to zero.

    Telegate protocol
    -----------------
    .. image:: /_static/telegate.png
        :align: center
        :width: 150
        :height: 300px

    Quantum gate teleportation, also known as telegate, reduces the topological requirements by 
    substituting two-qubit gates with other cost-effective resources: auxiliary entangled states, 
    local measurements, and single-qubit operations [#]_. This is another feature available in 
    :py:mod:`cunqa` in the quantum communications scheme, managed by the 
    :py:class:`~cunqa.circuit.ControlContext` class. Here is an example analogous to the one 
    presented above:

        >>> circuit_1 = CunqaCircuit(2, id = "1")
        >>> circuit_2 = CunqaCircuit(1, id = "2")
        >>>
        >>> circuit_1.h(0); circuit_1.cx(0,1)
        >>>
        >>> with circuit_1.expose(1, circuit_2) as rcontrol: # exposing qubit at circuit_1
        >>>     circuit_2.cx(rcontol, 1) # applying telegate operation controlled by the exposed 
        qubit
        >>>
        >>> circuit_1.measure_all()
        >>> circuit_2.measure_all()

    Here there is no need for an ancilla since the control is the exposed qubit from the other 
    circuit/virtual QPU.

    .. warning::
        Note that the circuit specification in :py:meth:`CunqaCircuit.expose` cannot be done by 
        passing the circuit :py:attr:`CunqaCircuit.id` since the 
        :py:class:`~cunqa.circuit.ControlContext` object needs the :py:class:`CunqaCircuit` object 
        in order to manage the telegate block.

    References:
    ~~~~~~~~~~~
    [#] `Review of Distributed Quantum Computing. From single QPU to High Performance Quantum 
    # Computing <https://arxiv.org/abs/2404.01265>`_

    """
    
    # global attributes
    _ids: set = set() #: Set with ids in use.
    _communicated: dict[str, CunqaCircuit] = {} #: Dictionary with the circuits that employ communication directives.

    _id: str #: Circuit identificator.
    is_parametric: bool  #: Whether the circuit contains parametric gates.
    has_cc: bool #: Whether the circuit contains classical communications with other circuit.
    has_qc: bool #: Whether the circuit contains quantum communications with other circuit.
    is_dynamic: bool #: Whether the circuit has local non-unitary operations.
    instructions: list[dict] #: Set of operations applied to the circuit.
    quantum_regs: dict  #: Dictionary of quantum registers as ``{"name": [assigned qubits]}``.
    classical_regs: dict #: Dictionary of classical registers of the circuit as ``{"name": [assigned clbits]}``.
    sending_to: list[str] #: List of circuit ids to which the current circuit is sending measurement outcomes or qubits. 
    _telegate: list[dict] #: List with the gates to be put into the telegate protocol.


    def __init__(
            self, 
            num_qubits: int, 
            num_clbits: Optional[int] = None, 
            id: Optional[str] = None
        ):

        """
        Class constructor to create a CunqaCirucit. Only the ``num_qubits`` argument is mandatory, 
        also ``num_clbits`` can be provided if there is intention to incorporate intermediate 
        measurements. If no ``id`` is provided, one is generated randomly, then it can be accessed 
        through the class attribute :py:attr:`~CunqaCircuit._id`.

        Args:
            num_qubits (int): Number of qubits of the circuit.

            num_clbits (int): Numeber of classical bits for the circuit. A classical register is 
            initially added.

            id (str): Label for identifying the circuit. This id is then used for refering to the 
            circuit in classical and quantum communications methods.
        """

        self.is_parametric = False
        self.has_cc = False
        self.has_qc = False
        self.is_dynamic = False
        self.instructions = []
        self.quantum_regs = {}
        self.classical_regs = {}        
        self.sending_to = []
        self.is_parametric = False
        self._telegate = None 

        self.add_q_register("q0", num_qubits)
        
        if num_clbits is not None:
            self.add_cl_register("c0", num_clbits)

        if not id:
            self._id = "CunqaCircuit_" + _generate_id()
        elif id in self._ids:
            self._id = "CunqaCircuit_" + _generate_id()
            logger.warning(f"Id {id} was already used for another circuit, using an automatically "
                           f"generated one: {self._id}.")
        else:
            self._id = id

    @property
    def info(self) -> dict:
        """
        Information about the main class attributes given as a dictinary.
        """
        return {
            "id":self._id, 
            "instructions":self.instructions, 
            "num_qubits": self.num_qubits,
            "num_clbits": self.num_clbits,
            "classical_registers": self.classical_regs,
            "quantum_registers": self.quantum_regs, 
            "has_cc":self.has_cc, 
            "has_qc": self.has_qc, 
            "is_dynamic":self.is_dynamic, 
            "sending_to":self.sending_to, 
            "is_parametric": self.is_parametric
        }

    @property
    def num_qubits(self) -> int:
        """
        Number of qubits of the circuit.
        """
        return sum([sum(qr) for qr in self.quantum_regs.values()])
    
    @property
    def num_clbits(self) -> int:
        """
        Number of classical bits of the circuit.
        """
        return sum([sum(qr) for qr in self.classical_regs.values()])

    def add_instructions(self, instructions: Union[dict, list[dict]]):
        """
        Class method to add an instruction to the CunqaCircuit.

        Each instruction must have as mandatory keys "name" and "qubits", while other keys are 
        accepted: "clbits", "params", "circuits" or "remote_conditional_reg".

        Args:
            instruction (dict | list[dist]): instruction to be added.
        """

        if (isinstance(instr, dict)):
            instructions = [instructions]

        for instr in instructions:
            if self._telegate is not None:
                self._telegate.append(instr)
            else:
                self.instructions.append(instr)
                    
    def add_q_register(self, name: str, num_qubits: int):
        """
        Class method to add a quantum register to the circuit. A quantum register is understood as 
        a group of qubits with a label.

        Args:
            name (str): label for the quantum register.

            num_qubits (int): number of qubits.
        """

        if not num_qubits:
            raise AttributeError("The num_qubits attribute must be strictly positive.")
        
        new_name = name
        if new_name in self.quantum_regs:
            i = 0
            while f"{name}_{i}" in self.quantum_regs:
                i += 1
            new_name = f"{name}_{i}"
            logger.warning(f"{name} for quantum register in use, renaming to {new_name}.")

        self.quantum_regs[new_name] = [(self.num_qubits + i) for i in range(num_qubits)]
        return new_name

    def add_cl_register(self, name, num_clbits):
        """
        Class method to add a classical register to the circuit. A classical register is understood 
        as a group of classical bits with a label.

        Args:
            name (str): label for the quantum register.

            number_clbits (int): number of classical bits.
        """
        if not num_clbits:
            raise AttributeError("The num_qubits attribute must be strictly positive.")

        new_name = name
        if new_name in self.classical_regs:
            i = 0
            while f"{name}_{i}" in self.classical_regs:
                i += 1
            new_name = f"{name}_{i}"
            logger.warning(f"{name} for classical register in use, renaming to {new_name}.")
        
        self.classical_regs[new_name] = [(self.num_clbits + i) for i in range(num_clbits)]
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

    def sy(self, qubit: int) -> None:
        """
        Class method to apply sy gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sy",
            "qubits":[qubit]
        })
    
    def sydg(self, qubit: int) -> None:
        """
        Class method to apply sydg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sydg",
            "qubits":[qubit]
        })

    def sz(self, qubit: int) -> None:
        """
        Class method to apply sz gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sz",
            "qubits":[qubit]
        })
    
    def szdg(self, qubit: int) -> None:
        """
        Class method to apply szdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"szdg",
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

    def P0(self, qubit: int) -> None:
        """
        Class method to apply P0 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"p0",
            "qubits":[qubit]
        })

    def P1(self, qubit: int) -> None:
        """
        Class method to apply P1 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"p1",
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
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
            second one target qubit.
        """
        self._add_instruction({
            "name":"cx",
            "qubits":[*qubits]
        })
    
    def cy(self, *qubits: int) -> None:
        """
        Class method to apply cy gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
            second one target qubit.
        """
        self._add_instruction({
            "name":"cy",
            "qubits":[*qubits]
        })

    def cz(self, *qubits: int) -> None:
        """
        Class method to apply cz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
            second one target qubit.
        """
        self._add_instruction({
            "name":"cz",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits: int) -> None:
        """
        Class method to apply csx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
            second one target qubit.
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
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
            the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits: int) -> None:
        """
        Class method to apply ccy gate to the given qubits. Gate is decomposed asfollows as it is 
        not commonly supported by simulators.
        q_0: ──────────────■─────────────
                           │             
        q_1: ──────────────■─────────────
             ┌──────────┐┌─┴─┐┌─────────┐
        q_2: ┤ Rz(-π/2) ├┤ X ├┤ Rz(π/2) ├
             └──────────┘└───┘└─────────┘

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
            the following one will be target qubit.
        """
        self._add_instruction({
            "name":"rz",
            "qubits":[qubits[-1]],
            "params":[-np.pi/2]
        })
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })
        self._add_instruction({
            "name":"rz",
            "qubits":[qubits[-1]],
            "params":[np.pi/2]
        })

    def ccz(self, *qubits: int) -> None:
        """
        Class method to apply ccz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
            the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cecr(self, *qubits: int) -> None:
        """
        Class method to apply cecr gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cecr",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits: int) -> None:
        """
        Class method to apply cswap gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
            the following one will be target qubit.
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
            param (float | int | str): parameter for the parametric gate. String identifies a 
            variable parameter (needs to be assigned) with the string label.

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

    def u(
            self, 
            theta:  Union[float, int, str], 
            phi:  Union[float, int, str], 
            lam:  Union[float, int, str], 
            qubit: int
        ) -> None:
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

    def u3(
            self, 
            theta:  Union[float, int, str], 
            phi:  Union[float, int, str], 
            lam:  Union[float, int, str], 
            qubit: int
        ) -> None:
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

    def p(self, param:  Union[float, int, str], qubit: int) -> None:
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

    def r(self, theta:  Union[float, int, str], phi:  Union[float, int, str], qubit: int) -> None:
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

    def RotInvX(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply RotInvX gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"rotinvx",
            "qubits":[qubit],
            "params":[param]
        })

    def rRotInvY(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rRotInvY gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"rotinvy",
            "qubits":[qubit],
            "params":[param]
        })
    
    def RotInvZ(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply RotInvZ gate to the given qubit.

        Args:
            param (float | int): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"rotinvz",
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

    def cr(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cr gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cr",
            "qubits":[*qubits],
            "params":[param]
        })

    def crx(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply crx gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
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
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
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
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
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
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
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
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu1",
            "qubits":[*qubits],
            "params":[param]
        })

    def cu2(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cu2 gate to the given qubits.

        Args:
            param (float | int): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu2",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def cu3(
            self, 
            theta: Union[float, int, str], 
            phi: Union[float, int, str], 
            lam: Union[float,int, str], 
            *qubits: int
        ) -> None: # three parameters
        """
        Class method to apply cu3 gate to the given qubits.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
            and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu3",
            "qubits":[*qubits],
            "params":[theta,phi,lam]
        })
    
    def cu(
            self, 
            theta: Union[float,int, str], 
            phi: Union[float,int, str], 
            lam: Union[float,int, str], 
            gamma: Union[float,int, str], 
            *qubits: int
        ) -> None: # four parameters
        """
        Class method to apply cu gate to the given qubits.

        Args:
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
            gamma (float | int): angle.
            qubits (int | list[int]): qubits in which the gate is applied, first one will be the 
            control qubit and second one the target qubit.
        """
        self._add_instruction({
            "name":"cu",
            "qubits":[*qubits],
            "params":[theta, phi, lam, gamma]
        })
    
    # methods for implementing conditional LOCAL gates
    def unitary(self, matrix: list[list[list[complex]]], *qubits: int) -> None:
        """
        Class method to apply a unitary gate created from an unitary matrix provided.

        Args:
            matrix (list | numpy.ndarray): unitary operator in matrix form to be applied to the 
            given qubits.

            qubits (int): qubits to which the unitary operator will be applied.

        """
        if (isinstance(matrix, np.ndarray) and 
            (matrix.shape[0] == matrix.shape[1]) and 
            (matrix.shape[0]%2 == 0)):
            matrix = list(matrix)
        elif (isinstance(matrix, list) and 
              isinstance(matrix[0], list) and 
              all([len(matrix) == len(m) for m in matrix]) and 
              (len(matrix)%2 == 0)):
            matrix = matrix
        else:
            logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                         f"(2^n,2^n) [TypeError].")
            raise SystemExit # User's level
        
        matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]


        self._add_instruction({
            "name":"unitary",
            "qubits":[*qubits],
            "params":[matrix]
        })

    def multicontrol(
            self, 
            base_gate: str, 
            num_ctrl_qubits: int, 
            qubits: list[int], 
            params: list[float, int] = []
        ):
        """
        Class method to apply a multicontrolled gate to the given qubits.

        Args:
            base_gate (str): name of the gate to convert to multicontrolled.
            num_ctrl_qubits ( int): number of qubits that control the gate.
            qubits (list[int]): qubits in which the gate is applied, first num_ctrl_qubits will be 
            the control qubits and the remaining the target qubits.
            params (list[float | int | Parameter]): list of parameters for the gate.
            
        .. warning:: This instructions is currently only supported for Aer simulator.
        """
        mgate_name = "mc" + base_gate

        self._add_instruction({
            "name": mgate_name,
            "num_ctrl_qubits": num_ctrl_qubits,
            "qubits": qubits,
            "num_qubits": len(qubits),
            "params": params
        })
        
    def measure(self, qubits: Union[int, list[int]], clbits: Union[int, list[int]]) -> None:
        """
        Class method to add a measurement of a qubit or a list of qubits and to register that 
        measurement in the given classical bits.

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
        Class to apply a global measurement of all of the qubits of the circuit. An additional 
        classcial register will be added and labeled as "measure".
        """
        new_clreg = "measure"

        new_clreg = self.add_cl_register(new_clreg, self.num_qubits)

        for q in range(self.num_qubits):

            self._add_instruction({
                "name":"measure",
                "qubits":[q],
                "clbits":[self.classical_regs[new_clreg][q]],
                "clreg":[]
            })

    def c_if(
            self, 
            gate: str, 
            control_qubit: int, 
            target_qubit: int, 
            param: Optional[float] = None, 
            matrix: Optional[list[list[list[complex]]]] = None
        ) -> None:
        """
        Method for implementing a gate conditioned to a classical measurement. The control qubit 
        provided is measured, if it's 1 the gate provided is applied to the given qubits.

        For parametric gates, only one-parameter gates are supported, therefore only one parameter 
        must be passed.

        The gates supported by the method are the following: h, x, y, z, rx, ry, rz, cx, cy, cz, 
        unitary.

        To implement the conditioned uniraty gate, the corresponding matrix should be passed by the 
        `matrix` argument.

        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            control_qubit (int): control qubit whose classical measurement will control the 
            execution of the gate.

            target_qubit (int | list[int]): list of qubits or qubit to which the gate is intended 
            to be applied.

            param (float | int): parameter for the case parametric gate is provided.

            matrix (list | numpy.ndarray): unitary operator in matrix form to be applied to the 
            given qubits.
        """

        self.is_dynamic = True
        
        if isinstance(gate, str):
            name = "c_if_" + gate
        else:
            logger.error(f"gate specification must be str, but {type(gate)} "
                         f"was provided [TypeError].")
            raise SystemExit
        
        if isinstance(control_qubit, int):
            list_control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} "
                         f"was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubit, int):
            list_target_qubit = [target_qubit]
        elif isinstance(target_qubit, list):
            list_target_qubit = target_qubit
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(target_qubit)} "
                         f"was provided [TypeError].")
            raise SystemExit
        
        if (gate == "unitary") and (matrix is not None):

            if (isinstance(matrix, np.ndarray) and 
                (matrix.shape[0] == matrix.shape[1]) and 
                (matrix.shape[0]%2 == 0)):
                matrix = list(matrix)
            elif (isinstance(matrix, list) and 
                  isinstance(matrix[0], list) and 
                  all([len(m) == len(matrix) for m in matrix]) and 
                  (len(matrix)%2 == 0)):
                matrix = matrix
            else:
                logger.error(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                             f"(2^n,2^n), n=1,2 [TypeError].")
                raise SystemExit # User's level

            matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]

            self.measure(list_control_qubit[0], list_control_qubit[0])
            self._add_instruction({
                "name": name,
                "qubits": _flatten([list_target_qubit]),
                "conditional_reg":_flatten([list_control_qubit]),
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

        # TODO: move this checkpoints to _check_instruction method
        if gate in SUPPORTED_GATES_PARAMETRIC_1:
            if param is None:
                logger.error(f"Since a parametric gate was provided ({gate}) a parameter should be "
                             f"passed [ValueError].")
                raise SystemExit
            elif isinstance(param, float) or isinstance(param, int):
                list_param = [param]
            else:
                logger.error(f"param must be int or float, but {type(param)} was provided "
                             f"[TypeError].")
                raise SystemExit
        else:
            if param is not None:
                logger.warning("A parameter was provided but gate is not parametric, therefore it "
                               "will be ignored.")
            list_param = []

        if name in SUPPORTED_GATES_CONDITIONAL:

            self.measure(list_control_qubit[0], list_control_qubit[0])
            self._add_instruction({
                "name": gate,
                "qubits": _flatten([list_target_qubit]),
                "conditional_reg": list_control_qubit,
                "params":list_param
            })

        else:
            logger.error(f"Gate {gate} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing 

    # TODO: check if simulators accept reset instruction as native
    def reset(self, qubits: Union[int, list]):
        """
        Class method to add reset to zero instruction to a qubit or list of qubits 
        (use after measure).

        Args:
            qubits (int): qubits to which the reset operation is applied.
        
        """

        if isinstance(qubits, list):
            for q in qubits:
                self.instructions.append({
                    'name': 'c_if_x', 
                    'qubits': [q, q], 
                    'conditional_reg': [q], 
                    'params': []
                })

        elif isinstance(qubits, int):
            self.instructions.append({
                'name': 'c_if_x', 
                'qubits': [qubits, qubits], 
                'conditional_reg': [qubits], 
                'params': []
            })

        else:
            logger.error(f"Argument for reset must be list or int, but {type(qubits)} was provided.")

    def measure_and_send(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to measure and send a bit from the current circuit to a remote one.
        
        Args:

            qubit (int): qubit to be measured and sent.

            target_circuit (str | CunqaCircuit): id of the circuit or circuit to which the result 
            of the measurement is sent.

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
            target_circuit_id = target_circuit._id
        else:
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but "
                         f"{type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        

        self._add_instruction({
            "name": "measure_and_send",
            "qubits": qubits,
            "circuits": [target_circuit_id]
        })


        self.sending_to.append(target_circuit_id)

    def remote_c_if(
            self, 
            gate: str, 
            qubits: Union[int, list[int]], 
            control_circuit: Union[str, CunqaCircuit], 
            param: Optional[float] = None, 
            matrix: Optional[list[list[list[complex]]]] = None, 
            num_ctrl_qubits: int = None
        ) -> None:
        """
        Class method to apply a distributed instruction as a gate condioned by a non local classical 
        measurement from a remote circuit and applied locally.

        The gates supported by the method are the following: h, x, y, z, rx, ry, rz, cx, cy, cz, 
        unitary.

        To implement the conditioned uniraty gate, the corresponding matrix should be passed by the 
        `param` argument.
        
        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            target_qubits (int | list[int]): qubit or qubits to which the gate is conditionally 
            applied.

            param (float | int): parameter in case the gate provided is parametric.

            control_circuit (str | CunqaCircuit): id of the circuit or circuit from which the 
            condition is sent.

        """

        self.is_dynamic = True
        self.has_cc = True
        
        if isinstance(qubits, int):
            qubits = [qubits]
        elif isinstance(qubits, list):
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(qubits)} was provided "
                         f"[TypeError].")
            raise SystemExit
        
        if param is not None:
            params = [param]
        else:
            params = []

        if control_circuit is None:
            logger.error("control_circuit not provided.")
            raise SystemExit
        
        elif isinstance(control_circuit, str):
            control_circuit = control_circuit

        elif isinstance(control_circuit, CunqaCircuit):
            control_circuit = control_circuit._id
        else:
            logger.error(f"control_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, "
                         f"but {type(control_circuit)} was provided [TypeError].")
            raise SystemExit
        
        if (gate == "unitary") and (matrix is not None):

            if (isinstance(matrix, np.ndarray) and 
                (matrix.shape[0] == matrix.shape[1]) and 
                (matrix.shape[0]%2 == 0)):
                matrix = list(matrix)
            elif (isinstance(matrix, list) and 
                  isinstance(matrix[0], list) and 
                  all([len(m) == len(matrix) for m in matrix]) and 
                  (len(matrix)%2 == 0)):
                matrix = matrix
            else:
                logger.error("matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                             "(2^n,2^n), n=1,2 [TypeError].")
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

        # Create the instruction's dictionary first incase we need to add the multicontrol key
        instr_dict = {
            "name": gate,
            "qubits": qubits,
            "remote_conditional_reg":qubits,
            "params":params,
        }
        if num_ctrl_qubits is not None:
            instr_dict["num_ctrl_qubits"] = num_ctrl_qubits

        self._add_instruction(instr_dict)

    def qsend(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a qubit from the current circuit to another one.
        
        Args:
            qubit (int): qubit to be sent.

            target_circuit (str | CunqaCircuit): id of the circuit or circuit to which the qubit is 
            sent.
        """
        self.has_qc = True
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
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but "
                         f"{type(target_circuit)} was provided [TypeError].")
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
            qubit (int): ancilla to which the received qubit is assigned.

            control_circuit (str | CunqaCircuit): id of the circuit from which the qubit is received.
        """
        self.has_qc = True
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
            logger.error(f"control_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, "
                         f"but {type(control_circuit)} was provided [TypeError].")
            raise SystemExit
        
        self._add_instruction({
            "name": "qrecv",
            "qubits": qubits,
            "circuits": [control_circuit_id]
        })

    def expose(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> 'ControlContext':
        """
        Class method to expose a qubit from the current circuit to another one for a telegate 
        operation. The exposed qubit will be used at the target circuit as the control qubit in 
        controlled operations.
        
        Args:
            qubit (int): qubit to be exposed.
            target_circuit (str | CunqaCircuit): id of the circuit or circuit where the exposed 
            qubit is used.
        
        Returns:
            A :py:class:`ControlContext` object to manage remotly controlled operations in the given 
            circuit.


        .. warning::
            In the current version, :py:meth:`~CunqaCircuit.expose` instruction is only supported 
            for MQT DDSIM (Munich) simulator. If circuits with such instructions are sent to other 
            simulators, an error will occur at the virtual QPU.
        """

        self.has_qc = True
        self.is_dynamic = True
        
        list_control_qubit = [qubit]
        
        if isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit._id
        else:
            logger.error(f"target_circuit must be <class 'cunqa.circuit.CunqaCircuit'>, but "
                         f"{type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        

        self._add_instruction({
            "name": "expose",
            "qubits": list_control_qubit,
            "circuits": [target_circuit_id]
        })
        return ControlContext(self,target_circuit)


class ControlContext:
    """
    Class to manage the controlled telegate operations from a circuit/virtual QPU to another.

    An object of this class is returned by the :py:meth:`~cunqa.circuit.CunqaCircuit.expose` method.
    Used as a context manager, it can be passed to controlled operations in order to implement 
    telgate operations:

        >>> with circuit_1.expose(0, circuit_2) as rcontrol:
        >>>     circuit_2.cx(rcontrol, 0)
    
    Then, when the block ends, the :py:class:`ControlContext` adds the propper "rcontrol" 
    instruction to the target circuit.
    """
    def __init__(self, control_circuit: 'CunqaCircuit', target_circuit: 'CunqaCircuit') -> int:
        """Class constructor.
        
            Args:
                control_circuit (~cunqa.circuit.CunqaCircuit): circuit which qubit is exposed.
            
                target_circuit (~cunqa.circuit.CunqaCircuit): circuit in which the instructions are 
                implemented.
        """
        if isinstance(control_circuit, CunqaCircuit) and isinstance(target_circuit, CunqaCircuit):
            self.control_circuit = control_circuit
            self.target_circuit = target_circuit
            
            control_id = control_circuit.info['id']
            print(f"ID del control circuit:{control_id}")
            target_id = target_circuit.info['id']
            print(f"ID del target circuit:{target_id}")
        else:
            logger.error(f"control_circuit and target_circuit must be <class 'cunqa.circuit.CunqaCircuit'>, "
                         f"but {type(num_qubits)} was provided [TypeError].")
            raise SystemExit

    def __enter__(self):
        self.target_circuit._telegate = []
        return -1

    def __exit__(self, exc_type, exc_val, exc_tb):
        for instruction in self.target_circuit._telegate:
            if instruction["name"] in ["qsend", "qrecv", "expose", "recv"]:
                logger.error("Remote operations, quantum or classical, are not allowed within a "
                             "telegate block.")
                raise SystemExit
        instr = {
            "name": "rcontrol",
            "qubits": [],  
            "instructions": self.target_circuit._telegate,
            "circuits": [self.control_circuit._id]
        }
        self.target_circuit.instructions.append(instr)
        self.target_circuit._telegate = None

        return False



#################################################################################
#################### PURELY UTILITARIAN/MATHEMATICAL METHODS ####################
#################################################################################

def _transitive_combinations(pairs: set) -> set:
    """
    Method used to find deep connectivity between circuits from first order connections. Used on 
    CunqaCircuit's metaclass.
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
