"""
    Holds functions for converting circuits into the different formats: :py:class:`qiskit.QuantumCircuit`, :py:class:`cunqa.circuit.CunqaCircuit` and json :py:class:`dict`.

    There is the general :py:func:`convert` function, that identifies the input format and transforms according to the format desired by the variable *convert_to*.

    On the other hand, there are other functions to transform between specific formats:

        - :py:class:`~cunqa.circuit.CunqaCircuit` **↔** :py:class:`qiskit.QuantumCircuit` **:** :py:func:`cunqac_to_qc`, :py:func:`qc_to_cunqac`.
        - :py:class:`~cunqa.circuit.CunqaCircuit` **↔** :py:class:`dict` **:** :py:func:`cunqac_to_json`, :py:func:`json_to_cunqac`.
        - :py:class:`qiskit.QuantumCircuit` **↔** :py:class:`dict` **:** :py:func:`qc_to_json`, :py:func:`json_to_qc`.

    .. warning::
        It is not possible to convert circuits with classical or quantum communications instructions into :py:class:`qiskit.QuantumCircuit`
        since these are not supported by this format. It one tries, an error will be raised.
"""


from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit, Parameter, ParameterExpression
import qiskit._accelerate.circuit # Handles Symbolic expressions for ParameterExpressions

#SymbolExpr = qiskit._accelerate.circuit.ParameterExpression

from typing import Tuple, Union, Optional
import sympy
import copy

from cunqa.circuit.circuit import CunqaCircuit
from cunqa.circuit.parameter import Variable
from cunqa.logger import logger


def convert(circuit : Union['QuantumCircuit', 'CunqaCircuit', dict], convert_to : str) -> Union['QuantumCircuit', 'CunqaCircuit', dict]:
    """
        Function to convert a quantum circuit to the desired format.
        Detects the intup format and transforms into the one specified by *convert_to*, that can be ``"QuantumCircuit`` for :py:class:`qiskit.QuantumCircuit`,
        ``"CunqaCircuit"`` for :py:class:`~cunqa.circuit.CunqaCircuit` and ``"json"`` for a json :py:class:`dict`.

        Args:
            circuit (qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit | dict): circuit to be transformed.
        
        Returns:
            The circuit in the desired format accordingly to *convert_to*.

    """
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
    Transforms a :py:class:`qiskit.QuantumCircuit` to json :py:class:`dict`.

    Args:
        qc (qiskit.QuantumCircuit): circuit to transform to json.

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
            "is_parametric": _is_parametric(qc),
            "is_dynamic": False,
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }
        if _is_parametric(qc):
            json_data["current_params"] = []
            json_data["param_expressions"] = {"sympy_exprs": [], "lambda_funcs": []}

        for instruction in copy.deepcopy(qc.data):
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
                # Process instructions to avoid unhandled Parameters 
                params = instruction.params
                for i, param in enumerate(params):
                    if isinstance(param, Parameter):

                        var = Variable(str(param))
                        json_data["current_params"].append({var: None})
                        json_data["param_expressions"]["sympy_exprs"].append(var)
                        json_data["param_expressions"]["lambda_funcs"].append(lambda x: x)
                        params[i]= var

                    elif isinstance(param, ParameterExpression):

                        expr = sympy.sympify(sympy.parsing.sympy_parser.parse_expr(str(param)))
                        json_data["current_params"].append({symbol: None for symbol in expr.free_symbols})
                        json_data["param_expressions"]["sympy_exprs"].append(expr)
                        json_data["param_expressions"]["lambda_funcs"].append(sympy.lambdify(tuple(expr.free_symbols), expr, 'numpy'))
                        params[i]= expr

                    else:
                        json_data["current_params"].append(param)
                        json_data["param_expressions"]["sympy_exprs"].append(None)
                        json_data["param_expressions"]["lambda_funcs"].append(None)
                        # No modification on params needed



                if (instruction.operation._condition != None):
                    json_data["is_dynamic"] = True
                    json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":params,
                                                "conditional_reg":[instruction.operation._condition[0]._index]
                                                })
                else:
                    json_data["instructions"].append({"name":instruction.name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg, qubit)],
                                                "params":params
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
    """
    Converts a :py:class:`qiskit.QuantumCircuit` into a :py:class:`~cunqa.circuit.CunqaCircuit`.

    Args:
        qc (qiskit.QuantumCircuit): object that defines the quantum circuit.
    Returns:
        The corresponding :py:class:`~cunqa.circuit.CunqaCircuit` with the propper instructions and characteristics.
    """
    return json_to_cunqac(qc_to_json(qc))

