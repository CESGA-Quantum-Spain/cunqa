"""
    Holds our custom circuit class and functions to transform between circuit formats.
"""
from cunqa.logger import logger
import numpy as np
import random
import string

def generate_id(size=4):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choices(chars, k=size))

SUPPORTED_GATES_1Q = ["id","x", "y", "z", "h", "s", "sdg", "sx", "sxdg", "t", "tdg", "u1", "u2", "u3", "u", "p", "r", "rx", "ry", "rz"]
SUPPORTED_GATES_2Q = ["swap", "cx", "cy", "cz", "csx", "cp", "cu", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "crx", "cry", "crz", "ecr", "c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz", "d_c_if_h", "d_c_if_x","d_c_if_y","d_c_if_z","d_c_if_rx","d_c_if_ry","d_c_if_rz"]
SUPPORTED_GATES_3Q = [ "ccx","ccy", "ccz","cswap"]
SUPPORTED_GATES_PARAMETRIC_1 = ["u1", "p", "rx", "ry", "rz", "rxx", "ryy", "rzz", "rzx","cp", "crx", "cry", "crz", "cu1","c_if_rx","c_if_ry","c_if_rz", "d_c_if_rx","d_c_if_ry","d_c_if_rz"]
SUPPORTED_GATES_PARAMETRIC_2 = ["u2", "r"]
SUPPORTED_GATES_PARAMETRIC_3 = ["u", "u3", "cu3"]
SUPPORTED_GATES_PARAMETRIC_4 = ["cu"]
SUPPORTED_GATES_CONDITIONAL = ["c_if_unitary","c_if_h", "c_if_x","c_if_y","c_if_z","c_if_rx","c_if_ry","c_if_rz","c_if_cx","c_if_cy","c_if_cz"]
SUPPORTED_GATES_DISTRIBUTED = ["d_c_if_unitary", "d_c_if_h", "d_c_if_x","d_c_if_y","d_c_if_z","d_c_if_rx","d_c_if_ry","d_c_if_rz","d_c_if_cx","d_c_if_cy","d_c_if_cz", "d_c_if_ecr"]
all_gates = set(SUPPORTED_GATES_1Q)
all_gates.update(SUPPORTED_GATES_2Q + SUPPORTED_GATES_3Q + SUPPORTED_GATES_PARAMETRIC_1 + SUPPORTED_GATES_PARAMETRIC_2 + SUPPORTED_GATES_PARAMETRIC_3 + SUPPORTED_GATES_PARAMETRIC_4 + SUPPORTED_GATES_CONDITIONAL + SUPPORTED_GATES_DISTRIBUTED)


class CunqaCircuitError(Exception):
    """Exception for error during circuit desing in ``CunqaCircuit``."""
    pass

