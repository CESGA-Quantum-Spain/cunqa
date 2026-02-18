"""
    Holds the :py:func:`~cunqa.qiskit_deps.transpile.transpiler` function that translates circuit instructions into native instructions that a certain virtual QPU understands.

    It is important and it is assumed that the circuit that is sent to the virtual QPU for its simulation is transplated into the propper native gates
    and adapted to te backend's topology.

    Once the user has decribed the circuit :py:class:`~cunqa.circuit.CunqaCircuit`, :py:class:`qiskit.QuantumCircuit` or json ``dict``,
    :py:mod:`cunqa` provides two alternatives for transpiling it accordingly to a certain virtual QPU's backend:

        - When submmiting the circuit, set `transpile` as ``True`` and provide the rest of transpilation instructions:

            >>> qpu.run(circuit, transpile = True, ...)

          This option is ``False`` by default.

        - Use :py:func:`transpiler` function before sending the circuit:

            >>> circuit_transpiled = transpiler(circuit, target_qpu = qpu)
            >>> qpu.run(circuit_transpiled)

    .. warning::
        If the circuit is not transpiled, errors will not raise, but the output of the simulation will not be coherent.
    
"""

from cunqa.qiskit_deps.cunqabackend import CunqaBackend # simulator (qjob.py), para transpilar (qpu.py), instanciacion (qutils.py)
from cunqa.backend import Backend
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.parameter import Variable
from cunqa.logger import logger
import copy

from qiskit import QuantumCircuit, transpile
from qiskit.transpiler import TranspilerError

from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit, CircuitError, Parameter, ParameterExpression

from typing import Union


