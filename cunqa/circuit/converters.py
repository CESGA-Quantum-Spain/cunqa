"""
    Holds functions for converting circuits into the different formats: :py:class:`qiskit.QuantumCircuit`, :py:class:`~cunqa.circuit.CunqaCircuit` and json :py:class:`dict`.

    There is the general :py:func:`convert` function, that identifies the input format and transforms according to the format desired by the variable *convert_to*.

    .. warning::
        It is not possible to convert circuits with classical or quantum communications instructions into :py:class:`qiskit.QuantumCircuit`
        since these are not supported by this format. It one tries, an error will be raised.
"""


from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit, CircuitError
from qiskit.qasm2 import dumps as dumps2
from qiskit.qasm3 import dumps as dumps3

from typing import Tuple, Union, Optional
from .core import CunqaCircuit
from .helpers import generate_id
from cunqa.logger import logger


class ConvertersError(Exception):
    """Exception for error during conversion between circuit types."""
    pass

SUPPORTED_QISKIT_OPERATIONS = {
    'unitary','ryy', 'rz', 'z', 'p', 'rxx', 'rx', 'cx', 'id', 'x', 'sxdg', 'u1', 
    'ccy', 'rzz', 'rzx', 'ry', 's', 'cu', 'crz', 'ecr', 't', 'ccx', 'y', 'cswap', 
    'r', 'sdg', 'csx', 'crx', 'ccz', 'u3', 'u2', 'u', 'cp', 'tdg', 'sx', 'cu1', 
    'swap', 'cy', 'cry', 'cz','h', 'cu3', 'measure', 'if_else', 'barrier'
}


def qc_to_json(qc : 'QuantumCircuit') -> dict:
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
            "id": "QuantumCircuit_" + generate_id(),
            "is_parametric": _is_parametric(qc),
            "is_dynamic": False,
            "instructions":[],
            "sending_to":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }

        for instruction in qc.data:

            logger.debug(f"Processing instruction: {instruction}")

            if instruction.operation.name not in SUPPORTED_QISKIT_OPERATIONS:
                logger.error(f"Instruction {instruction.operation.name} not supported for conversion [ValueError].")
                raise ConvertersError

            qreg = [r._register.name for r in instruction.qubits]
            qubit = [q._index for q in instruction.qubits]
            
            clreg = [r._register.name for r in instruction.clbits]
            bit = [b._index for b in instruction.clbits]

            if instruction.operation.name == "barrier":
                pass

            elif instruction.operation.name == "measure":

                json_data["instructions"].append({"name":instruction.operation.name,
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "clbits":[classical_registers[k][b] for k,b in zip(clreg, bit)]
                                                })

            elif instruction.operation.name == "unitary":

                json_data["instructions"].append({"name":instruction.operation.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":[[list(map(lambda z: [z.real, z.imag], row)) for row in instruction.operation.params[0].tolist()]] #only difference, it ensures that the matrix appears as a list, and converts a+bj to (a,b)
                                                })
                
            elif instruction.operation.name == "if_else":

                json_data["is_dynamic"] = True

                if not any([sub_circuit is None for sub_circuit in instruction.operation.params]):
                    logger.error("if_else instruction with \'else\' case is not supported for the current version [ValueError].")
                    raise ConvertersError
                else:
                    sub_circuit = [sub_circuit for sub_circuit in instruction.operation.params if sub_circuit is not None][0]

                if instruction.condition[1] not in [1]:
                    logger.error("Only 1 is accepted as condition for classicaly contorlled operations for the current version [ValueError].")
                    raise ConvertersError
                
                for re in qc.qregs:
                    sub_circuit.add_register(re)

                sub_instructions = qc_to_json(sub_circuit)["instructions"]

                for sub_instruction in sub_instructions:
                    json_data["instructions"].append(sub_instruction)
                
            elif (instruction.operation._condition != None):

                if instruction.operation._condition[1] not in [1]:
                    logger.error("Only 1 is accepted as condition for classicaly contorlled operations for the current version [ValueError].")
                    raise ConvertersError

                json_data["is_dynamic"] = True
                json_data["instructions"].append({"name":instruction.operation.name, 
                                            "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                            "params":instruction.operation.params
                                            })                
            
            else:
                json_data["instructions"].append({"name":instruction.operation.name, 
                                            "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                            "params":instruction.operation.params
                                            })
        return json_data
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from `qiskit.QuantumCircuit` to json dict [{type(error).__name__}].")
        logger.error(f"{error}")
        raise error

def cunqa_to_json(cunqac : 'CunqaCircuit') -> dict:
    """
    Converts a :py:class:`~cunqa.circuit.CunqaCircuit` into a json :py:type:`dict` circuit.

    Args:
        cunqac (~cunqa.circuit.CunqaCircuit): object that defines the quantum circuit.

    Returns:
        The corresponding json :py:type:`dict` circuit with the propper instructions and characteristics.
    """
    return cunqac.info

def _is_parametric(circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit']) -> bool:
    """
    Function to determine weather a cirucit has gates that accept parameters, not necesarily parametric :py:class:`qiskit.QuantumCircuit`.
    For example, a circuit that is composed by hadamard and cnot gates is not a parametric circuit; but if a circuit has any of the gates defined in `parametric_gates` we
    consider it a parametric circuit for our purposes.

    Args:
        circuit (qiskit.QuantumCircuit | dict | str): the circuit from which we want to find out if it's parametric.

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