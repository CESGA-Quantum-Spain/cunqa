import json
from cunqa.logger import logger
from cunqa.qjob import gather
from cunqa.circuit import from_json_to_qc, CunqaCircuit
from cunqa.qpu import QPU
from qiskit import QuantumCircuit
from qiskit.exceptions import QiskitError
from qiskit.qasm2 import QASM2Error
import numpy as np



def run_distributed(circuits, qpus, **run_args):
    """
    Method to send circuits to serveral QPUs allowing classical communications among them. 
    
    Each circuit will be sent to the given QPUs in order, therefore both lists must be of the same size.

    For a more advanced and personalized mapping see <class 'cunqa.mappers.QJobMapper'> and <class 'cunqa.mappers.QPUCircuitMapper'>
    described below, but these will not suppor classical communications.

    If `transpile`, `initial_layout` or `opt_level` are passed as **run_args they will be ignored because for the initial version
    transpilation is not supported. The arguments provided will be the same for the all `QJobs` created.

    Args:
    ---------
    circuits (list[json dict,  <class 'cunqa.circuit.CunqaCircuit'>]): circuits to be run.

    qpus (list[<class 'cunqa.qpu.QPU'>]): QPU objects associated to the virtual QPUs in which the circuits want to be run.
    
    **run_args: any other run arguments and parameters.

    Return:
    ---------
    List of <class `cunqa.qjob.QJobs`> objects.
    """
    
    distributed_qjobs = []
    circuit_jsons = []

    #Check wether the circuits are valid and extract jsons
    for circuit in circuits:
        if not circuit.is_distributed:
            logger.error(f"Circuits to run must be distributed.")
            raise SystemExit # User's level
        
        if isinstance(circuit, CunqaCircuit):
            circuit_jsons.append(circuit.cunqa_info)
        elif isinstance(circuit, json):
            circuit_jsons.append(circuit)
        else:
            logger.error(f"Objects of the list `circuits` must be  <class 'cunqa.circuit.CunqaCircuit'> or jsons, but {type(circuit)} was given. [{TypeError.__name__}].")
            raise SystemExit # User's level

    #Check wether the QPUs are valid
    if not all(qpu._family_name == qpus[0]._family_name for qpu in qpus):
        names = set()
        for qpu in qpus:
            names.add(qpu._family_name)
        logger.error(f"QPU objects provided are from different families ({list(names)}). For this version, classical communications beyond families are not supported.")
        raise SystemExit # User's level
    
    logger.debug(f"Run arguments provided for simulation: {run_args}")
    
    warn = False
    run_parameters = {}
    for k,v in run_args.items():
        if k == "transpile" or k == "initial_layout" or k == "opt_level":
            if not warn:
                logger.warning("Transpilation instructions are not supported for this version. Default `transpilation=False` is set.")
        else:
            run_parameters[k] = v

    # no need to capture errors bacuse they are captured at `QPU.run`
    for circuit, qpu in zip(circuits, qpus):
        distributed_qjobs.append(qpu.run(circuit, **run_parameters))

    return distributed_qjobs


class QJobMapper:
    """
    Class to map the method `QJob.upgrade_parameters` to a list of QJobs.
    """
    def __init__(self, qjobs):
        """
        Initializes the QJobMapper class.

        Args:
        ---------
        qjobs (list[<class 'qjob.QJob'>]): list of QJobs to be mapped.

        """
        self.qjobs = qjobs
        logger.debug(f"QJobMapper initialized with {len(qjobs)} QJobs.")

    def __call__(self, func, population):
        """
        Callable method to map the function to the given QJobs.

        Args:
        ---------
        func (func): function to be mapped to the QJobs. It must take as argument the an object <class 'qjob.Result'>.

        population (list[list]): population of vectors to be mapped to the QJobs.

        Return:
        ---------
        List of results of the function applied to the QJobs for the given population.
        """
        qjobs_ = []
        for i, params in enumerate(population):
            qjob = self.qjobs[i]
            logger.debug(f"Uptading params for QJob {qjob} in QPU {qjob._QPU.id}...")
            qjob.upgrade_parameters(params.tolist())
            qjobs_.append(qjob)

        logger.debug("About to gather results ...")
        results = gather(qjobs_) # we only gather the qjobs we upgraded.

        return [func(result) for result in results]


class QPUCircuitMapper:
    """
    Class to map the function `qpu.QPU.run()` to a list of QPUs.
    """
    def __init__(self, qpus, ansatz, transpile = False, initial_layout = None, **run_parameters):
        """
        Initializes the QPUCircuitMapper class.

        Args:
        ---------
        qpus (list[<class 'qpu.QPU'>]): list of QPU objects.

        circuit (json dict, <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or QASM2 str): circuit to be run in the QPUs.

        transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

        initial_layout (list[int]): Initial position of virtual qubits on physical qubits for transpilation.

        **run_parameters : any other simulation instructions.

        """
        self.qpus = qpus

        if isinstance(ansatz, QuantumCircuit):
            self.ansatz = ansatz

        else:
            logger.error(f"Parametric circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, but {type(ansatz)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level

        self.transpile = transpile
        self.initial_layout = initial_layout
        self.run_parameters = run_parameters
        
        logger.debug(f"QPUMapper initialized with {len(qpus)} QPUs.")

    def __call__(self, func, population):
        """
        Callable method to map the function to the QPUs.

        Args:
        ---------
        func (func): function to be mapped to the QPUs. It must take as argument the an object <class 'qjob.Result'>.

        params (list[list]): population of vectors to be mapped to the circuits sent to the QPUs.

        Return:
        ---------
        List of the results of the function applied to the output of the circuits sent to the QPUs.
        """

        qjobs = []

        try:
            for i, params in enumerate(population):
                qpu = self.qpus[i % len(self.qpus)]
                circuit_assembled = self.ansatz.assign_parameters(params)

                logger.debug(f"Sending QJob to QPU {qpu.id}...")
                qjobs.append(qpu.run(circuit_assembled, transpile = self.transpile, initial_layout = self.initial_layout, **self.run_parameters))

            
            logger.debug(f"About to gather {len(qjobs)} results ...")
            results = gather(qjobs)

            return [func(result) for result in results]

        
        except  QiskitError as error:
            logger.error(f"Error while assigning parameters to QuantumCircuit, please check they have the correct size [{type(error).__name__}]: {error}.")
            raise SystemExit # User's level
        
        except Exception as error:
            logger.error(f"Some error occurred with the circuit [{type(error).__name__}]: {error}")
            raise SystemExit # User's level