class CunqaCircuit:
    f"""
    Class to define a quantum circuit for the `cunqa` api.

    Supported gates:
        {all_gates!r}
    """

    def __init__(self, num_qubits, num_clbits = None, id = None):

        if not isinstance(num_qubits, int):
            logger.error(f"num_qubits must be an int, but a {type(num_qubits)} was provided [TypeError].")
            raise SystemExit
        
        if id is None:
            self._id = "cunqacircuit_" + generate_id()
        elif isinstance(id, str):
            self._id = id
        else:
            logger.error(f"id must be a str, but a {type(id)} was provided [TypeError].")
            raise SystemExit
        
        self.is_parametric = False


        self.instructions = []

        self.quantum_regs = {'q0':[q for q in range(num_qubits)]}

        if num_clbits is None:
            self.classical_regs = {}
        
        elif isinstance(num_clbits, int):
            self.classical_regs = {'c0':[c for c in range(num_clbits)]}


    @property
    def num_qubits(self):
        return len(flatten([[q for q in qr] for qr in self.quantum_regs.values()]))
    
    @property
    def num_clbits(self):
        return len(flatten([[c for c in cr] for cr in self.classical_regs.values()]))

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
                elif (instruction["name"] in SUPPORTED_GATES_2Q) or (instruction["name"] in SUPPORTED_GATES_DISTRIBUTED):
                    # we include as 2 qubit gates the distributed gates
                    gate_qubits = 2
                elif (instruction["name"] in SUPPORTED_GATES_3Q):
                    gate_qubits = 3

                elif any([instruction["name"] == u for u in ["unitary", "c_if_unitary", "d_c_if_unitary"]]) and ("params" in instruction):
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
                    elif (instruction["name"] in SUPPORTED_GATES_DISTRIBUTED and len(set(instruction["qubits"])) != len(instruction["qubits"][1:])):
                        logger.error(f"qubits provided for instruction cannot be repeated.")
                        raise ValueError
                    elif (instruction["name"] not in SUPPORTED_GATES_DISTRIBUTED and len(set(instruction["qubits"])) != len(instruction["qubits"])):
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
                if ("params" in instruction) and (not instruction["name"] in {"unitary", "c_if_unitary", "d_c_if_unitary"}) and (len(instruction["params"]) != 0):
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
    
    # =============== INSTRUCTIONS ===============
    
    # Methods for implementing non parametric single-qubit gates

    def id(self, qubit):
        """
        Class method to apply id gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"id",
            "qubits":[qubit]
        })
    
    def x(self, qubit):
        """
        Class method to apply x gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"x",
            "qubits":[qubit]
        })
    
    def y(self, qubit):
        """
        Class method to apply y gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"y",
            "qubits":[qubit]
        })

    def z(self, qubit):
        """
        Class method to apply z gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"z",
            "qubits":[qubit]
        })
    
    def h(self, qubit):
        """
        Class method to apply h gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"h",
            "qubits":[qubit]
        })

    def s(self, qubit):
        """
        Class method to apply s gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"y",
            "qubits":[qubit]
        })

    def sdg(self, qubit):
        """
        Class method to apply sdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sdg",
            "qubits":[qubit]
        })

    def sx(self, qubit):
        """
        Class method to apply sx gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sx",
            "qubits":[qubit]
        })
    
    def sxdg(self, qubit):
        """
        Class method to apply sxdg gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"sxdg",
            "qubits":[qubit]
        })
    
    def t(self, qubit):
        """
        Class method to apply t gate to the given qubit.

        Args:
            qubit (int): qubit in which the gate is applied.
        """
        self._add_instruction({
            "name":"t",
            "qubits":[qubit]
        })
    
    def tdg(self, qubit):
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

    def swap(self, *qubits):
        """
        Class method to apply swap gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"swap",
            "qubits":[*qubits]
        })

    def ecr(self, *qubits):
        """
        Class method to apply ecr gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied.
        """
        self._add_instruction({
            "name":"ecr",
            "qubits":[*qubits]
        })

    def cx(self, *qubits):
        """
        Class method to apply cx gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cx",
            "qubits":[*qubits]
        })
    
    def cy(self, *qubits):
        """
        Class method to apply cy gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cy",
            "qubits":[*qubits]
        })

    def cz(self, *qubits):
        """
        Class method to apply cz gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first one will be control qubit and second one target qubit.
        """
        self._add_instruction({
            "name":"cz",
            "qubits":[*qubits]
        })
    
    def csx(self, *qubits):
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

    def ccx(self, *qubits):
        """
        Class method to apply ccx gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccx",
            "qubits":[*qubits]
        })

    def ccy(self, *qubits):
        """
        Class method to apply ccy gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccy",
            "qubits":[*qubits]
        })

    def ccz(self, *qubits):
        """
        Class method to apply ccz gate to the given qubits.

        Args:
            ``*qubits`` (int): qubits in which the gate is applied, first two will be control qubits and the following one will be target qubit.
        """
        self._add_instruction({
            "name":"ccz",
            "qubits":[*qubits]
        })

    def cswap(self, *qubits):
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

    def u1(self, param, qubit):
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
    
    def u2(self, theta, phi, qubit):
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

    def u(self, theta, phi, lam, qubit):
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

    def u3(self, theta, phi, lam, qubit):
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

    def p(self, param, qubit):
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

    def r(self, theta, phi, qubit):
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

    def rx(self, param, qubit):
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

    def ry(self, param, qubit):
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
    
    def rz(self, param, qubit):
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

    def rxx(self, param, *qubits):
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
    
    def ryy(self, param, *qubits):
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

    def rzz(self, param, *qubits):
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

    def rzx(self, param, *qubits):
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

    def crx(self, param, *qubits):
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

    def cry(self, param, *qubits):
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

    def crz(self, param, *qubits):
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

    def cp(self, param, *qubits):
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

    def cu1(self, param, *qubits):
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
    
    def cu3(self, theta, phi, lam, *qubits): # three parameters
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
    
    def cu(self, theta, phi, lam, gamma, *qubits): # four parameters
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
    def unitary(self, matrix, *qubits):
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
        

    def measure(self, qubits, clbits):
        """
        Class method to add a measurement of a qubit or a list of qubits and to register that measurement in the given classical bits.

        Args:
            qubits (list[int] or int): qubits to measure.

            clbits (list[int] or int): clasical bits where the measurement will be registered.
        """
        if not (isinstance(qubits,list) and isinstance(clbits,list)):
            qubits = [qubits]; clbits = [clbits]
        
        for q,c in zip(qubits,clbits):
            self._add_instruction({
                "name":"measure",
                "qubits":[q],
                "clbits":[c]
            })

    def measure_all(self):
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


    def c_if(self, gate, control_qubit, target_qubit, param = None, matrix = None):
        """
        Method for implementing a gate contiioned to a classical measurement. The control qubit provided is measured, if it's 1 the gate provided is applied to the given qubits.

        For parametric gates, only one-parameter gates are supported, therefore only one parameter must be passed.

        Args:
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
            control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubit, int):
            target_qubit = [target_qubit]
        elif isinstance(target_qubit, list):
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
                "qubits": flatten([control_qubit, target_qubit]),
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
            elif isinstance(param, float) or isinstance(param,int):
                param = [param]
            else:
                logger.error(f"param must be int or float, but {type(param)} was provided [TypeError].")
                raise SystemExit
        else:
            if param is not None:
                logger.warning("A parameter was provided but gate is not parametric, therefore it will be ignored.")
            param = []


        
        if name in SUPPORTED_GATES_CONDITIONAL:

            self._add_instruction({
                "name": name,
                "qubits": flatten([control_qubit, target_qubit]),
                "params":param
            })

        else:
            logger.error(f"Gate {gate} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing 
        

    def send_gate(self, gate, param, control_qubit = None, target_qubit = None, target_circuit = None):
        """
        Class method to apply a distributed instruction as a gate condioned by a local classical measurement and applied in a different circuit.
        
        Args:
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            control_qubit (int): control qubit from self.

            target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit to which we will send the gate or the circuit itself.

            target_qubit (int): qubit where the gate will be conditionally applied.

            param (float or int): parameter in case the gate provided is parametric.

        """

        if isinstance(gate, str):
            name = "d_c_if_" + gate
        else:
            logger.error(f"gate specification must be str, but {type(gate)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(control_qubit, int):
            control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubit, int):
            target_qubit = [target_qubit]
        elif isinstance(target_qubit, list):
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(target_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if param is not None:
            params = [param]
        else:
            params = []

        if target_circuit is None:
            logger.error("target_circuit not provided.")
            raise SystemExit
        
        elif isinstance(target_circuit, str):
            target_circuit = target_circuit

        elif isinstance(target_circuit, CunqaCircuit):
            target_circuit = target_circuit._id
        else:
            logger.error(f"target_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(target_circuit)} was provided [TypeError].")
            raise SystemExit
        
        if name in SUPPORTED_GATES_DISTRIBUTED:

            self._add_instruction({
                "name": name,
                "qubits": flatten([control_qubit, target_qubit]),
                "params":params,
                "circuits": [self._id, target_circuit]
            })
        else:
            logger.error(f"Gate {gate.__str__} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing 
    

    def recv_gate(self, gate, param, control_qubit = None, control_circuit = None, target_qubit = None):
        """
        Class method to apply a distributed instruction as a gate condioned by a non local classical measurement from a different circuit and applied locally.
        
        Args:   
            gate (str): gate to be applied. Has to be supported by CunqaCircuit.

            param (float or int): parameter in case the gate provided is parametric.

            control_qubit (int): control qubit from self.

            target_circuit (str, <class 'cunqa.circuit.CunqaCircuit'>): id of the circuit to which we will send the gate or the circuit itself.

            target_qubit (int): qubit where the gate will be conditionally applied.       
        """

        if isinstance(gate, str):
            name = "d_c_if_" + gate
        else:
            logger.error(f"gate specification must be str, but {type(gate)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(control_qubit, int):
            control_qubit = [control_qubit]
        else:
            logger.error(f"control qubit must be int, but {type(control_qubit)} was provided [TypeError].")
            raise SystemExit
        
        if isinstance(target_qubit, int):
            target_qubit = [target_qubit]
        elif isinstance(target_qubit, list):
            pass
        else:
            logger.error(f"target qubits must be int ot list, but {type(target_qubit)} was provided [TypeError].")
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
            control_circuit = control_circuit._id
        else:
            logger.error(f"control_circuit must be str or <class 'cunqa.circuit.CunqaCircuit'>, but {type(control_circuit)} was provided [TypeError].")
            raise SystemExit
        
        if name in SUPPORTED_GATES_DISTRIBUTED:

            self._add_instruction({
                "name": name,
                "qubits": flatten([control_qubit, target_qubit]),
                "params":params,
                "circuits": [control_circuit, self._id]
            })
        else:
            logger.error(f"Gate {gate.__str__} is not supported for conditional operation.")
            raise SystemExit
            # TODO: maybe in the future this can be check at the begining for a more efficient processing



                
def flatten(lists):
    return [element for sublist in lists for element in sublist]

from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit

def qc_to_json(qc: QuantumCircuit):
    """
    Transforms a QuantumCircuit to json dict.

    Args:
        qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): circuit to transform to json.

    Return:
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
        
        quantum_registers, classical_registers = _registers_dict(qc)
        
        json_data = {
            "id": "",
            "is_parametric": is_parametric(qc),
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
                                                "memory":[classical_registers[k][b] for k,b in zip(creg,bit)]
                                                })
                    

        return json_data
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from QuantumCircuit to json dict [{type(error).__name__}].")
        raise error

def from_json_to_qc(circuit_dict):
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
                inst = CircuitInstruction( 
                    operation = Instruction(name = instruction['name'],
                                            num_qubits = len(instruction['qubits']),
                                            num_clbits = 0,
                                            params = instruction['params']
                                            ),
                    qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                    clbits = ()
                    )
                qc.append(inst)
            elif instruction['name'] == 'measure':
                bit = instruction['memory'][0]
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
                else:
                    logger.error(f"Bit {bit} not found in {bits}, please check the format of the circuit json.")
                    raise IndexError
                qc.append(inst)
                
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

def _registers_dict(qc):
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

def is_parametric(circuit):
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
    elif isinstance(circuit, CunqaCircuit):
        return circuit.is_parametric
    elif isinstance(circuit, str):
        lines = circuit.splitlines()
        for line in lines:
            line = line.strip()
            if any(line.startswith(gate) for gate in parametric_gates):
                return True
        return False
