from __future__ import annotations
from functools import singledispatch
import copy

from qiskit import QuantumCircuit
from qiskit.circuit import Parameter, ParameterExpression


from cunqa.circuit import CunqaCircuit
from cunqa.circuit.helpers import generate_id
from cunqa.logger import logger

SUPPORTED_QISKIT_OPERATIONS = {
    'unitary','ryy', 'rz', 'z', 'p', 'rxx', 'rx', 'cx', 'id', 'x', 'sxdg', 'u1', 
    'ccy', 'rzz', 'rzx', 'ry', 's', 'cu', 'crz', 'ecr', 't', 'ccx', 'y', 'cswap', 
    'r', 'sdg', 'csx', 'crx', 'ccz', 'u3', 'u2', 'u', 'cp', 'tdg', 'sx', 'cu1', 
    'swap', 'cy', 'cry', 'cz','h', 'cu3', 'measure', 'if_else', 'barrier', 'reset'
}

@singledispatch
def to_ir(circuit: object) -> dict:
    meth = getattr(circuit, "to_ir", None)
    if callable(meth):
        return meth()

    raise TypeError(
        f"Not a method to convert {type(circuit).__name__} to dict."
    )

@to_ir.register
def _(c: CunqaCircuit) -> dict:
    return copy.deepcopy(c.info)

@to_ir.register
def _(c: dict) -> dict:
    logger.warning("Circuit is already in IR format, returning it as is.")
    return c

@to_ir.register
def _(c: QuantumCircuit) -> dict:
    """
    Transforms a `qiskit.QuantumCircuit` to json `dict`.

    Args:
        c (qiskit.QuantumCircuit): circuit to transform to json.

    Return:
        Json dict with the circuit information.
    """
    quantum_registers = {}
    qinit = 0
    for qr in c.qregs:
        quantum_registers[qr.name] = list(range(qinit, qinit + qr.size))
        qinit += qr.size

    classical_registers = {}
    cinit = 0
    for cr in c.cregs:
        classical_registers[cr.name] = list(range(cinit, cinit + cr.size))
        cinit += cr.size
    
    json_data = {
        "id": "QuantumCircuit_" + generate_id(),
        "is_dynamic": False,
        "instructions":[],
        "sending_to":[],
        "num_qubits":sum([q.size for q in c.qregs]),
        "num_clbits": sum([c.size for c in c.cregs]),
        "quantum_registers": quantum_registers,
        "classical_registers": classical_registers, 
        "params":[]
    }

    for instruction in c.data:
        if instruction.operation.name not in SUPPORTED_QISKIT_OPERATIONS:
            raise ValueError(f"Instruction {instruction.operation.name} not supported for conversion.")

        qreg = [r._register.name for r in instruction.qubits]
        qubit = [q._index for q in instruction.qubits]
        
        clreg = [r._register.name for r in instruction.clbits]
        bit = [b._index for b in instruction.clbits]

        if instruction.operation.name == "barrier":
            pass

        elif instruction.operation.name == "measure":
            json_data["instructions"].append({
                "name":instruction.operation.name,
                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                "clbits":[classical_registers[k][b] for k,b in zip(clreg, bit)]
            })

        elif instruction.operation.name == "unitary":
            json_data["instructions"].append({
                "name":instruction.operation.name, 
                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                "params":[[list(map(lambda z: [z.real, z.imag], row)) 
                           for row in instruction.operation.params[0].tolist()]]
            })

        elif instruction.operation.name == "if_else":
            json_data["is_dynamic"] = True

            if not any([sub_circuit is None for sub_circuit in instruction.operation.params]):
                raise ValueError("if_else instruction with \'else\' case is not supported for the " \
                                 "current version.")
            else:
                sub_circuit = [
                    sub_circuit for sub_circuit in instruction.operation.params 
                    if sub_circuit is not None
                ][0]

            if instruction.condition[1] not in [1]:
                raise ValueError("Only 1 is accepted as condition for classicaly controlled " \
                                 "operations for the current version.")
            
            for re in c.qregs:
                sub_circuit.add_register(re)

            sub_instructions = to_ir(sub_circuit)["instructions"]

            cc_instruction = {
                "name": "cif",
                "clbits": [classical_registers[k][b] for k,b in zip(clreg, bit)],
                "instructions": sub_instructions
                }
            
            json_data["instructions"].append(cc_instruction)

        else:

            instruction_params = [str(param) if (isinstance(param, Parameter) or isinstance(param, Parameter)) else param for param in instruction.operation.params]
        
            instruction = {"name":instruction.operation.name, 
                           "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                           "params":instruction_params
                          }
            
            if instruction.condition != None:

                if instruction.operation._condition[1] not in [1]:
                    raise ValueError("Only 1 is accepted as condition for classicaly controlled operations for the current version.")

                cc_clbit = classical_registers[instruction.condition[0]._register.name][instruction.condition[0]._index]

                json_data["is_dynamic"] = True
                json_data["instructions"].append({"name":"cif",
                                            "clbits":[cc_clbit],
                                            "params":[instruction]
                                            })
            
            else:
                json_data["instructions"].append(instruction)

    return json_data