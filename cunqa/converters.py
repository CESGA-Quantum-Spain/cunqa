"""
    Holds functions for converting circuits into the different formats: :py:class:`qiskit.QuantumCircuit`, :py:class:`~cunqa.circuit.CunqaCircuit` and json :py:class:`dict`.

    There is the general :py:func:`convert` function, that identifies the input format and transforms according to the format desired by the variable *convert_to*.

    .. warning::
        It is not possible to convert circuits with classical or quantum communications instructions into :py:class:`qiskit.QuantumCircuit`
        since these are not supported by this format. It one tries, an error will be raised.
"""


from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit
from qiskit.qasm2 import dumps as dumps2
from qiskit.qasm3 import dumps as dumps3

from typing import Tuple, Union, Optional

from cunqa.circuit import CunqaCircuit, _is_parametric
from cunqa.logger import logger

class ConvertersError(Exception):
    """Exception for error during conversion between circuit types."""
    pass

SUPPORTED_QISKIT_OPERATIONS = {'unitary','ryy', 'rz', 'z', 'p', 'rxx', 'rx', 'cx', 'id', 'x', 'sxdg', 'u1', 'ccy', 'rzz', 'rzx', 'ry', 's', 'cu', 'crz', 'ecr', 't', 'ccx', 'y', 'cswap', 'r', 'sdg', 'csx', 'crx', 'ccz', 'u3', 'u2', 'u', 'cp', 'tdg', 'sx', 'cu1', 'swap', 'cy', 'cry', 'cz','h', 'cu3', 'measure', 'if_else', 'barrier'}

def convert(circuit : Union['QuantumCircuit', 'CunqaCircuit', dict], convert_to : str) -> Union['QuantumCircuit', 'CunqaCircuit', str, dict]:
    """
        Function to convert a quantum circuit to the desired format.
        Detects the intup format and transforms into the one specified by *convert_to*, that can be ``"QuantumCircuit`` for :py:class:`qiskit.QuantumCircuit`,
        ``"CunqaCircuit"`` for :py:class:`~cunqa.circuit.CunqaCircuit` and ``"dict"`` for a json :py:class:`dict`.

        Args:
            circuit (qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit | dict): circuit to be transformed.
            convert_to (str): especification of target format, can be ``"QuantumCircuit``, ``"CunqaCircuit"`` or ``"dict"``.
        
        Returns:
            The circuit in the desired format accordingly to *convert_to*.

    """
    if convert_to not in ["QuantumCircuit", "CunqaCircuit", "qasm", "dict"]:
        logger.error(f"{convert_to} is not a valid circuit format to convert to [{NameError.__name__}].")
        raise SystemExit


    try:
        if isinstance(circuit, QuantumCircuit):
            if convert_to == "QuantumCircuit":
                logger.warning("Provided circuit was already a QuantumCircuit.")
                converted_circuit = circuit
            elif convert_to == "CunqaCircuit":
                converted_circuit = _qc_to_cunqac(circuit)
            elif convert_to == "dict":
                converted_circuit = _qc_to_json(circuit)
            elif convert_to == "qasm":
                converted_circuit = _qc_to_qasm(circuit)

        elif isinstance(circuit, CunqaCircuit):
            if convert_to == "QuantumCircuit":
                converted_circuit = _cunqac_to_qc(circuit)
            elif convert_to == "CunqaCircuit":
                logger.warning("Provided circuit was already a CunqaCircuit.")
                converted_circuit = circuit
            elif convert_to == "dict":
                converted_circuit = _cunqac_to_json(circuit)
            elif convert_to == "qasm":
                converted_circuit = _cunqac_to_qasm(circuit)

        elif isinstance(circuit, dict):
            if convert_to == "QuantumCircuit":
                converted_circuit = _json_to_qc(circuit)
            elif convert_to == "CunqaCircuit":
                converted_circuit = _json_to_cunqac(circuit)
            elif convert_to == "dict":
                logger.warning("Provided circuit was already a dict.")
                converted_circuit = circuit
            elif convert_to == "qasm":
                converted_circuit = _json_to_qasm(circuit)
        elif isinstance(circuit, str):
            if convert_to == "QuantumCircuit":
                converted_circuit = _qasm_to_qc(circuit)
            elif convert_to == "CunqaCircuit":
                converted_circuit = _qasm_to_cunqac(circuit)
            elif convert_to == "dict":
                converted_circuit = _qasm_to_json(circuit)
            elif convert_to == "qasm":
                logger.warning("Provided circuit was already a OpenQASM.")
                converted_circuit = circuit

        else:
            logger.error(f"Provided circuit must be a QuantumCircuit, a CunqaCircuit, an OpenQASM or a dict [{TypeError.__name__}].")
            raise SystemExit
        
        return converted_circuit
    except Exception as error:
            logger.error(f" Unable to convert circuit to {convert_to} [{type(error).__name__}].")
            raise SystemExit


