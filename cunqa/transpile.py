"""
    Holds our wrapper for the qiskit transpiler.
"""
from cunqa.backend import Backend
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.converters import convert
from cunqa.logger import logger

from qiskit import QuantumCircuit
from qiskit.qasm2.exceptions import QASM2Error
from qiskit.exceptions import QiskitError
from qiskit.transpiler.preset_passmanagers import generate_preset_pass_manager
from qiskit.providers.models import BackendConfiguration
from qiskit.providers.backend_compat import convert_to_target
from qiskit.transpiler.exceptions import TranspilerError
from qiskit.qasm2 import dumps


class TranspileError(Exception):
    """Exception for error during the transpilation of a circuit to a given Backend. """
    pass

def transpiler(circuit, backend, opt_level = 1, initial_layout = None):
    """
    Function to transpile a circuit according to a given backend. Circuit must be qiskit QuantumCircuit, dict or QASM2 string. If QASM2 string is provided, function will also return circuit in QASM2.

    Args:
    -----------
    circuit (dict, <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or QASM2 str): circuit to be transpiled. 

    backend (<class 'backend.Backend'>): backend which transpilation will be done respect to.

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

            if circuit.has_cc:
                logger.error(f"CunqaCircuit with distributed instructions was provided, transpilation is not avaliable at the moment. Make sure you are using a cunqasimulator backend, then transpilation is not necessary [{TypeError.__name__}].")
                raise SystemExit
            else:
                qc = convert(circuit.info, "QuantumCircuit")

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
        return convert(qc_transpiled, "json")
    
    elif isinstance(circuit, CunqaCircuit):
        return convert(qc_transpiled, "CunqaCircuit")

    
