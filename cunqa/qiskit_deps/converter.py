"""
    Holds functions for converting circuits into the different formats: :py:class:`qiskit.QuantumCircuit`, :py:class:`~cunqa.circuit.CunqaCircuit` and json :py:class:`dict`.

    There is the general :py:func:`convert` function, that identifies the input format and transforms according to the format desired by the variable *convert_to*.

    .. warning::
        It is not possible to convert circuits with classical or quantum communications instructions into :py:class:`qiskit.QuantumCircuit`
        since these are not supported by this format. It one tries, an error will be raised.
"""
from qiskit import QuantumCircuit
from cunqa.circuit.helpers import generate_id
from cunqa.logger import logger

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
        
        quantum_registers = {}
        qinit = 0
        for qr in qc.qregs:
            quantum_registers[qr.name] = list(range(qinit, qinit + qr.size))
            qinit += qr.size

        classical_registers = {}
        cinit = 0
        for cr in qc.cregs:
            classical_registers[cr.name] = list(range(cinit, cinit + cr.size))
            cinit += cr.size

        logger.debug(f"Localized quamtum registers: {quantum_registers}\n Localized classical registers: {classical_registers}")
        
        json_data = {
            "id": "QuantumCircuit_" + generate_id(),
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
                raise ValueError(f"Instruction {instruction.operation.name} not supported for conversion.")

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
                    raise ValueError("if_else instruction with \'else\' case is not supported for the current version.")
                else:
                    sub_circuit = [sub_circuit for sub_circuit in instruction.operation.params if sub_circuit is not None][0]

                if instruction.condition[1] not in [1]:
                    raise ValueError("Only 1 is accepted as condition for classicaly contorlled operations for the current version.")
                
                for re in qc.qregs:
                    sub_circuit.add_register(re)

                sub_instructions = qc_to_json(sub_circuit)["instructions"]

                for sub_instruction in sub_instructions:
                    json_data["instructions"].append(sub_instruction)
                
            elif (instruction.operation._condition != None):

                if instruction.operation._condition[1] not in [1]:
                    raise ValueError("Only 1 is accepted as condition for classicaly contorlled operations for the current version.")

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
        logger.error(f"Some error occurred during transformation from `qiskit.QuantumCircuit` to json dict [{type(error).__name__}].")
        raise error