def _qc_to_cunqac(qc : 'QuantumCircuit') -> 'CunqaCircuit':
    """
    Converts a :py:class:`qiskit.QuantumCircuit` into a :py:class:`~cunqa.circuit.CunqaCircuit`.

    Args:
        qc (qiskit.QuantumCircuit): object that defines the quantum circuit.
    Returns:
        The corresponding :py:class:`~cunqa.circuit.CunqaCircuit` with the propper instructions and characteristics.
    """
    return _json_to_cunqac(_qc_to_json(qc))


def _qc_to_json(qc : 'QuantumCircuit') -> dict:
    """
    Transforms a :py:class:`qiskit.QuantumCircuit` to json :py:class:`dict`.

    Args:
        qc (qiskit.QuantumCircuit): circuit to transform to json.

    Return:
        Json dict with the circuit information.
    """
    try:
        
        quantum_registers, classical_registers = _registers_dict(qc)

        logger.debug(f"Localized quamtum registers: {quantum_registers}\n Localized classical registers: {classical_registers}")
        
        json_data = {
            "id": "",
            "is_parametric": _is_parametric(qc),
            "is_dynamic": False,
            "has_cc":False,
            "has_qc":False,
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers,
            "has_cc":False,
            "has_qc":False,
        }

        for instruction in qc.data:

            logger.debug(f"Processing instruction: {instruction}")

            if instruction.name not in SUPPORTED_QISKIT_OPERATIONS:
                logger.error(f"Instruction {instruction.name} not supported for conversion [ValueError].")
                raise ConvertersError

            qreg = [r._register.name for r in instruction.qubits]
            qubit = [q._index for q in instruction.qubits]
            
            clreg = [r._register.name for r in instruction.clbits]
            bit = [b._index for b in instruction.clbits]

            if instruction.name == "barrier":
                pass

            elif instruction.name == "measure":

                json_data["instructions"].append({"name":instruction.name,
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "clbits":[classical_registers[k][b] for k,b in zip(clreg, bit)]
                                                })

            elif instruction.name == "unitary":

                json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":[[list(map(lambda z: [z.real, z.imag], row)) for row in instruction.params[0].tolist()]] #only difference, it ensures that the matrix appears as a list, and converts a+bj to (a,b)
                                                })
                
            elif instruction.name == "if_else":

                json_data["is_dynamic"] = True

                # beacuse of Qiskit's notation, the mesaurement is already done, we do not need to add it.
                # we use as conditional_reg the clbit specified by the Qiskit instruction

                if not any([sub_circuit is None for sub_circuit in instruction.params]):
                    logger.error("if_else instruction with \'else\' case is not supported for the current version [ValueError].")
                    raise ConvertersError
                else:
                    sub_circuit = [sub_circuit for sub_circuit in instruction.params if sub_circuit is not None][0]

                if instruction.condition[1] not in [1]:
                    logger.error("Only 1 is accepted as condition for classicaly contorlled operations for the current version [ValueError].")
                    raise ConvertersError
                
                for re in qc.qregs:
                    sub_circuit.add_register(re)

                sub_instructions = _qc_to_json(sub_circuit)["instructions"]

                for sub_instruction in sub_instructions:

                    sub_instruction["conditional_reg"] = [classical_registers[k][b] for k,b in zip(clreg, bit)]
                    json_data["instructions"].append(sub_instruction)
                
            elif (instruction.operation._condition != None):

                if instruction.operation._condition[1] not in [1]:
                    logger.error("Only 1 is accepted as condition for classicaly contorlled operations for the current version [ValueError].")
                    raise ConvertersError

                json_data["is_dynamic"] = True
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
       
        return json_data
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from `qiskit.QuantumCircuit` to json dict [{type(error).__name__}].")
        logger.error(f"{error}")
        raise error
    