def transpiler(circuit, backend, opt_level = 1, initial_layout = None, seed = None) -> Union['CunqaCircuit', dict, 'QuantumCircuit']:
    """
    Function to transpile a circuit according to a given :py:class:`~cunqa.qpu.QPU`.
    The circuit is returned in the same format as it was originally.

    Transpilation instructions are `opt_level`, which defines how optimal is the transpilation, default is ``1``; `initial_layout`
    specifies the set of "real" qubits to which the quantum registers of the circuit are assigned.
    These instructions are associated to the `qiskit.transpiler.compiler.transpile <https://quantum.cloud.ibm.com/docs/api/qiskit/1.2/compiler#qiskit.compiler.transpile>`_,
    since it is used in the process.

    Args:
        circuit (dict | qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit): circuit to be transpiled.

        backend (~cunqa.qpu.Backend): backend which transpilation will be done respect to.

        opt_level (int): optimization level for creating the `qiskit.transpiler.passmanager.StagedPassManager`. Default set to 1.

        initial_layout (list[int]): initial position of virtual qubits on physical qubits for transpilation, lenght must be equal to the number of qubits in the circuit.
    
        seed (int): transpilation seed.
    """

    logger.debug("Converting to QuantumCircuit...")
    
    try:

        if isinstance(circuit, QuantumCircuit):
            if initial_layout is not None and len(initial_layout) != circuit.num_qubits:
                logger.error(f"initial_layout must be of the size of the circuit: {circuit.num_qubits} [{TypeError.__name__}].")
                raise SystemExit # User's level
            
            qc = circuit

        elif isinstance(circuit, CunqaCircuit):
            if circuit.has_cc or circuit.has_qc:
                logger.error(f"CunqaCircuit with distributed instructions was provided, transpilation is not avaliable at the moment. Make sure you are using a cunqasimulator backend, then transpilation is not necessary [{TypeError.__name__}].")
                raise SystemExit
            
            current_params = circuit.current_params
            qc = convert(circuit.info, "QuantumCircuit")

        elif isinstance(circuit, dict):
            if initial_layout is not None and len(initial_layout) != circuit['num_qubits']:
                logger.error(f"initial_layout must be of the size of the circuit: {circuit['num_qubits']} [{TypeError.__name__}].")
                raise SystemExit # User's level
            
            qc = convert(circuit, "QuantumCircuit")

        else:
            logger.error(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, <class 'cunqa.circuit.circuit.CunqaCircuit'> or dict, but {type(circuit)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level
    
    except Exception as error:
        logger.error(f"Some error occurred, please check sintax and logic of the resulting circuit [{type(error).__name__}]: {error}")
        raise SystemExit # User's level
        
    logger.debug("Circuit converted to QuantumCircuit")

    # backend check
    if isinstance(backend, Backend):
        cunqabackend = CunqaBackend(backend = backend)
    else:
        logger.error(f"backend must be <class 'cunqa.backend.Backend'>, but {type(backend)} was provided [{TypeError.__name__}].")
        raise SystemExit # User's level
    
    # transpilation
    try:
        qc_transpiled = transpile(qc, cunqabackend, initial_layout = initial_layout, optimization_level = opt_level, seed_transpiler = seed)
    
    except TranspilerError as error:
        logger.error(f"Some error occured with transpilation: {error} [TranspilerError]")

    except Exception as error:
        logger.error(f"Some error occurred with transpilation, please check that the target QPU is adequate for the provided circuit (enough number of qubits, simulator supports instructions, etc): {error} [{type(error).__name__}].")
        raise SystemExit # User's level

    # converting to input format and returning
    if isinstance(circuit, QuantumCircuit):
        return qc_transpiled
    
    elif isinstance(circuit, dict):
        return convert(qc_transpiled, "dict")
    
    elif isinstance(circuit, CunqaCircuit):
        cunqac_transpiled = convert(qc_transpiled, "CunqaCircuit")
        cunqac_transpiled._id = circuit._id + "_transpiled"
        
        assign_dict={}
        for expr in current_params:
            if isinstance(expr, dict):
                if None in list(expr.values()):
                    continue
                assign_dict.update(expr)

        cunqac_transpiled.assign_parameters(assign_dict)

        return cunqac_transpiled



SUPPORTED_QISKIT_OPERATIONS = {'unitary','ryy', 'rz', 'z', 'p', 'rxx', 'rx', 'cx', 'id', 'x', 'sxdg', 'u1', 'ccy', 'rzz', 'rzx', 'ry', 's', 'cu', 'crz', 'ecr', 't', 'ccx', 'y', 'cswap', 'r', 'sdg', 'csx', 'crx', 'ccz', 'u3', 'u2', 'u', 'cp', 'tdg', 'sx', 'cu1', 'swap', 'cy', 'cry', 'cz','h', 'cu3', 'measure', 'if_else', 'barrier', 'reset'}


def _from_ir_to_qc(circuit_dict: dict) -> QuantumCircuit:
    """
    Function to transform a circuit from CUNQA's intermidiate representation to :py:class:`qiskit.QuantumCircuit`.

    Instructions refering to communication directives are not yet supported for :py:class:`qiskit.QuantumCircuit`.

    Args:
        circuit_dict (dict): circuit instructions to be transformed.

    Return:
        :py:class:`qiskit.QuantumCircuit` with the given instructions.
    """

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

        # localizing qubits and clbits of the circuit
        circuit_qubits = []
        for qr, lista in quantum_registers.items():
            for i in lista: 
                circuit_qubits.append(i)
            qc.add_register(QuantumRegister(len(lista), qr))

        circuit_clbits = []
        for cr, lista in classical_registers.items():
            for i in lista: 
                circuit_clbits.append(i)
            qc.add_register(ClassicalRegister(len(lista), cr))

        param_counter = 0
        parameter_tracker = {} # No two Parameter instances with the same name can be created or FAILURE will occur when adding them to the circuit 
        for instruction in copy.deepcopy(instructions):
            params = []
            if instruction['name'] != 'measure':
                if 'params' in instruction:
                    params = instruction['params']
                    
                    if "param_expressions" in circuit_dict:
                        for i in range(len(params)):
                            expr = circuit_dict["param_expressions"][param_counter + i]
                            if expr is None:
                                continue

                            elif isinstance(expr, Variable):
                                if not str(expr) in parameter_tracker:
                                    parameter_tracker[str(expr)] = Parameter(str(expr)) # Create Parameters only once and reuse them all other times

                                params[i] = parameter_tracker[str(expr)]

                            elif _get_module(expr) == "sympy":
                                parameter_tracker |= {str(sym): Parameter(str(sym)) for sym in expr.free_symbols if str(sym) not in parameter_tracker} # Create and add any new Parameters
                                params[i] = ParameterExpression({parameter_tracker[str(sym)]: sym for sym in expr.free_symbols}, expr)

                    param_counter += len(params)

            if instruction['name'] not in SUPPORTED_QISKIT_OPERATIONS:
                logger.error(f"Instruction {instruction['name']} not supported for conversion [ValueError].")
                raise ConvertersError

            # instanciating instruction's classical and quantum bits

            inst_Clbit = []; inst_Qubit = []

            if ("clbits" in instruction) and (len(instruction['clbits']) != 0):
                for inst_clbit in instruction["clbits"]:
                    for k,v in classical_registers.items():
                        if inst_clbit in v:
                            inst_Clbit.append(Clbit(ClassicalRegister(len(v),k), v.index(inst_clbit)))

            if ("qubits" in instruction) and (len(instruction["qubits"]) != 0):
                for inst_qubit in instruction["qubits"]:
                    for k,v in quantum_registers.items():
                        if inst_qubit in v:
                            inst_Qubit.append(Qubit(QuantumRegister(len(v),k), v.index(inst_qubit)))

            inst_operation = Instruction(name = instruction['name'],
                                        num_qubits = len(inst_Qubit),
                                        num_clbits = len(inst_Clbit),
                                        params = params
                                        )
            
            # checking for conditional operations

            if 'conditional_reg' in instruction:
                inst_conditional_reg = instruction['conditional_reg'][0]
                for k,v in classical_registers.items():
                        if inst_conditional_reg in v:
                            inst_operation._condition = (Clbit(ClassicalRegister(len(v),k), v.index(inst_conditional_reg)), 1)
            
            # adding instruction

            inst = CircuitInstruction( 
                operation = inst_operation,
                qubits = inst_Qubit,
                clbits = inst_Clbit
                )
            
            qc.append(inst)
        
        return qc
        
    except KeyError as error:
        logger.error(f"Some error with the keys of `instructions` occured, please check the format [{type(error).__name__}].")
        raise ConvertersError
    
    except TypeError as error:
        logger.error(f"Error when reading instructions, check that the given elements have the correct type [{type(error).__name__}].")
        raise ConvertersError
    
    except IndexError as error:
        logger.error(f"Error with format for classical_registers [{type(error).__name__}].")
        raise ConvertersError
    
    except CircuitError as error:
        logger.error(f"Error in construction of the QuantumCircuit object [{type(error).__name__}].")
        raise ConvertersError

    except Exception as error:
        logger.error(f"Error when converting json dict to QuantumCircuit [{type(error).__name__}].")
        raise ConvertersError
    

def _get_module(obj):
    """ Returns the root module that the passed object is from."""
    if not hasattr(obj, '__module__'):
        return
    return obj.__module__.split('.')[0]