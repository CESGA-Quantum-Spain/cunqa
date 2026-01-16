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
from cunqa.qiskit_deps.converter import convert
from cunqa.logger import logger

from qiskit import QuantumCircuit, transpile
from qiskit.transpiler import TranspilerError


def transpiler(circuit, backend, opt_level = 1, initial_layout = None, seed = None):
    """
    Function to transpile a circuit according to a given :py:class:`~cunqa.qpu.QPU`.
    The circuit is returned in the same format as it was originally.

    Transpilation instructions are `opt_level`, which defines how optimal is the transpilation, default is ``1``; `initial_layout`
    specifies the set of "real" qubits to which the quantum registers of the circuit are assigned.
    These instructions are associated to the `qiskit.transpiler.compiler.transpile <https://quantum.cloud.ibm.com/docs/api/qiskit/1.2/compiler#qiskit.compiler.transpile>`_,
    since it is used in the process.

    Args:
        circuit (dict | qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit): circuit to be transpiled.

        qpu (~cunqa.qpu.QPU): backend which transpilation will be done respect to.

        opt_level (int): optimization level for creating the `qiskit.transpiler.passmanager.StagedPassManager`. Default set to 1.

        initial_layout (list[int]): initial position of virtual qubits on physical qubits for transpilation, lenght must be equal to the number of qubits in the circuit.
    """

    # converting to QuantumCircuit
    try:

        if isinstance(circuit, QuantumCircuit):
            if initial_layout is not None and len(initial_layout) != circuit.num_qubits:
                logger.error(f"initial_layout must be of the size of the circuit: {circuit.num_qubits} [{TypeError.__name__}].")
                raise SystemExit # User's level
            else:
                qc = circuit

        elif isinstance(circuit, CunqaCircuit):

            if circuit.is_dynamic:
                logger.error(f"CunqaCircuit with distributed instructions was provided, transpilation is not avaliable at the moment. Make sure you are using a cunqasimulator backend, then transpilation is not necessary [{TypeError.__name__}].")
                raise SystemExit
            else:
                qc = convert(circuit, "QuantumCircuit")

        elif isinstance(circuit, dict):
            if initial_layout is not None and len(initial_layout) != circuit['num_qubits']:
                logger.error(f"initial_layout must be of the size of the circuit: {circuit.num_qubits} [{TypeError.__name__}].")
                raise SystemExit # User's level
            else:
                qc = convert(circuit, "QuantumCircuit")

        else:
            logger.error(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, <class 'cunqa.circuit.circuit.CunqaCircuit'> or dict, but {type(circuit)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level
    
    except Exception as error:
        logger.error(f"Some error occurred, please check sintax and logic of the resulting circuit [{type(error).__name__}]: {error}")
        raise SystemExit # User's level
        

    # qpu check
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
        return convert(qc_transpiled, "CunqaCircuit")