def _qc_to_qasm(qc : 'QuantumCircuit', version = "3.0") -> str:
    
    try:
        if (version == "2.0"):
            return dumps2(qc)
        elif (version == "3.0"):
            return dumps3(qc)
        else:
            logger.error(f"OpenQASM{version} is not supported.")
            raise SystemExit
    except Exception as error:
        logger.error(f" Unable to convert circuit to OpenQASM{version} [{type(error).__name__}].")
        raise SystemExit
    

def _cunqac_to_qc(cunqac : 'CunqaCircuit') -> 'QuantumCircuit':
    """
    Converts a :py:class:`~cunqa.circuit.CunqaCircuit` into a :py:class:`qiskit.QuantumCircuit`.

    Args:
        cunqac (~cunqa.circuit.CunqaCircuit): object that defines the quantum circuit.

    Returns:
        The corresponding :py:class:`qiskit.QuantumCircuit` with the propper instructions and characteristics.
    """
    return _json_to_qc(_cunqac_to_json(cunqac))


def _cunqac_to_json(cunqac : 'CunqaCircuit') -> dict:
    circuit_json = {}
    circuit_json["id"] = cunqac._id
    circuit_json["is_parametric"] = cunqac.is_parametric
    circuit_json["is_dynamic"] = cunqac.is_dynamic
    circuit_json["has_cc"] = cunqac.has_cc
    circuit_json["has_qc"] = cunqac.has_qc
    circuit_json["num_qubits"] = cunqac.num_qubits
    circuit_json["num_clbits"] = cunqac.num_clbits
    circuit_json["quantum_registers"] = cunqac.quantum_regs
    circuit_json["classical_registers"] = cunqac.classical_regs
    circuit_json["instructions"] = cunqac.instructions

    return circuit_json


def _cunqac_to_qasm(cunqac : 'CunqaCircuit') -> str:
    return _qc_to_qasm(_cunqac_to_qc(cunqac))
    

