from cunqa.logger import logger
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.qpu import QPU
from qiskit import QuantumCircuit
from qiskit.exceptions import QiskitError
from qiskit.qasm2 import QASM2Error
import numpy as np

import re



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
    circuits (list[json dict, <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or QASM2 str]): circuits to be run.

    qpus (list[<class 'cunqa.qpu.QPU'>]): QPU objects associated to the virtual QPUs in which the circuits want to be run.
    
    **run_args: any other run arguments and parameters.

    Return:
    ---------
    List of <class `cunqa.qjob.QJobs`> objects.
    """

    distributed_qjobs = []
    circuit_jsons = []

    remote_controlled_gates = ["measure_and_send", "remote_c_if_h", "remote_c_if_x","remote_c_if_y","remote_c_if_z","remote_c_if_rx","remote_c_if_ry","remote_c_if_rz","remote_c_if_cx","remote_c_if_cy","remote_c_if_cz", "remote_c_if_ecr"]
    correspondence = {}

    #Check wether the circuits are valid and extract jsons
    for circuit in circuits:
        if (isinstance(circuit, CunqaCircuit) and not circuit.is_distributed):
            logger.error(f"Circuits to run must be distributed.")
            raise SystemExit # User's level
        
        if isinstance(circuit, CunqaCircuit):
            extended_cunqa_info = {"id":circuit._id, "instructions":circuit.instructions, "num_qubits": circuit.num_qubits,"num_clbits": circuit.num_clbits,"classical_registers": circuit.classical_regs,"quantum_registers": circuit.quantum_regs, "exec_type":"dynamic", "is_distributed":circuit.is_distributed, "is_parametric":circuit.is_parametric}
            circuit_jsons.append(extended_cunqa_info)

        elif isinstance(circuit, dict):
            circuit_jsons.append(circuit)
        else:
            logger.error(f"Objects of the list `circuits` must be  <class 'cunqa.circuit.CunqaCircuit'> or jsons, but {type(circuit)} was given. [{TypeError.__name__}].")
            raise SystemExit # User's level
    
    #check wether there are enough qpus and create an allocation dict that for every circuit id has the info of the QPU to which it will be sent
    if len(circuit_jsons)>len(qpus):
        logger.error(f"There are not enough QPUs: {len(circuit_jsons)} circuits were given, but only {len(qpus)} QPUs [{ValueError.__name__}].")
        raise SystemExit # User's level
    else:
        if len(circuit_jsons)<len(qpus):
            logger.warning("More QPUs provided than the number of circuits. Last QPUs will remain unused.")
        for circuit, qpu in zip(circuit_jsons, qpus):
            correspondence[circuit["id"]] = qpu._comm_endpoint
        

    #Check wether the QPUs are valid
    if not all(qpu._family == qpus[0]._family for qpu in qpus):
        logger.debug(f"QPUs of different families were provided.")
        if not all(re.match(r"^tcp://", qpu._comm_endpoint) for qpu in qpus):
            names = set()
            for qpu in qpus:
                names.add(qpu._family)
            logger.error(f"QPU objects provided are from different families ({list(names)}). For this version, classical communications beyond families are only supported with zmq communication type.")
            raise SystemExit # User's level
    
    logger.debug(f"Run arguments provided for simulation: {run_args}")
    
    #translate circuit ids in comm instruction to qpu endpoints
    for circuit in circuit_jsons:
        for instr in circuit["instructions"]:
            if instr["name"] in remote_controlled_gates:
                instr["qpus"] =  [correspondence[instr["circuits"][0]]]
                instr.pop("circuits")
    
    warn = False
    run_parameters = {}
    for k,v in run_args.items():
        if k == "transpile" or k == "initial_layout" or k == "opt_level":
            if not warn:
                logger.warning("Transpilation instructions are not supported for this version. Default `transpilation=False` is set.")
        else:
            run_parameters[k] = v

    # no need to capture errors bacuse they are captured at `QPU.run`
    for circuit, qpu in zip(circuit_jsons, qpus):
        logger.debug(f"The following circuit will be sent: {circuit}")
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

                logger.debug(f"Sending QJob to QPU {qpu._id}...")
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