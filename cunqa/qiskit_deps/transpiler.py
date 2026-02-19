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
from typing import Union
from sympy import Symbol
import copy

from cunqa.qiskit_deps.cunqabackend import CunqaBackend # simulator (qjob.py), para transpilar (qpu.py), instanciacion (qutils.py)
from cunqa.qpu import Backend
from cunqa.logger import logger
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.parameter import Param
from cunqa.circuit.ir import to_ir


from qiskit import QuantumCircuit, transpile
from qiskit.transpiler import TranspilerError
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit, CircuitError, Parameter, ParameterExpression


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
                raise TypeError(f"initial_layout must be of the size of the circuit, {circuit.num_qubits}, while it is {len(initial_layout)}.")
            
            qc = circuit

        elif isinstance(circuit, CunqaCircuit):
            # if circuit.has_cc or circuit.has_qc:
            #     raise TypeError(f"CunqaCircuit with distributed instructions was provided, for which transpilation is not avaliable at the moment.")
            
            qc = _from_ir_to_qc(circuit.info)

        elif isinstance(circuit, dict):
            if initial_layout is not None and len(initial_layout) != circuit['num_qubits']:
                raise TypeError(f"initial_layout must be of the size of the circuit, {circuit.num_qubits}, while it is {len(initial_layout)}.")
            
            qc = _from_ir_to_qc(circuit)

        else:
            raise TypeError(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, <class 'cunqa.circuit.core.CunqaCircuit'> or ir dict, but {type(circuit)} was provided.")
    
    except Exception as error:
        raise error 
        
    logger.debug("Circuit converted to QuantumCircuit.")


    # transpilation
    try:
        cunqabackend = CunqaBackend(backend = backend)
        qc_transpiled = transpile(qc, cunqabackend, initial_layout = initial_layout, optimization_level = opt_level, seed_transpiler = seed)
    
    except TranspilerError as error:
        logger.error(f"Some error occured with transpilation.")
        raise error

    except Exception as error:
        logger.error(f"Some error occurred with transpilation, please check that the target QPU is adequate for the provided circuit (enough number of qubits, simulator supports instructions, etc): {error} [{type(error).__name__}].")
        raise error 
     
    
    # converting to input format and returning

    if isinstance(circuit, QuantumCircuit):
        return qc_transpiled
    
    elif isinstance(circuit, dict):
        return to_ir(qc_transpiled)
    
    elif isinstance(circuit, CunqaCircuit):

        dict_transpiled = to_ir(qc_transpiled)

        cunqac_transpiled = CunqaCircuit(dict_transpiled["num_qubits"], dict_transpiled["num_clbits"], id = circuit._id + "_transpiled")
        cunqac_transpiled.add_instructions(dict_transpiled["instructions"])

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

    # extract key information from ir dict
    try:
        instructions = circuit_dict['instructions']
        num_qubits = circuit_dict['num_qubits']
        num_clbits = circuit_dict["num_clbits"]
        quantum_registers = circuit_dict['quantum_registers']
        classical_registers = circuit_dict['classical_registers']

    except KeyError as error:
        logger.error(f"Circuit json not correct, requiered keys must be: 'instructions', 'num_qubits', 'num_clbits', 'quantum_resgisters' and 'classical_registers' [{type(error).__name__}].")
        raise error
        
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


    # processing instructions

    for instruction in copy.deepcopy(instructions):

        instruction_name = instruction['name']
        instruction_qubits = instruction.get("qubits", None)
        instruction_clbits = instruction.get("clbits", None)
        instruction_params = instruction.get("params", [])

        # instanciating instruction's classical and quantum bits

        qiskit_Clbit = []; qiskit_Qubit = []

        if (instruction_clbits is not None) and (len(instruction_clbits) != 0):

            for inst_clbit in instruction_clbits:
                for k,v in classical_registers.items():
                    if inst_clbit in v:
                        qiskit_Clbit.append(Clbit(ClassicalRegister(len(v),k), v.index(inst_clbit)))

        if (instruction_qubits is not None) and (len(instruction_qubits) != 0):
            for inst_qubit in instruction_qubits:
                for k,v in quantum_registers.items():
                    if inst_qubit in v:
                        qiskit_Qubit.append(Qubit(QuantumRegister(len(v),k), v.index(inst_qubit)))

        # processing params: Param, value or instructions for subcircuits in cif instruction

        qiskit_params = []; qiskit_cif_subcircs = []

        for param in instruction_params:

            if isinstance(param, Param):

                symbol_map = {Parameter(symbol.name): symbol for symbol in param.variables}
                qiskit_paramexp = ParameterExpression(symbol_map, param.expr)

                qiskit_params.append(qiskit_paramexp)

            elif isinstance(param, float) or isinstance(param, int):

                qiskit_params.append(param)

            elif isinstance(param, dict):

                qiskit_cif_subcircs.append(_from_ir_to_qc({"instructions":[param],
                                                    "num_qubits":num_qubits,
                                                    "num_clbits":num_clbits,
                                                    "classical_registers":classical_registers,
                                                    "quantum_registers":quantum_registers}).data[0])

            else:
                logger.error("Instruction params not supported in qiskit.QuantumCircuit.")
                raise TypeError


        # processing of the instruction itself

        if  instruction_name == "measure":

            for qubit,clbit in zip(instruction_qubits, instruction_clbits):
                qc.measure(qubit,clbit)

        elif instruction_name == "unitary":

            qc.unitary(instruction.get("elements", []), qiskit_Qubit)

        elif instruction_name == "cif":

            with qc.if_test((qiskit_Clbit, 1)) as else_:
                qc.append(qiskit_cif_subcircs)

        elif instruction_name in SUPPORTED_QISKIT_OPERATIONS:

            qiskit_operation = Instruction(name = instruction_name,
                                            num_qubits = len(qiskit_Qubit),
                                            num_clbits = len(qiskit_Clbit),
                                            params = qiskit_params)
            
            qiskit_instruction = CircuitInstruction(operation = qiskit_operation,
                                                    qubits = qiskit_Qubit,
                                                    clbits = qiskit_Clbit
                                                    )
            qc.append(qiskit_instruction)
            
        else:
            logger.error(f"Instruction {instruction_name} not supported in qiskit.QuantumCircuit.")
            raise ValueError
    
    return qc
