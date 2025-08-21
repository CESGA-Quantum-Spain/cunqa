from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit

from typing import Tuple, Union, Optional

from cunqa.circuit import CunqaCircuit
from cunqa.logger import logger


def convert(circuit : Union['QuantumCircuit', 'CunqaCircuit', dict], convert_to : str) -> Union['QuantumCircuit', 'CunqaCircuit', dict]:
    if isinstance(circuit, QuantumCircuit):
        if convert_to == "QuantumCircuit":
            logger.warning("Provided circuit was already a QuantumCircuit")
            converted_circuit = circuit
        elif convert_to == "CunqaCircuit":
            converted_circuit = qc_to_cunqac(circuit)
        elif convert_to == "json":
            converted_circuit = qc_to_json(circuit)
        else:
            logger.error(f"Unable to convert circuit to {convert_to}")
            converted_circuit = circuit
    elif isinstance(circuit, CunqaCircuit):
        if convert_to == "QuantumCircuit":
            converted_circuit = cunqac_to_qc(circuit)
        elif convert_to == "CunqaCircuit":
            logger.warning("Provided circuit was already a CunqaCircuit")
            converted_circuit = circuit
        elif convert_to == "json":
            converted_circuit = cunqac_to_json(circuit)
        else:
            logger.error(f"Unable to convert circuit to {convert_to}")
            converted_circuit = circuit
    elif isinstance(circuit, dict):
        if convert_to == "QuantumCircuit":
            converted_circuit = json_to_qc(circuit)
        elif convert_to == "CunqaCircuit":
            converted_circuit = json_to_cunqac(circuit)
        elif convert_to == "json":
            logger.warning("Provided circuit was already a CunqaCircuit")
            converted_circuit = circuit
        else:
            logger.error(f"Unable to convert circuit to {convert_to}")
            converted_circuit = circuit
    else:
        logger.error(f"Provided circuit must be a QuantumCircuit, a CunqaCircuit or a json")
        converted_circuit = circuit
    
    return converted_circuit



def qc_to_json(qc : 'QuantumCircuit') -> dict:
    """
    Transforms a ``qiskit.circuit.QuantumCircuit`` to json dict.

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
            "is_dynamic": False,
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }
        for instruction in qc.data:
            qreg = [r._register.name for r in instruction.qubits]
            qubit = [q._index for q in instruction.qubits]
            
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
                    

        return json_data 
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from QuantumCircuit to json dict [{type(error).__name__}].")
        raise error


def qc_to_cunqac(qc : 'QuantumCircuit') -> 'CunqaCircuit':
    return json_to_cunqac(qc_to_json(qc)) 


def cunqac_to_json(cunqac : 'CunqaCircuit') -> dict:
    circuit_json = {}
    circuit_json["id"] = cunqac._id
    circuit_json["is_parametric"] = cunqac.is_parametric
    circuit_json["is_dynamic"] = cunqac.is_dynamic
    circuit_json["num_qubits"] = cunqac.num_qubits
    circuit_json["num_clbits"] = cunqac.num_clbits
    circuit_json["quantum_registers"] = cunqac.quantum_regs
    circuit_json["classical_registers"] = cunqac.classical_regs
    circuit_json["instructions"] = cunqac.instructions

    return circuit_json


def cunqac_to_qc(cunqac : 'CunqaCircuit') -> 'QuantumCircuit':
    return json_to_qc(cunqac_to_json(cunqac))


def json_to_cunqac(circuit_dict : dict) -> 'CunqaCircuit':
    cunqac = CunqaCircuit(circuit_dict["num_qubits"], circuit_dict["num_clbits"], circuit_dict["id"])
    cunqac.from_instructions(circuit_dict["instructions"])

    return cunqac


def json_to_qc(circuit_dict: dict) -> 'QuantumCircuit':
    """
    Function to transform a circuit in json dict format to `qiskit.circuit.QuantumCircuit <https://quantum.cloud.ibm.com/docs/es/api/qiskit/qiskit.circuit.QuantumCircuit>`_.

    Args:
        circuit_dict (dict): circuit to be transformed to ``qiskit.circuit.QuantumCircuit``.

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
    Returns a list of two dicts corresponding to the classical and quantum registers of the circuit supplied.

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