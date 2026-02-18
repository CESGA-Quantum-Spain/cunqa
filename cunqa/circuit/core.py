"""Core implementation of CUNQA's custom quantum circuit abstraction."""

from __future__ import annotations

import numpy as np
from typing import Union, Optional
from sympy.core.sympify import sympify, SympifyError

from cunqa.circuit.helpers import generate_id
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
          - Single-qubit gates
          - :py:meth:`id`, :py:meth:`x`, :py:meth:`y`, :py:meth:`z`, :py:meth:`h`, :py:meth:`s`, :py:meth:`sdg`,
            :py:meth:`sx`, :py:meth:`sxdg`, :py:meth:`t`, :py:meth:`tdg`, :py:meth:`u1`, :py:meth:`u2`, :py:meth:`u3`,
            :py:meth:`u`, :py:meth:`p`, :py:meth:`r`, :py:meth:`rx`, :py:meth:`ry`, :py:meth:`rz`

        * - 
          - Two-qubit gates
          - :py:meth:`swap`, :py:meth:`cx`, :py:meth:`cy`, :py:meth:`cz`, :py:meth:`csx`, :py:meth:`cp`, :py:meth:`cu`,
            :py:meth:`cu1`, :py:meth:`cu3`, :py:meth:`rxx`, :py:meth:`ryy`, :py:meth:`rzz`, :py:meth:`rzx`,
            :py:meth:`crx`, :py:meth:`cry`, :py:meth:`crz`, :py:meth:`ecr`

        * - 
          - Three-qubit gates
          - :py:meth:`ccx`, :py:meth:`ccy`, :py:meth:`ccz`, :py:meth:`cswap`

        * - 
          - Multi-qubit gates
          - :py:meth:`unitary`

        * - Local non-unitary operations
          - Local non-unitary operations
          - :py:meth:`cif`, :py:meth:`measure`, :py:meth:`measure_all`, :py:meth:`reset`

        * - Remote operations
          - Classical communication
          - :py:meth:`send`, :py:meth:`recv`

        * - 
          - Quantum communication
          - :py:meth:`qsend`, :py:meth:`qrecv`, :py:meth:`expose`


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
    .. automethod:: send
    .. automethod:: recv


    Quantum communication directives
    --------------------------------
    .. automethod:: qsend
    .. automethod:: qrecv
    .. automethod:: expose


    Non-unitary operations
    ----------------------
    .. automethod:: cif
    .. dropdown:: All non-unitary operations
        :animate: fade-in-slide-down

        .. automethod:: measure_all
        .. automethod:: measure
        .. automethod:: reset

    Unitary operations
    ------------------
    .. automethod:: unitary

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
        .. automethod:: t
        .. automethod:: tdg
        .. automethod:: u1
        .. automethod:: u2
        .. automethod:: u3
        .. automethod:: u
        .. automethod:: p
        .. automethod:: r
        .. automethod:: rx
        .. automethod:: ry
        .. automethod:: rz

    .. dropdown:: Two-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: swap
        .. automethod:: cx
        .. automethod:: cy
        .. automethod:: cz
        .. automethod:: csx
        .. automethod:: cp
        .. automethod:: cu
        .. automethod:: rxx
        .. automethod:: ryy
        .. automethod:: rzz
        .. automethod:: rzx
        .. automethod:: crx
        .. automethod:: cry
        .. automethod:: crz
        .. automethod:: ecr
    
    .. dropdown:: Three-qubit gates
        :animate: fade-in-slide-down

        .. automethod:: ccx
        .. automethod:: ccy
        .. automethod:: ccz
        .. automethod:: cswap
        
    """

    
    # global attributes
    _ids: set = set() #: Set with ids in use.
    _communicated: dict[str, CunqaCircuit] = {} #: Dictionary with the circuits that employ communication directives.

    _id: str #: Circuit identifier.
    is_dynamic: bool #: Whether the circuit has local non-unitary operations.
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
        self.instructions = []
        self.params = []
        self.quantum_regs = {}
        self.classical_regs = {}        
        self.sending_to = set()

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
            "sending_to": list(self.sending_to),
            "params": self.params
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
        Class method to add an instruction to the CunqaCircuit. 

        Args:
            instruction (dict | list[dist]): instruction to be added.
        """
        
        if "params" in instructions:
            
            # Converting the string to a symbolic expression
            try:
                exprs = sympify(instructions["params"])
            except SympifyError:
                raise ValueError(f"Expression {instructions['params']} cannot be converted to "
                                 f"symbolic expression.")
            
            # Adding to the instruction the Param object or a real number depending on the specified
            # (if real, the parameter will not be changed)
            new_list = []
            for expr, param in zip(exprs, instructions["params"]):
                if not expr.is_real:
                    new_param = Param(expr)
                    self.params.append(new_param)
                    new_list.append(new_param)
                else:
                    new_list.append(param)
            instructions["params"] = new_list
            
        if (isinstance(instructions, dict)):
            self.instructions.append(instructions)
        else:
            for instr in instructions:
                self.instructions.append(instr)
                    
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
    # Methods for implementing non parametric single-qubit gates

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

    def P0(self, qubit: int) -> None:
        """
        Class method to apply P0 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"p0",
            "qubits":[qubit]
        })

    def P1(self, qubit: int) -> None:
        """
        Class method to apply P1 gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"p1",
            "qubits":[qubit]
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

    # methods for non parametric two-qubit gates

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

    # methods for non parametric three-qubit gates

    def ccx(self, *qubits: int) -> None:
        """
        Class method to apply ccx gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first two will be control qubits and 
                          the following one will be target qubit.
        """
        self.add_instructions({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits: int) -> None:
        """
        Class method to apply ccy gate to the given qubits. Gate is decomposed as follows as it is 
        not commonly supported by simulators.

        .. code-block::
        
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
        self.add_instructions({
            "name":"rz",
            "qubits":[qubits[-1]],
            "params":[-np.pi/2]
        })
        self.add_instructions({
            "name":"ccx",
            "qubits":[*qubits]
        })
        self.add_instructions({
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
        self.add_instructions({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cecr(self, *qubits: int) -> None:
        """
        Class method to apply cecr gate to the given qubits.

        Args:
            qubits (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self.add_instructions({
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
        self.add_instructions({
            "name":"cswap",
            "qubits":[*qubits]
        })

    
    # methods for parametric single-qubit gates

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
            "qubits":[qubit],
            "params":[theta,phi]
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
            theta (float | int): angle.
            phi (float | int): angle.
            lam (float | int): angle.
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
            theta (float | int): angle.
            phi (float | int): angle.
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

    def RotInvX(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply RotInvX gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvx",
            "qubits":[qubit],
            "params":[param]
        })

    def RotInvY(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply rRotInvY gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvy",
            "qubits":[qubit],
            "params":[param]
        })
    
    def RotInvZ(self, param:  Union[float,int, str], qubit: int) -> None:
        """
        Class method to apply RotInvZ gate to the given qubit.

        Args:
            param (float | int | str): parameter for the parametric gate.

            qubit (int): qubit in which the gate is applied.
        """
        self.add_instructions({
            "name":"rotinvz",
            "qubits":[qubit],
            "params":[param]
        })

    # methods for parametric two-qubit gates

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

    def cr(self, param:  Union[float,int, str], *qubits: int) -> None:
        """
        Class method to apply cr gate to the given qubits.

        Args:
            param (float | int | str): parameter for the parametric gate.
            qubits (int): qubits in which the gate is applied, first one will be the control qubit 
                          and second one the target qubit.
        """
        self.add_instructions({
            "name":"cr",
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
        self.add_instructions({
            "name":"cu",
            "qubits":[*qubits],
            "params":[theta, phi, lam, gamma]
        })
    
    # methods for implementing conditional LOCAL gates
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
            "elements":[matrix]
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

        self.add_instructions({
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
            self.add_instructions({
                "name":"measure",
                "qubits":[q],
                "clbits":[c]
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

    def cif(
            self, 
            clbits: Union[int, list[int]]
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
            clbits (int | list[int]): clbits employed as the condition.
        """
        self.is_dynamic = True
        return ClassicalControlContext(self, clbits)

    def send(self, clbits: Union[int, list[int]], recving_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a bit (previously measured from a qubit) from the current circuit to a remote one. 
        
        Args:

            clbits (int): bits to be sent.

            recving_circuit (str | CunqaCircuit): id of the circuit or circuit object to which the 
                                                bit is sent.

        """
        self.is_dynamic = True
        
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

        self.is_dynamic = True

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

    def qsend(self, qubit: int, recving_circuit: Union[str, 'CunqaCircuit']) -> None:
        """
        Class method to send a qubit from the current circuit to another one.
        
        Args:
            qubit (int): qubit to be sent.

            recving_circuit (str | CunqaCircuit): id of the circuit or circuit to which the qubit is 
                                                 sent.
        """
        self.is_dynamic = True
        
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
        self.is_dynamic = True
        
        if isinstance(control_circuit, str):
            control_circuit_id = control_circuit
        elif isinstance(control_circuit, CunqaCircuit):
            control_circuit_id = control_circuit.id
        
        self.add_instructions({
            "name": "qrecv",
            "qubits": [qubit],
            "circuits": [control_circuit_id]
        })

    def expose(self, qubit: int, target_circuit: Union[str, 'CunqaCircuit']) -> 'QuantumControlContext':
        """
        Class method to expose a qubit from the current circuit to another one for a telegate 
        operation. The exposed qubit will be used at the target circuit as the control qubit in 
        controlled operations.
        
        Args:
            qubit (int): qubit to be exposed.
            target_circuit (str | CunqaCircuit): id of the circuit or circuit object where the exposed 
                                                 qubit is used.
        
        Returns:
            A :py:class:`QuantumControlContext` object to manage remotly controlled operations in the given 
            circuit.

        Usage example:

        .. code-block:: python

            with origin_circ.expose(0, target_circuit) as rqubit, subcircuit:
                subcircuit.cx(rqubit, 1)
            
        """ 
        self.is_dynamic = True
        
        if isinstance(target_circuit, str):
            target_circuit_id = target_circuit
        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit_id = target_circuit.id
        
        self.add_instructions({
            "name": "expose",
            "qubits": [qubit],
            "circuits": [target_circuit_id]
        })
        return QuantumControlContext(self, target_circuit)


class ClassicalControlContext:
    def __init__(self, circuit, clbits: Union[int, list[int]]):
        self._circuit = circuit
        self._clbits = clbits
    
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
            "clbits": [self._clbits],
            "instructions": instructions
        }
        self._circuit.add_instructions(cif)
 
        return False

class QuantumControlContext:
    def __init__(self, control_circuit: 'CunqaCircuit', target_circuit: 'CunqaCircuit') -> int:
        self.control_circuit = control_circuit
        self.target_circuit = target_circuit

    def __enter__(self):
        self._subcircuit = CunqaCircuit(self.target_circuit.num_qubits, self.target_circuit.num_clbits)
        return -1, self._subcircuit

    def __exit__(self, exc_type, exc_val, exc_tb):
        instructions = []
        for instruction in self._subcircuit.instructions:
            if instruction["name"] in ["qsend", "qrecv", "expose", "recv"]:
                raise RuntimeError("Remote operations, quantum or classical, are not allowed within a telegate block.")
            instructions.append(instruction)

        rcontrol = {
            "name": "rcontrol",
            "instructions": instructions,
            "circuits": [self.control_circuit.info['id']]
        }
        self.target_circuit.add_instructions(rcontrol)

        return False