"""
    Holds the :py:func:`~cunqa.transpile.transpiler` function that translates circuit instructions into native instructions that a certain virtual QPU understands.

    It is important and it is assumed that the circuit that is sent to the virtual QPU for its simulation is transplated into the propper native gates
    and adapted to te backend's topology.

    Once the user has decribed the circuit :py:class:`~cunqa.circuit.CunqaCircuit`, :py:class:`qiskit.QuantumCircuit` or json ``dict``,
    :py:mod:`cunqa` provides two alternatives for transpiling it accordingly to a certain backend:

        - When submmiting the circuit, set `transpile` as ``True`` and provide the rest of transpilation instructions:

            >>> qpu.run(circuit, transpile = True, ...)

          This option is ``False`` by default.

        - Use :py:func:`transpiler` function before sending the circuit:

            >>> circuit_transpiled = transpiler(circuit, backend = qpu.backend)
            >>> qpu.run(circuit_transpiled)

    .. warning::
        If the circuit is not transpiled, errors will not raise, but the output of the simulation will not be coherent.
    
"""
from cunqa.backend import Backend
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.converters import convert
from cunqa.logger import logger

from qiskit import QuantumCircuit
from qiskit.qasm2 import dumps
from qiskit.qasm2.exceptions import QASM2Error
from qiskit.exceptions import QiskitError
from qiskit.transpiler.exceptions import TranspilerError
from qiskit.transpiler.preset_passmanagers import generate_preset_pass_manager
from qiskit.providers.models import BackendConfiguration
from qiskit.providers.backend_compat import convert_to_target


class TranspileError(Exception):
    """Exception for error during the transpilation of a circuit to a given :py:class:`~cunqa.backend.Backend`. """
    pass

def transpiler(circuit, backend, opt_level = 1, initial_layout = None):
    """
    Function to transpile a circuit according to a given :py:class:`~cunqa.backend.Backend`.
    The circuit is returned in the same format as it was originally.

    Transpilation instructions are `opt_level`, which defines how optimal is the transpilation, default is ``1``; `initial_layout`
    specifies the set of "real" qubits to which the quantum registers of the circuit are assigned.
    These instructions are associated to the `qiskit.transpiler.passmanager.StagedPassManager <https://quantum.cloud.ibm.com/docs/es/api/qiskit/qiskit.transpiler.StagedPassManager>`_,
    since it is used in the process.

    Args:
        circuit (dict | qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit): circuit to be transpiled.

        backend (~cunqa.backend.Backend): backend which transpilation will be done respect to.

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

            if circuit.has_cc or circuit.has_qc:
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
    
        elif isinstance(circuit, str):
            qc = QuantumCircuit.from_qasm_str(circuit)

        else:
            logger.error(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or dict, but {type(circuit)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level

    except QASM2Error as error:
        logger.error(f"Error with QASM2 string, please check that the sintex is correct [{type(error).__name__}]: {error}.")
        raise SystemExit # User's level
    
    except  QiskitError as error:
        logger.error(f"Error with QASM2 string, please check that the logic of the resulting circuit is correct [{type(error).__name__}]: {error}.")
        raise SystemExit # User's level
    
    except Exception as error:
        logger.error(f"Some error occurred, please check sintax and logic of the resulting circuit [{type(error).__name__}]: {error}")
        raise SystemExit # User's level
        

    # backend check
    if isinstance(backend, Backend):
        configuration = backend.__dict__
    else:
        logger.error(f"Transpilation backend must be <class 'python.backend.Backend'> [{TypeError.__name__}].")
        raise SystemExit # User's level
    
    # transpilation
    try:
        #TODO: Revise the hardcoded args
        args = {
            "backend_name": configuration["name"],
            "backend_version": configuration["version"],
            "n_qubits":configuration["n_qubits"],
            "basis_gates": configuration["basis_gates"],
            "gates":[], # might not work
            "local":False,
            "simulator":False if configuration["simulator"] == "QMIO" else True,
            "conditional":True, 
            "open_pulse":False, #TODO: another simulator distinct from Aer might suppor open pulse.
            "memory":True,
            "max_shots":100000,
            "coupling_map":configuration["coupling_map"]
        }

        backend_configuration = BackendConfiguration(**args)
        target =  convert_to_target(backend_configuration)
        pm = generate_preset_pass_manager(optimization_level = opt_level, target = target, initial_layout = initial_layout)
        qc_transpiled = pm.run(qc)
    
    except KeyError as error:
        logger.error(f"Error in cofiguration of the backend, some keys are missing [{type(error).__name__}].")
        raise SystemExit # User's level
    
    except TranspilerError as error:
        logger.error(f"Error during transpilation: {error}")
        raise SystemExit
    
    except Exception as error:
        logger.error(f"Some error occurred with configuration of the backend, please check that the formats are correct [{type(error).__name__}].")
        raise SystemExit # User's level

    # converting to input format and returning
    if isinstance(circuit, str):
        return dumps(qc_transpiled)
    
    elif isinstance(circuit, QuantumCircuit):
        return qc_transpiled
    
    elif isinstance(circuit, dict):
        return convert(qc_transpiled, "dict")
    
    elif isinstance(circuit, CunqaCircuit):
        return convert(qc_transpiled, "CunqaCircuit")

    