def _json_to_qc(circuit_dict: dict) -> 'QuantumCircuit':
    """
    Function to transform a circuit in json dict format to :py:class:`qiskit.QuantumCircuit`.

    Args:
        circuit_dict (dict): circuit instructions to be transformed.

    Return:
        :py:class:`qiskit.QuantumCircuit` with the given instructions.
    """

    if "has_cc" in circuit_dict:
        if circuit_dict["has_cc"]:
            logger.error(f"Cannot convert to QuantumCircuit a circuit with classsical communications [{TypeError.__name__}].")
            raise ConvertersError
    elif "has_qc" in circuit_dict:
        if circuit_dict["has_qc"]:
            logger.error(f"Cannot convert to QuantumCircuit a circuit with quantum communications [{TypeError.__name__}].")
            raise ConvertersError

    #Extract key information from the json
    try:
        instructions = circuit_dict['instructions']
        num_qubits = circuit_dict['num_qubits']
        quantum_registers = circuit_dict['quantum_registers']
        classical_registers = circuit_dict['classical_registers']

    except KeyError as error:
        logger.error(f"Circuit json not correct, requiered keys must be: 'instructions', 'num_qubits', 'num_clbits', 'quantum_resgisters' and 'classical_registers' [{type(error).__name__}].")
        raise error
        
    # Proceed with translation
    try:
        qc = QuantumCircuit()

        qubits = []
        for qr, lista in quantum_registers.items():
            for i in lista: 
                qubits.append(i)
            qc.add_register(QuantumRegister(len(lista), qr))

        clbits = []
        for cr, lista in classical_registers.items():
            for i in lista: 
                clbits.append(i)
            qc.add_register(ClassicalRegister(len(lista), cr))

        for instruction in instructions:

            if instruction['name'] not in SUPPORTED_QISKIT_OPERATIONS:
                logger.error(f"Instruction {instruction['name']} not supported for conversion [ValueError].")
                raise ConvertersError

            if instruction['name'] == 'measure':
                bit = instruction['clbits'][0]
                if bit in clbits: # checking that the bit referenced in the instruction it actually belongs to a register
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
                break # we skip to the next instruction

            if 'params' in instruction:
                params = instruction['params']
            else:
                params = []

            if 'conditional_reg' in instruction:
                bit = instruction['conditional_reg'][0]
                if bit in clbits: # checking that the bit referenced in the instruction it actually belongs to a register
                    for k,v in classical_registers.items():
                        if bit in v:
                            reg = k
                            l = len(v)
                            clbit = v.index(bit)
                            condition = (Clbit(ClassicalRegister(l, reg), clbit), 1)
            else:
                condition = None
            
            inst = CircuitInstruction( 
                operation = Instruction(name = instruction['name'],
                                        num_qubits = len(instruction['qubits']),
                                        num_clbits = 0,
                                        params = params,
                                        condition = condition
                                        ),
                qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                clbits = ()
                )
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


def _json_to_cunqac(circuit_dict : dict) -> 'CunqaCircuit':
    """
    Converts a json :py:type:`dict` circuit into a :py:class:`~cunqa.circuit.CunqaCircuit`.

    Args:
        circuit_dict (dict): json with the propper structure for defining a quantum circuit.
    
    Returns:
        An object :py:class:`~cunqa.circuit.CunqaCircuit` with the corresponding instructions and characteristics.
    """
    try:
        cunqac = CunqaCircuit(circuit_dict["num_qubits"], circuit_dict["num_clbits"], circuit_dict["id"])
        cunqac.from_instructions(circuit_dict["instructions"])
        return cunqac
    except Exception as error:
        logger.error(f"Some error occured during transformation from json dict to `cunqa.circuit.CunqaCircuit` [{type(error).__name__}].")
        raise ConvertersError


def _json_to_qasm(circuit_json : dict) -> str:
    return _qc_to_qasm(_json_to_qc(circuit_json))


def _qasm_to_qc(circuit_qasm : str) -> 'QuantumCircuit':
    
    try:
        return QuantumCircuit.from_qasm_str(circuit_qasm)
    except Exception as error:
        logger.error(f" Unable to convert OpenQASM to qiskit.QuantumCircuit [{type(error).__name__}].")
        raise SystemExit
    

def _qasm_to_cunqac(circuit_qasm : str) -> 'CunqaCircuit':
    return _qc_to_cunqac(_qasm_to_qc(circuit_qasm))


def _qasm_to_json(circuit_qasm : str) -> dict:
    return _qc_to_json(_qasm_to_qc(circuit_qasm))

    
def _registers_dict(qc: 'QuantumCircuit') -> "list[dict]":
    """
    Returns a list of two dicts corresponding to the classical and quantum registers of the circuit supplied.

    Args
        qc (qiskit.QuantumCircuit): quantum circuit whose number of registers we want to know

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


    