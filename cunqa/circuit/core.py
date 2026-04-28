"""Core implementation of CUNQA's custom quantum circuit abstraction."""

from __future__ import annotations

import numpy as np
import copy
from typing import Union, Optional
from sympy.core.sympify import sympify, SympifyError

from cunqa.utils import generate_id
from cunqa.circuit.parameter import Param

from cunqa.logger import logger

class CunqaCircuit:
    """
    Quantum circuit abstraction for the CUNQA API. 
    This class allows the design of quantum circuits to be executed in the vQPUs. Upon 
    initialization, the circuit is defined by its number of qubits and, optionally, its number of
    classical bits and a user-defined identifier. If no identifier is provided, a unique one is
    generated automatically. The circuit identifier is later used to reference the circuit in
    communication-related instructions.

    Once created, instructions can be appended to the circuit using the provided methods,
    including single- and multi-qubit gates, measurements, classically controlled operations,
    and remote communication primitives.

    .. list-table:: Supported operations
        :header-rows: 1
        :widths: 20 30 50
        :class: wrap-table

        * - Group
          - Category
          - Operations

        * - Unitary operations
          - Single-qubit gates with no parameters
          - :py:meth:`id`, :py:meth:`x`, :py:meth:`y`, :py:meth:`z`, :py:meth:`h`, :py:meth:`s`, :py:meth:`sdg`, :py:meth:`sx`, :py:meth:`sxdg`, :py:meth:`sy`, :py:meth:`sydg`, :py:meth:`sz`, :py:meth:`szdg`, :py:meth:`t`, :py:meth:`tdg`, :py:meth:`p0`, :py:meth:`p1`, :py:meth:`v`, :py:meth:`vdg`, :py:meth:`k`, :py:meth:`hz2` 

        * -
          - Single-qubit gates with one parameter
          - :py:meth:`u1`, :py:meth:`p`, :py:meth:`rx`, :py:meth:`ry`, :py:meth:`rz`, :py:meth:`rotinvx`, :py:meth:`rotinvy`, :py:meth:`rotinvz` 
            
        * - 
          - Single-qubit gates with two parameters
          - :py:meth:`u2`, :py:meth:`r`
          
        * - 
          - Single-qubit gates with three parameters
          - :py:meth:`u3`
          
        * - 
          - Single-qubit gates with four parameters  
          - :py:meth:`u` 
            
        * - 
          - Two-qubit gates with no parameters
          - :py:meth:`id2`, :py:meth:`swap`, :py:meth:`iswap`, :py:meth:`fusedswap`, :py:meth:`cx`, :py:meth:`cy`, :py:meth:`cz`, :py:meth:`ch`, :py:meth:`csx`, :py:meth:`csxdg`, :py:meth:`csy`, :py:meth:`csz`, :py:meth:`cs`, :py:meth:`csdg`, :py:meth:`ecr`, :py:meth:`ct`, :py:meth:`dcx`  

        * -
          - Two-qubit gates with one parameter 
          - :py:meth:`cu1`, :py:meth:`cp`, :py:meth:`crx`, :py:meth:`cry`, :py:meth:`crz`, :py:meth:`rxx`, :py:meth:`ryy`, :py:meth:`rzz`, :py:meth:`rxy`, :py:meth:`rzx`

        * -
          - Two-qubit gates with two parameters
          - :py:meth:`cu2`, :py:meth:`cr`, :py:meth:`xxmyy`, :py:meth:`xxpyy`, :py:meth:`fs`

        * -
          - Two-qubit gates with three parameters
          - :py:meth:`cu3`

        * -
          - Two-qubit gates with four parameters
          - :py:meth:`cu`

        * - 
          - Three-qubit gates with no parameters
          - :py:meth:`ccx`, :py:meth:`ccy`, :py:meth:`ccz`, :py:meth:`cswap`, :py:meth:`cecr`

        * - 
          - Multicontrol gates with no parameters
          - :py:meth:`mcx`, :py:meth:`mcy`, :py:meth:`mcz`, :py:meth:`mcsx`

        * - 
          - Multicontrol gates with one parameter
          - :py:meth:`mcp`, :py:meth:`mcrx`, :py:meth:`mcry`, :py:meth:`mcrz`

        * - 
          - Multicontrol gates with four parameters
          - :py:meth:`mcu`

        * - 
          - Special gates
          - :py:meth:`unitary`, :py:meth:`randomunitary`, :py:meth:`diagonal`, :py:meth:`multiplexer`, :py:meth:`multipauli`, :py:meth:`multipaulirotation`, :py:meth:`sparsematrix`, :py:meth:`amplitudedampingnoise`, :py:meth:`bitflipnoise`, :py:meth:`dephasingnoise`, :py:meth:`depolarizingnoise`, :py:meth:`independentxznoise`, :py:meth:`twoqubitdepolarizingnoise`

        * - Local non-unitary operations
          - Local non-unitary operations
          - :py:meth:`cif`, :py:meth:`measure`, :py:meth:`measure_all`, :py:meth:`reset`

        * - Remote operations
          - Classical communication
          - :py:meth:`send`, :py:meth:`recv`

        * - 
          - Quantum communication
          - :py:meth:`qsend`, :py:meth:`qrecv`, :py:meth:`expose`


    Attributes
    ==========
    .. autoattribute:: id
        :annotation: : str
    .. autoattribute:: info
        :annotation: : dict
    .. autoattribute:: num_qubits
        :annotation: : int
    .. autoattribute:: num_clbits
        :annotation: : int
    .. autoattribute:: instructions
    .. autoattribute:: is_dynamic
    .. autoattribute:: quantum_regs
    .. autoattribute:: classical_regs
    .. autoattribute:: sending_to

    Operations
    ==========

    Classical communication directives
    ----------------------------------
    .. dropdown:: Classical communication directives
        :animate: fade-in-slide-down

        .. automethod:: send
        .. automethod:: recv


    Quantum communication directives
    --------------------------------
    .. dropdown:: Quantum communication directives
        :animate: fade-in-slide-down

        .. automethod:: qsend
        .. automethod:: qrecv
        .. automethod:: expose


    Non-unitary operations
    ----------------------
    .. dropdown:: Non-unitary operations
        :animate: fade-in-slide-down

        .. automethod:: save_state
        .. automethod:: measure_all
        .. automethod:: measure
        .. automethod:: reset
        .. automethod:: cif

    Unitary operations
    ------------------

    .. dropdown:: Single-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: i
        .. automethod:: x
        .. automethod:: y
        .. automethod:: z
        .. automethod:: h
        .. automethod:: s
        .. automethod:: sdg
        .. automethod:: sx
        .. automethod:: sxdg
        .. automethod:: sy
        .. automethod:: sydg
        .. automethod:: sz
        .. automethod:: szdg
        .. automethod:: t
        .. automethod:: tdg
        .. automethod:: p0
        .. automethod:: p1
        .. automethod:: v
        .. automethod:: vdg
        .. automethod:: k
        .. automethod:: hz2
        .. automethod:: u1
        .. automethod:: u2
        .. automethod:: u3
        .. automethod:: u
        .. automethod:: p
        .. automethod:: r
        .. automethod:: rx
        .. automethod:: ry
        .. automethod:: rz
        .. automethod:: raxis
        .. automethod:: rotinvx
        .. automethod:: rotinvy
        .. automethod:: rotinvz

    .. dropdown:: Two-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: id2
        .. automethod:: swap
        .. automethod:: iswap
        .. automethod:: sqrtswap
        .. automethod:: fusedswap
        .. automethod:: ecr
        .. automethod:: cx
        .. automethod:: cy
        .. automethod:: cz
        .. automethod:: ch
        .. automethod:: csx
        .. automethod:: csxdg
        .. automethod:: cs
        .. automethod:: csdg
        .. automethod:: ct
        .. automethod:: dcx
        .. automethod:: rxx
        .. automethod:: ryy
        .. automethod:: rzz
        .. automethod:: rxy
        .. automethod:: rzx
        .. automethod:: cr
        .. automethod:: crx
        .. automethod:: cry
        .. automethod:: crz
        .. automethod:: cp
        .. automethod:: cu1
        .. automethod:: cu2
        .. automethod:: cu3
        .. automethod:: cu
        .. automethod:: xxmyy
        .. automethod:: xxpyy
        .. automethod:: fs
    
    .. dropdown:: Three-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: ccx
        .. automethod:: ccy
        .. automethod:: ccz
        .. automethod:: cecr
        .. automethod:: cswap
        .. automethod:: csqrtswap

    .. dropdown:: Multi-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: mx
        .. automethod:: cmx

    .. dropdown:: Multicontrol gates
        :animate: fade-in-slide-down

        .. automethod:: mcx
        .. automethod:: mcy
        .. automethod:: mcz
        .. automethod:: mch
        .. automethod:: mcsx
        .. automethod:: mcs
        .. automethod:: mcp
        .. automethod:: mct
        .. automethod:: mcmx
        .. automethod:: mcswap
        .. automethod:: mcsqrtswap
        .. automethod:: mcrx
        .. automethod:: mcry
        .. automethod:: mcrz
        .. automethod:: mcraxis
        .. automethod:: mcu1
        .. automethod:: mcu2
        .. automethod:: mcu3
        .. automethod:: mcu
        .. automethod:: mcpaulistr
        .. automethod:: mcpauligadget
        .. automethod:: mcphasegadget

    .. dropdown:: Special gates
        :animate: fade-in-slide-down

        .. automethod:: unitary
        .. automethod:: randomunitary
        .. automethod:: diagonal
        .. automethod:: phasegadget
        .. automethod:: cphasegadget
        .. automethod:: paulistr
        .. automethod:: cpaulistr
        .. automethod:: pauligadget
        .. automethod:: nonunitarypauligadget
        .. automethod:: cpauligadget
        .. automethod:: multipauli
        .. automethod:: multipaulirotation
        .. automethod:: amplitudedampingnoise
        .. automethod:: bitflipnoise
        .. automethod:: dephasingnoise
        .. automethod:: depolarizingnoise
        .. automethod:: independentxznoise
        .. automethod:: twoqubitdepolarizingnoise


        
    """    
    # global attributes
    _ids: set = set() #: Set with ids in use.
    _communicated: dict[str, CunqaCircuit] = {} #: Dictionary with the circuits that employ communication directives.

    _id: str #: Circuit identifier.
    is_dynamic: bool #: Whether the circuit has local non-unitary operations.
    has_cc: bool #: Whether the circuit has classical communications.
    has_qc: bool #: Whether the circuit has quantum communications.
    instructions: list[dict] #: Set of operations applied to the circuit.
    quantum_regs: dict  #: Dictionary of quantum registers as ``{"name": [assigned qubits]}``.
    classical_regs: dict #: Dictionary of classical registers of the circuit as ``{"name": [assigned clbits]}``.
    sending_to: set[str] #: Set of circuit ids to which the current circuit is sending measurement outcomes or qubits. 
    params: list[Param] #: Ordered list of the parameters names that the circuit currently has.
    
    def __init__(
            self, 
            num_qubits: int, 
            num_clbits: Optional[int] = None, 
            id: Optional[str] = None
        ):
        self.is_dynamic = False
        self.has_cc = False
        self.has_qc = False
        self.instructions = []
        self.params = []
        self.quantum_regs = {}
        self.classical_regs = {}        
        self.sending_to = set()
        self.blocks_with_comms = []

        self.add_q_register("q0", num_qubits)
        
        if num_clbits is not None and num_clbits != 0:
            self.add_cl_register("c0", num_clbits)

        if not id:
            self._id = "CunqaCircuit_" + generate_id()
        elif id in self._ids:
            self._id = "CunqaCircuit_" + generate_id()
            logger.warning(f"Id {id} was already used for another circuit, using an automatically "
                           f"generated one: {self._id}.")
        else:
            self._id = id

    @property
    def id(self) -> str:
        """Returns circuit id."""
        return self._id

    @property
    def info(self) -> dict:
        """
        Information of the instance attributes, given in a dictionary.
        """
        return {
            "id": self._id,
            "instructions": self.instructions,
            "num_qubits": self.num_qubits,
            "num_clbits": self.num_clbits,
            "classical_registers": self.classical_regs,
            "quantum_registers": self.quantum_regs,
            "is_dynamic": self.is_dynamic, 
            "has_cc": self.has_cc, 
            "has_qc": self.has_qc, 
            "sending_to": list(self.sending_to),
            "params": self.params,
            "blocks_with_comms": self.blocks_with_comms
        }

    @property
    def num_qubits(self) -> int:
        """
        Number of qubits of the circuit.
        """
        return sum([len(qr) for qr in self.quantum_regs.values()])
    
    @property
    def num_clbits(self) -> int:
        """
        Number of classical bits of the circuit.
        """
        return sum([len(qr) for qr in self.classical_regs.values()])

    def add_instructions(self, instructions: Union[dict, list[dict]]):
        """
        Class method to add one or multiple instructions to the CunqaCircuit. 

        Args:
            instructions (dict | list[dist]): instruction(s) to be added.
        """
        def handle_params(instruction):
            if "params" in instruction and len(instruction["params"]) != 0:
                if any([isinstance(p, Param) for p in instruction["params"]]):
                    for p in instruction["params"]:
                        new_params = []
                        if isinstance(p, Param):
                            # Copy needed for circuit transformations to avoid aliasing
                            new_param = copy.deepcopy(p);  new_params.append(new_param)
                            self.params.append(new_param)

                    new_instr = copy.deepcopy(instruction)
                    new_instr["params"] = new_params

                    return new_instr

                
                # Converting the string to a symbolic expression
                try:
                    exprs = sympify(instruction["params"])
                except SympifyError:
                    raise ValueError(f"Expression {instruction['params']} cannot be converted to "
                                    f"symbolic expression.")
                
                # Adding to the instruction the Param object or a real number depending on the specified
                # (if real, the parameter will not be changed)
                new_list = []
                for expr, param in zip(exprs, instruction["params"]):
                    if not expr.is_real:
                        new_param = Param(expr)
                        self.params.append(new_param)
                        new_list.append(new_param)
                    else:
                        new_list.append(param)
                instruction["params"] = new_list
            
            return instruction

        if isinstance(instructions, dict):
            new_instr = handle_params(instructions)
            self.instructions.append(new_instr)
        else:
            for instr in instructions:
                new_instr = handle_params(instr)
                self.instructions.append(new_instr)
                    
    def add_q_register(self, name: str, num_qubits: int):
        """
        Class method to add a quantum register to the circuit. A quantum register is understood as 
        a group of qubits with a label.

        Args:
            name (str): label for the quantum register.
            num_qubits (int): number of qubits.
        """

        if num_qubits < 1:
            raise ValueError("The num_qubits attribute must be strictly positive.")
        
        new_name = name
        if new_name in self.quantum_regs:
            i = 0
            while f"{name}_{i}" in self.quantum_regs:
                i += 1
            new_name = f"{name}_{i}"
            logger.warning(f"{name} for quantum register in use, renaming to {new_name}.")

        self.quantum_regs[new_name] = [(self.num_qubits + i) for i in range(num_qubits)]
        return new_name

    def add_cl_register(self, name: str, num_clbits: int):
        """
        Class method to add a classical register to the circuit. A classical register is understood 
        as a group of classical bits with a label.

        Args:
            name (str): label for the quantum register.
            number_clbits (int): number of classical bits.
        """
        if num_clbits < 1:
            raise ValueError("The num_qubits attribute must be strictly positive.")

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
    
    # ----------------------------------
    # Classical communication directives
    # ----------------------------------
    
    def send(self, clbits: Union[int, list[int]], recving_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a bit (previously measured from a qubit) from the current circuit to a 
        remote one. 
        
        Args:

            clbits (int): bits to be sent.

            recving_circuit (str | CunqaCircuit): id of the circuit or circuit object to which the 
                                                bit is sent.

        """
        self.is_dynamic = True; self.has_cc = True
        
        if isinstance(clbits, int):
            clbits = [clbits]
        
        if isinstance(recving_circuit, str):
            recving_circuit_id = recving_circuit
        elif isinstance(recving_circuit, CunqaCircuit):
            recving_circuit_id = recving_circuit.id

        self.add_instructions({
            "name": "send",
            "clbits": clbits,
            "circuits": [recving_circuit_id]
        })

        self.sending_to.add(recving_circuit_id)

    def recv(self, clbits: Union[int, list[int]], sending_circuit: Union[str, CunqaCircuit]) -> None:
        """
        Class method to receive a bit (previously measured from a qubit) from a remote circuit into 
        a classical register of the receiving circuit.
        
        Args:
            clbits (int | list[int]): indexes of the cl registers where the bits will be stored.

            sending_circuit (str | CunqaCircuit): id of the circuit or circuit object from which the 
                                                  bit is sent.

        """
        self.is_dynamic = True; self.has_cc = True

        if isinstance(clbits, int):
            clbits = [clbits]

        if isinstance(sending_circuit, str):
            sending_circuit_id = sending_circuit
        elif isinstance(sending_circuit, CunqaCircuit):
            sending_circuit_id = sending_circuit.id

        self.add_instructions({
            "name": "recv",
            "clbits": clbits,
            "circuits": [sending_circuit_id]
        })
        
    # --------------------------------
    # Quantum communication directives
    # --------------------------------
    
    def qsend(self, qubit: int, recving_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a qubit from the current circuit to another one.
        
        Args:
            qubit (int): qubit to be sent.

            recving_circuit (str | CunqaCircuit): id of the circuit or circuit to which the qubit is 
                                                 sent.
        """
        self.is_dynamic = True; self.has_qc = True
        
        if isinstance(recving_circuit, str):
            recving_circuit_id = recving_circuit
        elif isinstance(recving_circuit, CunqaCircuit):
            recving_circuit_id = recving_circuit.id
        
        self.add_instructions({
            "name": "qsend",
            "qubits": [qubit],
            "circuits": [recving_circuit_id]
        })

    def qrecv(self, qubit: int, control_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to receive a qubit from a remote circuit into an ancilla qubit.
        
        Args:
            qubit (int): ancilla to which the received qubit is assigned.

            control_circuit (str | CunqaCircuit): id of the circuit from which the qubit is received.
        """
        self.is_dynamic = True; self.has_qc = True
        
        if isinstance(control_circuit, str):
            control_circuit_id = control_circuit
        elif isinstance(control_circuit, CunqaCircuit):
            control_circuit_id = control_circuit.id
        
        self.add_instructions({
            "name": "qrecv",
            "qubits": [qubit],
            "circuits": [control_circuit_id]
        })

    def expose(self, qubits: Union[list[int], int], target_circuit: Union[str, 'CunqaCircuit']) -> 'QuantumControlContext':
        """
        Class method to expose a qubit from the current circuit to another one for a telegate 
        operation. The exposed qubit will be used at the target circuit as the control qubit in 
        controlled operations.
        
        Args:
            qubit (int | list): qubit to be exposed.
            target_circuit (str | CunqaCircuit): id of the circuit or circuit object where the exposed qubit is used.
        
        Returns:
            A :py:class:`QuantumControlContext` object to manage remotly controlled operations in 
            the given circuit.

        Usage example:

        .. code-block:: python

            with origin_circ.expose(0, target_circuit) as ([rqubit], subcircuit):
                subcircuit.cx(rqubit, 1)
            
        """ 
        self.is_dynamic = True; self.has_qc = True
        
        if isinstance(qubits, int):
            qubits = [qubits]
        
        if isinstance(target_circuit, str):
            target_circuit_id = target_circuit
        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit.id
        
        self.add_instructions({
            "name": "expose",
            "qubits": qubits,
            "circuits": [target_circuit_id]
        })
        return QuantumControlContext(self, target_circuit, len(qubits))

    # ----------------------
    # Non-unitary operations
    # ----------------------

    def save_state(self, pershot: bool = False, label: str = "_method_") -> None:
        """
        Instruction to save the state of the circuit simulation at the particular moment the 
        instruction is executed.

        Args:
            pershot (bool): determines wether the state is stored separatedly for each shot or 
                            averaged. Default: False
            label (str): key for the state in the result dict. Used to distinguish two states saved. 
                         Default: '_method_', which appears in the result as the name of the 
                         simulation method selected.
        """
        self.instructions.append({
            "name": "save_state",
            "qubits": list(range(self.num_qubits)),
            "snapshot_type": "list" if pershot else "single",
            "label": label
        })

    def measure_all(self) -> None:
        """
        Class to apply a global measurement of all of the qubits of the circuit. An additional 
        classcial register will be added and labeled as "measure".
        """
        new_clreg = self.add_cl_register("measure", self.num_qubits)

        for q in range(self.num_qubits):
            self.add_instructions({
                "name":"measure",
                "qubits":[q],
                "clbits":[self.classical_regs[new_clreg][q]],
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
            self.add_instructions({
                "name":"measure",
                "qubits":[q],
                "clbits":[c]
            })
            
    def reset(self, qubit: Union[int, list[int]]):
        """
        Class method to add reset to zero instruction to a qubit or list of qubits 
        (use after measure).

        Args:
            qubit (int, list[int]]): qubits to which the reset operation is applied.
        
        """

        self.instructions.append({
            'name': 'reset', 
            'qubits': [qubit]
        })
    
    def cif(
            self, 
            clbits: Union[int, list[int]],
            condition: int = 1,
            operation: str = "and"
        ) -> ClassicalControlContext:
        """
        Method for implementing a gate conditioned to a classical measurement. The control qubit 
        provided is measured, if it's 1 the gate provided is applied to the given qubits. In order
        to do this,  ``cif`` context manager is introduced, which enables a more expressive and 
        readable way to define classically controlled blocks:

        .. code-block:: python

            c = CunqaCircuit(2, 2)
            c.h(0)
            c.measure(0, 0)

            with c.cif(0) as cgates:
                cgates.x(1)

        In this example, the operations defined inside the ``cif`` block are executed only if the 
        value of classical bit 0 is equal to 1. Currently, this construct does not support an 
        explicit *else* branch. This design decision is based on the observation that none of the 
        reviewed algorithms or protocols require such functionality. Support for an *else* branch 
        may be added in future versions if needed.

        Args:
            clbits (int | list[int]): clbits to match the condition.
            condition (int): can be 1 or 0. The clbits will have to match this condition 
                             for the gate to be applied. Default = 1
            operation (str): can be "and", "or" or "xor". If multiple clbits are provided, their 
                             measures are operated on with the selected operation and the result is
                             matched to condition. "and" means all need to match the condition, 
                             "or" means any of them should match and "xor" means there should be an
                             odd number of mathching measures. 
        """
        self.is_dynamic = True
        operation = (operation + "n") if (condition == 0) else operation
        return ClassicalControlContext(self, clbits, operation, condition)
    
    # ------------------
    # Unitary operations
    # ------------------
    
    # Non parametric single-qubit gates
    
    def i(self, qubit: int) -> None:
        """
        Class method to apply id gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"id",
            "qubits":[qubit]
        })
    
    def x(self, qubit: int) -> None:
        """
        Class method to apply x gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"x",
            "qubits":[qubit]
        })
    
    def y(self, qubit: int) -> None:
        """
        Class method to apply y gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"y",
            "qubits":[qubit]
        })

    def z(self, qubit: int) -> None:
        """
        Class method to apply z gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"z",
            "qubits":[qubit]
        })
    
    def h(self, qubit: int) -> None:
        """
        Class method to apply h gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"h",
            "qubits":[qubit]
        })

    def s(self, qubit: int) -> None:
        """
        Class method to apply s gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"s",
            "qubits":[qubit]
        })

    def sdg(self, qubit: int) -> None:
        """
        Class method to apply sdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sdg",
            "qubits":[qubit]
        })

    def sx(self, qubit: int) -> None:
        """
        Class method to apply sx gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sx",
            "qubits":[qubit]
        })
    
    def sxdg(self, qubit: int) -> None:
        """
        Class method to apply sxdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sxdg",
            "qubits":[qubit]
        })

    def sy(self, qubit: int) -> None:
        """
        Class method to apply sy gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sy",
            "qubits":[qubit]
        })
    
    def sydg(self, qubit: int) -> None:
        """
        Class method to apply sydg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sydg",
            "qubits":[qubit]
        })

    def sz(self, qubit: int) -> None:
        """
        Class method to apply sz gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"sz",
            "qubits":[qubit]
        })
    
    def szdg(self, qubit: int) -> None:
        """
        Class method to apply szdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"szdg",
            "qubits":[qubit]
        })
    
    def t(self, qubit: int) -> None:
        """
        Class method to apply t gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"t",
            "qubits":[qubit]
        })
    
    def tdg(self, qubit: int) -> None:
        """
        Class method to apply tdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"tdg",
            "qubits":[qubit]
        })

    def p0(self, qubit: int) -> None:
        """
        Class method to apply P0 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"p0",
            "qubits":[qubit]
        })

    def p1(self, qubit: int) -> None:
        """
        Class method to apply P1 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"p1",
            "qubits":[qubit]
        })

    def v(self, qubit: int) -> None:
        """
        Class method to apply v gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"v",
            "qubits":[qubit]
        })

    def vdg(self, qubit: int) -> None:
        """
        Class method to apply vdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"vdg",
            "qubits":[qubit]
        })

    def k(self, qubit: int) -> None:
        """
        Class method to apply k gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"k",
            "qubits":[qubit]
        })

    def hz2(self, qubit: int) -> None:
        """
        Class method to apply hz2 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"hz2",
            "qubits":[qubit]
        })

    # Parametric single-qubit gates

    def u1(self, param: Union[float, int, str], qubit: int) -> None:
        """
        Class method to apply u1 gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        
        self.add_instructions({
            "name":"u1",
            "qubits":[qubit],
            "params":[param]
        })
    
    def u2(self, theta:  Union[float, int, str], phi:  Union[float, int, str], qubit: int) -> None:
        """
        Class method to apply u2 gate to the given qubit.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"u2",
            "qubits": [qubit],
            "params": [theta,phi]
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
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"u3",
            "qubits":[qubit],
            "params":[theta,phi,lam]
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
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"u",
            "qubits":[qubit],
            "params":[theta,phi,lam]
        })

    def p(self, param:  Union[float,int,str], qubit: int) -> None:
        """
        Class method to apply p gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"p",
            "qubits":[qubit],
            "params":[param]
        })

    def r(self, theta:  Union[float,int,str], phi:  Union[float,int,str], qubit: int) -> None:
        """
        Class method to apply r gate to the given qubit.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"r",
            "qubits":[qubit],
            "params":[theta, phi]
        })

    def rx(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rx gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rx",
            "qubits":[qubit],
            "params":[param]
        })

    def ry(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply ry gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"ry",
            "qubits":[qubit],
            "params":[param]
        })
    
    def rz(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rz gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rz",
            "qubits":[qubit],
            "params":[param]
        })

    def raxis(self, param:  Union[float,int, str], axis: list[float], qubit: int) -> None:
        """
        Class method to apply a rotation in the specified axis to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            axis (list[float]): three element list with the values of X, Y and Z.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"raxis",
            "qubits":[qubit],
            "params":[param],
            "axis": axis
        })

    def rotinvx(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rotinvx gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvx",
            "qubits":[qubit],
            "params":[param]
        })

    def rotinvy(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rotinvy gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvy",
            "qubits":[qubit],
            "params":[param]
        })
    
    def rotinvz(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rotinvz gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvz",
            "qubits":[qubit],
            "params":[param]
        })

    # Non parametric two-qubit gates

    def id2(self, *qubits: int) -> None:
        """
        Class method to apply id2 gate to the given qubits.

        Args:
            qubits (list[int]): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"id2",
            "qubits":[*qubits]
        })

    def swap(self, *qubits: int) -> None:
        """
        Class method to apply swap gate to the given qubits.

        Args:
            qubits (list[int]): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"swap",
            "qubits":[*qubits]
        })

    def iswap(self, *qubits: int) -> None:
        """
        Class method to apply iswap gate to the given qubits.

        Args:
            qubits (list[int]): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"iswap",
            "qubits":[*qubits]
        })

    def sqrtswap(self, *qubits: int) -> None:
        """
        Class method to apply sqrtswap gate to the given qubits.

        Args:
            qubits (list[int]): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"sqrtswap",
            "qubits":[*qubits]
        })

    def fusedswap(self, block_size: int, *qubits: int) -> None:
        """
        Class method to apply fusedswap gate to the given qubits.

        Args:
            qubits (list[int]): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"fusedswap",
            "qubits":[*qubits],
            "block_size":[block_size]
        })

    def ecr(self, *qubits: int) -> None:
        """
        Class method to apply ecr gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
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
        self.add_instructions({
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
        self.add_instructions({
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
        self.add_instructions({
            "name":"cz",
            "qubits":[*qubits]
        })

    def ch(self, *qubits: int) -> None:
        """
        Class method to apply ch gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"ch",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits: int) -> None:
        """
        Class method to apply csx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"csx",
            "qubits":[*qubits]
        })

    def csxdg(self, *qubits: int) -> None:
        """
        Class method to apply csxdg gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"csxdg",
            "qubits":[*qubits]
        })

    def cs(self, *qubits: int) -> None:
        """
        Class method to apply cs gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"cs",
            "qubits":[*qubits]
        })

    def csdg(self, *qubits: int) -> None:
        """
        Class method to apply csdg gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"csdg",
            "qubits":[*qubits]
        })

    def ct(self, *qubits: int) -> None:
        """
        Class method to apply ct gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"ct",
            "qubits":[*qubits]
        })

    def dcx(self, *qubits: int) -> None:
        """
        Class method to apply dcx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"dcx",
            "qubits":[*qubits]
        })

    # Parametric two-qubit gates

    def rxx(self, param: Union[float,int,str], *qubits: int) -> None:
        """
        Class method to apply rxx gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"rxx",
            "qubits":[*qubits],
            "params":[param]
        })
    
    def ryy(self, param:  Union[float,int,str], *qubits: int) -> None:
        """
        Class method to apply ryy gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"ryy",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzz(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rzz gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"rzz",
            "qubits":[*qubits],
            "params":[param]
        })

    def rxy(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rxy gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"rxy",
            "qubits":[*qubits],
            "params":[param]
        })

    def rzx(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply rzx gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"rzx",
            "qubits":[*qubits],
            "params":[param]
        })

    def crx(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply crx gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"crx",
            "qubits":[*qubits],
            "params":[param]
        })

    def cry(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cry gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cry",
            "qubits":[*qubits],
            "params":[param]
        })

    def crz(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply crz gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"crz",
            "qubits":[*qubits],
            "params":[param]
        })

    def cp(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cp gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cp",
            "qubits":[*qubits],
            "params":[param]
        })

    def cu1(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cu1 gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cu1",
            "qubits":[*qubits],
            "params":[param]
        })

    def cu2(self, theta:  Union[float, int, str], 
                    phi:  Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply cu2 gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cu2",
            "qubits":[*qubits],
            "params":[theta, phi]
        })

    def cu3(self, theta:  Union[float, int, str], 
                    phi:  Union[float, int, str], 
                    lam:  Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply cu3 gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cu3",
            "qubits":[*qubits],
            "params":[theta, phi, lam]
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
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            gamma (float | int): angle.
            qubits (int | list[int]): qubits in which the gate is applied, first one will be the 
                                      control qubit and second one the target qubit.
        """
        self.add_instructions({
            "name":"cu",
            "qubits":[*qubits],
            "params":[theta, phi, lam, gamma]
        })

    def xxmyy(self, theta:  Union[float, int, str], 
                    phi:  Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply XX - YY gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"xxmyy",
            "qubits": [*qubits],
            "params": [theta, phi]
        })

    def xxpyy(self, theta:  Union[float, int, str], 
                    phi:  Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply XX + YY gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"xxpyy",
            "qubits": [*qubits],
            "params": [theta, phi]
        })
    
    def fs(self, theta:  Union[float, int, str], 
                    phi:  Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply FS gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"fs",
            "qubits": [*qubits],
            "params": [theta, phi]
        })
    # Non parametric three-qubit gates

    def ccx(self, *qubits: int) -> None:
        """
        Class method to apply ccx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"ccx",
            "qubits": [*qubits]
        })

    def ccz(self, *qubits: int) -> None:
        """
        Class method to apply ccz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits: int) -> None:
        """
        Class method to apply cswap gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"cswap",
            "qubits":[*qubits]
        })

    def csqrtswap(self, *qubits: int) -> None:
        """
        Class method to apply csqrtswap gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"csqrtswap",
            "qubits":[*qubits]
        })

    # Multiqubit gates, no param

    def mx(self, *qubits: int) -> None:
        """
        Class method to apply mx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name":"mx",
            "qubits":[*qubits]
        })

    def cmx(self, *qubits: int) -> None:
        """
        Class method to apply mcx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control and 
                          the following will be target qubits.
        """
        self.add_instructions({
            "name":"cmx",
            "qubits":[*qubits]
        })
    
    # Non parametric multicontrol gates

    def mcx(self, *qubits: int) -> None:
        """
        Class method to apply mcx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcx",
            "qubits":[*qubits]
        })
    
    def mcy(self, *qubits: int) -> None:
        """
        Class method to apply mcy gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcy",
            "qubits":[*qubits]
        })
        
    def mcz(self, *qubits: int) -> None:
        """
        Class method to apply mcz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcz",
            "qubits":[*qubits]
        })

    def mch(self, *qubits: int) -> None:
        """
        Class method to apply mch gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mch",
            "qubits":[*qubits]
        })
    
    def mcsx(self, *qubits: int) -> None:
        """
        Class method to apply mcsx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcsx",
            "qubits":[*qubits]
        })

    def mcs(self, *qubits: int) -> None:
        """
        Class method to apply mcs gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcs",
            "qubits":[*qubits]
        })

    def mct(self, *qubits: int) -> None:
        """
        Class method to apply mcz gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"mcz",
            "qubits":[*qubits]
        })

    def mcmx(self, num_controls: int, *qubits: int) -> None:
        """
        Class method to apply mcmx gate to the given qubits.

        Args:
            num_controls (int): number of control qubit 
            qubits (int): qubits in which the gate is applied, first num_controls will be control qubits and 
                          the following ones will be target qubits.
        """
        self.add_instructions({
            "name":"mcmx",
            "qubits":[*qubits],
            "num_controls": num_controls
        })

    def mcswap(self, *qubits: int) -> None:
        """
        Class method to apply mcswap gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcswap",
            "qubits": [*qubits],
        })

    def mcsqrtswap(self, *qubits: int) -> None:
        """
        Class method to apply mcmcsqrtswapswap gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcsqrtswap",
            "qubits": [*qubits],
        })
    
    # Parametric multicontrol gates
            
    def mcp(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply mcp gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcp",
            "qubits": [*qubits],
            "params": [theta]
        })
    
    def mcrx(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply mcrx gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcrx",
            "qubits": [*qubits],
            "params": [theta]
        })
    
    def mcry(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply mcry gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcry",
            "qubits": [*qubits],
            "params": [theta]
        })
        
    def mcrz(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply mcrz gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcrz",
            "qubits": [*qubits],
            "params": [theta]
        })

    def mcraxis(self, theta: Union[float, int, str], axis: list[float], *qubits: int) -> None:
        """
        Class method to apply mcraxis gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcraxis",
            "qubits": [*qubits],
            "params": [theta],
            "axis": axis
        })
    
    def mcu1(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply mcu1 gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcu1",
            "qubits": [*qubits],
            "params": [theta]
        })
    
    def mcu2(
        self, 
        theta: Union[float, int, str], 
        phi: Union[float, int, str],
        *qubits: int
    ) -> None:
        """
        Class method to apply mcu2 gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcu2",
            "qubits": [*qubits],
            "params": [theta, phi]
        })
    
    def mcu3(
        self, 
        theta:  Union[float, int, str], 
        phi:  Union[float, int, str], 
        lam:  Union[float, int, str],
        *qubits: int
    ) -> None:
        """
        Class method to apply mcu3 gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcu3",
            "qubits": [*qubits],
            "params": [theta, phi, lam]
        })
    
    def mcu(
        self, 
        theta:  Union[float, int, str], 
        phi:  Union[float, int, str], 
        lam:  Union[float, int, str],
        beta: Union[float, int, str],
        *qubits: int
    ) -> None:
        """
        Class method to apply mcu gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            phi (float | int | str): angle.
            lam (float | int | str): angle.
            beta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcu",
            "qubits": [*qubits],
            "params": [theta, phi, lam, beta]
        })

    def mcpaulistr(self, paulistr: str, *qubits: int) -> None:
        """
        Class method to apply mcpaulistr gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcpaulistr",
            "qubits": [*qubits],
            "paulistr": paulistr
        })

    def mcpauligadget(self, theta: Union[float, int, str], paulistr: str, *qubits: int) -> None:
        """
        Class method to apply mcrmcpauligadgetz gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "mcpauligadget",
            "qubits": [*qubits],
            "params": [theta],
            "paulistr": paulistr
        })

    def mcphasegadget(self, theta: Union[float, int, str], num_controls: int, *qubits: int) -> None:
        """
        Class method to apply mcphasegadget gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            num_controls (int): number of controls qubits
            qubits (int): qubits in which the gate is applied, first num_controls will be control qubits and 
                          the following ones will be target qubits.
        """
        self.add_instructions({
            "name": "mcphasegadget",
            "qubits": [*qubits],
            "params": [theta],
            "num_comtrols": num_controls
        })

    # Special gates
    
    def unitary(self, matrix: list[list[complex]], *qubits: int) -> None:
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
            raise ValueError(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                             f"(2^n,2^n) [TypeError].")
            
        matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]

        self.add_instructions({
            "name":"unitary",
            "qubits":[*qubits],
            "matrix":[matrix]
        })

    def cunitary(self, matrix: list[list[complex]], *qubits: int) -> None:
        """
        Class method to apply a controlled unitary gate created from an unitary matrix provided.

        Args:
            matrix (list | numpy.ndarray): unitary operator in matrix form to be applied to the given qubits. 

            qubits (int): qubits to which the unitary operator will be applied. The controlled qubit is the first one.

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
            raise ValueError(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                             f"(2^n,2^n) [TypeError].")
            
        matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]

        self.add_instructions({
            "name":"cunitary",
            "qubits":[*qubits],
            "matrix":[matrix]
        })

    # Alias
    densematrix = unitary

    def sparsematrix(self, matrix: list[list[complex]], *qubits: int) -> None:
        """
        Class method to apply a unitary gate created from an unitary sparse matrix provided. 

        Args:
            matrix (list | numpy.ndarray): sparse munitary operator in matrix form to be applied to the 
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
            raise ValueError(f"matrix must be a list of lists or <class 'numpy.ndarray'> of shape "
                             f"(2^n,2^n) [TypeError].")
            
        matrix = [list(map(lambda z: [z.real, z.imag], row)) for row in matrix]

        self.add_instructions({
            "name":"sparsematrix",
            "qubits":[*qubits],
            "matrix":[matrix]
        })
    

    def randomunitary(self, *qubits: int,  seed = None) -> None:
        """
        Class method to apply a randomunitary gate.

        Args:
            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"randomunitary",
                "qubits":[*qubits],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"randomunitary",
            "qubits":[*qubits]
            })


    def diagonal(self, diagonal: list[complex], *qubits: int) -> None:
        """
        Class method to apply a diagonal gate created from a complex vector.

        Args:
            diagonal (list | numpy.ndarray): list or np.array with diagonal elements

            qubits (int): qubits to which the unitary operator will be applied.

        """
        if (not isinstance(diagonal, np.ndarray) and not isinstance(diagonal, list)):
                raise ValueError(f"diagonal must be a list or <class 'numpy.ndarray'> [TypeError].")
            
        expanded_diagonal = [[z.real, z.imag] for z in diagonal]

        self.add_instructions({
            "name":"diagonal",
            "qubits":[*qubits],
            "matrix":[expanded_diagonal]
        })

    def phasegadget(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply phasegadget gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name": "phasegadget",
            "qubits": [*qubits],
            "params": [theta]
        })

    def cphasegadget(self, theta: Union[float, int, str], *qubits: int) -> None:
        """
        Class method to apply cphasegadget gate to the given qubits.

        Args:
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be the control qubit and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "cphasegadget",
            "qubits": [*qubits],
            "params": [theta]
        })
    
    def paulistr(self, paulistr: str, *qubits: int) -> None:
        """
        Class method to apply paulistr gate to the given qubits.

        Args:
            paulistr (str): pauli str to apply.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name": "paulistr",
            "qubits": [*qubits],
            "paulistr": paulistr
        })

    def cpaulistr(self, paulistr: str, *qubits: int) -> None:
        """
        Class method to apply cpaulistr gate to the given qubits.

        Args:
            paulistr (str): pauli str to apply.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "cpaulistr",
            "qubits": [*qubits],
            "paulistr": paulistr
        })
    
    def pauligadget(self, theta: Union[float, int, str], paulistr: str, *qubits: int) -> None:
        """
        Class method to apply pauligadget gate to the given qubits.

        Args:
            paulistr (str): pauli str to apply.
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "pauligadget",
            "qubits": [*qubits],
            "params": [theta],
            "paulistr": paulistr
        })

    def nonunitarypauligadget(self, theta: Union[float, int, str], paulistr: str, *qubits: int) -> None:
        """
        Class method to apply nonunitarypauligadget gate to the given qubits.

        Args:
            paulistr (str): pauli str to apply.
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied.
        """
        self.add_instructions({
            "name": "nonunitarypauligadget",
            "qubits": [*qubits],
            "params": [theta],
            "paulistr": paulistr
        })

    def cpauligadget(self, theta: Union[float, int, str], paulistr: str, *qubits: int) -> None:
        """
        Class method to apply cpauligadget gate to the given qubits.

        Args:
            paulistr (str): pauli str to apply.
            theta (float | int | str): angle.
            qubits (int): qubits in which the gate is applied, first will be the control qubit and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name": "cpauligadget",
            "qubits": [*qubits],
            "params": [theta],
            "paulistr": paulistr
        })

    def multipauli(self, pauli_id_list: list[int], *qubits: int) -> None:
        """
        Class method to apply a multipauli gate.

        Args:
            pauli_id_list (list[int]): list of Pauli ids.

            qubits (int): qubits to which the unitary operator will be applied.

        """
        self.add_instructions({
            "name":"multipauli",
            "qubits":[*qubits],
            "pauli_id_list":pauli_id_list
        })

    def multipaulirotation(self, param : float, pauli_id_list: list[int], *qubits: int) -> None:
        """
        Class method to apply a multipaulirotation gate.

        Args:
            param (float) : parameter
        
            pauli_id_list (list[int]): list of Pauli ids.

            qubits (int): qubits to which the unitary operator will be applied.

        """
        self.add_instructions({
            "name":"multipaulirotation",
            "qubits":[*qubits],
            "params":[param],
            "pauli_id_list":pauli_id_list
        })

    def amplitudedampingnoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a amplitudedampingnoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"amplitudedampingnoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"amplitudedampingnoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    def bitflipnoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a bitflipnoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"bitflipnoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"bitflipnoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    def dephasingnoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a dephasingnoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"dephasingnoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"dephasingnoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    def depolarizingnoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a depolarizingnoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"depolarizingnoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"depolarizingnoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    def independentxznoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a independentxznoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"independentxznoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"independentxznoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    def twoqubitdepolarizingnoise(self, prob: float, *qubits: int, seed = None) -> None:
        """
        Class method to apply a twoqubitdepolarizingnoise gate.

        Args:
            prob (float): probability.

            qubits (int): qubits to which the unitary operator will be applied.

            seed (None | int): seed.

        """
        if seed:
            self.add_instructions({
                "name":"twoqubitdepolarizingnoise",
                "qubits":[*qubits],
                "params":[prob],
                "seed":seed
            })
        else:
            self.add_instructions({
            "name":"twoqubitdepolarizingnoise",
            "qubits":[*qubits],
            "params":[prob]
            })

    # QuEST-specific 
    # (Any of the following (and their multicontrolled versions) can be controlled by 0: CS, CT, CH, CSWAP, CSQRTSWAP, CY, CZ, CPAULISTR, CRX, CRY, CRZ, CRAXIS, CPAULIGADGET, CPHASEGADGET, CMX, CUNITARY)
    def c0x(self, *qubits: int) -> None:
        """
        Class method to apply an controlled by zero x gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and 
                          second one target qubit.
        """
        self.add_instructions({
            "name":"cx",
            "qubits":[*qubits],
            "states": [0]
        })

    def mc0x(self, states: list[int], *qubits: int) -> None:
        """
        Class method to apply mcx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
            states (list[int]): each entry on the list can be either 1 or 0. Gate is controlled 
                                by the states provided.
        """
        self.add_instructions({
            "name":"mcx",
            "qubits":[*qubits],
            "states": states
        })

            
class ClassicalControlContext:
    def __init__(self, circuit, clbits: Union[int, list[int]], operation, condition: int = 1):
        self._circuit = circuit
        self._clbits = [clbits] if isinstance(clbits, int) else clbits
        self._condition = condition
        self._operation = operation
    
    def __enter__(self):
        self._subcircuit = CunqaCircuit(self._circuit.num_qubits)
        return self._subcircuit
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        instructions = []
        for instr in self._subcircuit.instructions:
            if instr["name"] in ["qsend", "qrecv", "expose", "recv"]:
                raise RuntimeError("Remote operations, quantum or classical, are not allowed within "
                                   "a telegate block.")
            instructions.append(instr)

        cif = {
            "name": "cif",
            "clbits": self._clbits,
            "instructions": instructions,
            "condition": self._condition,
            "operation": self._operation
        }
        self._circuit.add_instructions(cif)
 
        return False

class QuantumControlContext:
    def __init__(
        self, 
        control_circuit: 'CunqaCircuit', 
        target_circuit: 'CunqaCircuit', 
        num_qubits: int
    ) -> int:
        self.num_qubits = num_qubits
        self.control_circuit = control_circuit
        self.target_circuit = target_circuit

    def __enter__(self):
        self._subcircuit = CunqaCircuit(self.target_circuit.num_qubits, self.target_circuit.num_clbits)
        return [-i for i in range(1, self.num_qubits + 1)], self._subcircuit

    def __exit__(self, exc_type, exc_val, exc_tb):
        instructions = []
        for instruction in self._subcircuit.instructions:
            if instruction["name"] in ["qsend", "qrecv", "expose", "recv"]:
                raise RuntimeError("Remote operations, quantum or classical, are not allowed "
                                   "within a telegate block.")
            instructions.append(instruction)

        rcontrol = {
            "name": "rcontrol",
            "instructions": instructions,
            "circuits": [self.control_circuit.info['id']]
        }
        self.target_circuit.add_instructions(rcontrol)

        return False