def cunqac_to_json(cunqac : 'CunqaCircuit') -> dict:
    """
    Converts a :py:class:`~cunqa.circuit.CunqaCircuit` into a json :py:type:`dict` circuit.

    Args:
        cunqac (~cunqa.circuit.CunqaCircuit): object that defines the quantum circuit.

    Returns:
        The corresponding json :py:type:`dict` circuit with the propper instructions and characteristics.
    """
    return cunqac.info

def cunqac_to_qc(cunqac : 'CunqaCircuit') -> 'QuantumCircuit':
    """
    Converts a :py:class:`~cunqa.circuit.CunqaCircuit` into a :py:class:`qiskit.QuantumCircuit`.

    Args:
        cunqac (~cunqa.circuit.CunqaCircuit): object that defines the quantum circuit.

    Returns:
        The corresponding :py:class:`qiskit.QuantumCircuit` with the propper instructions and characteristics.
    """
    return json_to_qc(cunqac_to_json(cunqac))

def json_to_cunqac(circuit_dict : dict) -> 'CunqaCircuit':
    """
    Converts a json :py:type:`dict` circuit into a :py:class:`~cunqa.circuit.CunqaCircuit`.

    Args:
        circuit_dict (dict): json with the propper structure for defining a quantum circuit.
    
    Returns:
        An object :py:class:`~cunqa.circuit.CunqaCircuit` with the corresponding instructions and characteristics.
    """
    cunqac = CunqaCircuit(circuit_dict["num_qubits"], circuit_dict["num_clbits"], circuit_dict["id"])
    cunqac.from_instructions(circuit_dict["instructions"])

    return cunqac

def json_to_qc(circuit_dict: dict) -> 'QuantumCircuit':
    """
    Function to transform a circuit in json dict format to :py:class:`qiskit.QuantumCircuit`.

    Args:
        circuit_dict (dict): circuit instructions to be transformed.

    Return:
        :py:class:`qiskit.QuantumCircuit` with the given instructions.
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

        param_counter = 0
        parameter_tracker = {} # No two Parameter instances with the same name can be created or FAILURE will occur qhen adding them to the circuit 
        for instruction in copy.deepcopy(instructions):
            if instruction['name'] != 'measure':
                if 'params' in instruction:
                    params = instruction['params']
                    
                    if "param_expressions" in circuit:
                        for i in range(len(params)):
                            expr = circuit["param_expressions"]["sympy_exprs"][param_counter + i]
                            if expr is None:
                                continue

                            elif isinstance(expr, Variable):
                                if not str(expr) in parameter_tracker:
                                    parameter_tracker[str(expr)] = Parameter(str(expr)) # Create Parameters only once and reuse them all other times

                                params[i] = parameter_tracker[str(expr)]

                            elif get_module(expr) == "sympy":
                                parameter_tracker |= {str(sym): Parameter(str(sym)) for sym in expr.free_symbols if str(sym) not in parameter_tracker} # Create and add any new Parameters
                                params[i] = ParameterExpression({parameter_tracker[str(sym)]: sym for sym in expr.free_symbols}, expr)

                    param_counter += len(params)

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

def get_module(obj):
    """ Returns the root module that the passed object is from."""
    if not hasattr(obj, '__module__'):
        return
    return obj.__module__.split('.')[0]