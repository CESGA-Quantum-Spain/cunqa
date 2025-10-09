"""
    Contains map-like callables to distribute circuits in virtual QPUS, needed when communications among circuits are present.

    Submitting circuits with classical or quantum communications
    ---------------------------------------------------------

    When having classical or quantum communications among circuits, method :py:func:`~cunqa.qpu.QPU.run` is obsolete, circuits must be sent as an ensemble in order to ensure correct functioning of the communication protocols.

    So, once the virtual QPUs that allow the desired type of communications are raised and circuits have been defined using :py:class:`~cunqa.circuit.CunqaCircuit`,
    they can be sent using the :py:meth:`~run_distributed` function:

    >>> circuit1 = CunqaCircuit(1, "circuit_1")
    >>> circuit2 = CunqaCircuit(1, "circuit_2")
    >>> circuit1.h(0)
    >>> circuit1.measure_and_send(0, "circuit_2")
    >>> circuit2.remote_c_if("x", 0, "circuit_1")
    >>>
    >>> qpus = get_QPUs()
    >>>
    >>> qjobs = run_distributed([circuit1, circuit2], qpus)
    >>> results = gather(qjobs)


    Mapping circuits to virtual QPUs in optimization problems
    ---------------------------------------------------------

    **Variational Quantum Algorithms** [#]_ require from numerous executions of parametric cirucits, while in each step of the optimization process new parameters are assigned to them.
    This implies that, after parameters are updated, a new circuit must be created, transpiled and then sent to the quantum processor or simulator.
    For simplifying this process, we have :py:class:`~QJobMapper` and :py:class:`~QPUCircuitMapper` classes. Both classes are thought to be used for Scipy optimizers [#]_ as the *workers* argument in global optimizers.

    QJobMapper
    ==========
    :py:class:`~QJobMapper` takes a list of existing :py:class:`~cunqa.qjob.QJob` objects, then, the class can be called passing a set of **parameters** and a **cost function**.
    This callable updates the each existing :py:class:`~cunqa.qjob.QJob` object with such **parameters** through the :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` method.
    Then, it gathers the results of the executions and returns the **value of the cost function** for each.
    
    QPUCircuitMapper
    ===============
    On the other hand, :py:class:`~QPUCircuitMapper` is instanciated with a circuit and instructions for its execution, toguether with a list of the :py:class:`~cunqa.qpu.QPU` objects.
    The difference with :py:class:`~QJobMapper` is that here the method :py:meth:`~cunqa.qpu.QPU.run` is mapped to each QPU, passing it the circuit with the given parameters assigned so that for this case several :py:class:`~cunqa.qjob.QJob` objects are created.

    Examples utilizing both classes can be found in the `Examples gallery <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_. These examples focus on optimization of VQAs, using a global optimizer called Differential Evolution [#]_.

    References:
    ~~~~~~~~~~~

    .. [#] `Variational Quantum Algorithms arXiv <https://arxiv.org/abs/2012.09265>`_ .

    .. [#] `scipy.optimize documentation <https://docs.scipy.org/doc/scipy/reference/optimize.html>`_.

    .. [#] Differential Evolution initializes a population of inidividuals that evolute from generation to generation in order to collectively find the lowest value to a given cost function. This optimizer has shown great performance for VQAs [`Reference <https://arxiv.org/abs/2303.12186>`_]. It is well implemented at Scipy by the `scipy.optimize.differential_evolution <https://docs.scipy.org/doc/scipy/reference/generated/scipy.optimize.differential_evolution.html#scipy.optimize.differential_evolution>`_ function.



"""
from cunqa.logger import logger
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.qpu import QPU
from cunqa.qjob import QJob

from qiskit import QuantumCircuit
from qiskit.exceptions import QiskitError
from qiskit.qasm2 import QASM2Error
import numpy as np
from typing import  Optional, Union, Any

import time
import copy


def run_distributed(circuits: "list[Union[dict, 'CunqaCircuit']]", qpus: "list['QPU']", **run_args: Any):
    """
    Function to send circuits to serveral virtual QPUs allowing classical or quantum communications among them. 
    Each circuit will be sent to each QPU in order, therefore both lists must be of the same size.

    Because the function is destined for executions that require communications, only :py:class:`~cunqa.circuit.CunqaCircuit` or instruction sets are accepted.
    The arguments provided will be the same for all :py:class:`~cunqa.qjob.QJob` objects created.
    
    .. warning::
        If *transpile*, *initial_layout* or *opt_level* are passed as *run_args* they will be ignored since for the current version
        transpilation is not supported when communications are present.

    Args:
        circuits (list[dict] | list[~cunqa.circuit.CunqaCircuit]): circuits to be run.

        qpus (list[~cunqa.qpu.QPU]): QPU objects associated to the virtual QPUs in which the circuits want to be run.
    
        run_args: any other run arguments and parameters.

    Return:
        List of :py:class:`~cunqa.qjob.QJob` objects.
    """

    distributed_qjobs = []
    circuit_jsons = []

    remote_controlled_gates = ["measure_and_send", "remote_c_if", "recv", "qsend", "qrecv", "expose", "rcontrol"]
    correspondence = {}

    #Check wether the circuits are valid and extract jsons
    for circuit in circuits:
        if isinstance(circuit, CunqaCircuit):
            info_circuit_copy = copy.deepcopy(circuit.info) # To modify the info without modifying the attribute info of the circuit
            circuit_jsons.append(info_circuit_copy)

        elif isinstance(circuit, dict):
            circuit_jsons.append(circuit)
        else:
            logger.error(f"Objects of the list `circuits` must be  <class 'cunqa.circuit.CunqaCircuit'> or jsons, but {type(circuit)} was given. [{TypeError.__name__}].")
            raise SystemExit # User's level
    
    #check wether there are enough qpus and create an allocation dict that for every circuit id has the info of the QPU to which it will be sent
    if len(circuit_jsons) > len(qpus):
        logger.error(f"There are not enough QPUs: {len(circuit_jsons)} circuits were given, but only {len(qpus)} QPUs [{ValueError.__name__}].")
        raise SystemExit # User's level
    else:
        if len(circuit_jsons) < len(qpus):
            logger.warning("More QPUs provided than the number of circuits. Last QPUs will remain unused.")
        for circuit, qpu in zip(circuit_jsons, qpus):
            correspondence[circuit["id"]] = qpu._id
        

    # Check whether the QPUs are valid
    # TODO: check only makes sense if we have selected mpi option at compilation time. For the moment it remains commented.
    # if not all(qpu._family == qpus[0]._family for qpu in qpus):
    #     logger.debug(f"QPUs of different families were provided.")
    #     if not all(re.match(r"^tcp://", qpu._endpoint) for qpu in qpus):
    #         names = set()
    #         for qpu in qpus:
    #             names.add(qpu._family)
    #         logger.error(f"QPU objects provided are from different families ({list(names)}). For this version, classical communications beyond families are only supported with zmq communication type.")
    #         raise SystemExit # User's level
    
    logger.debug(f"Run arguments provided for simulation: {run_args}")
    
    #translate circuit ids in comm instruction to qpu endpoints
    for circuit in circuit_jsons:
        for instr in circuit["instructions"]:
            if instr["name"] in remote_controlled_gates:
                instr["qpus"] =  [correspondence[instr["circuits"][0]]]
                instr.pop("circuits")
        for i in range(len(circuit["sending_to"])):
            circuit["sending_to"][i] = correspondence[circuit["sending_to"][i]]
        circuit["id"] = correspondence[circuit["id"]]

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
        distributed_qjobs.append(qpu.run(circuit, **run_parameters))

    return distributed_qjobs


class QJobMapper:
    """
    Class to map the method :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` to a set of jobs sent to virtual QPUs.

    The core of the class is on its :py:meth:`~cunqa.mappers.QJobMapper.__call__` method, to which parameters that the method :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` takes are passed toguether with a cost function, so that a the value for this cost for each initial :py:class:`~cunqa.qjob.QJob` is returned.

    An example is shown below, once we have a list of :py:class:`~cunqa.qjob.QJob` objects as *qjobs*:

    >>> mapper = QJobMapper(qjobs)
    >>>
    >>> # defining the parameters set accordingly to the number of parameters
    >>> # of the circuit and the number of QJobs in the list.
    >>> new_parameters = [...]
    >>> 
    >>> # defining the cost function passed to the result of each QJob
    >>> def cost_function(result):
    >>>     counts = result.counts
    >>>     ...
    >>>     return cost_value
    >>> 
    >>> cost_values = mapper(cost_function, new_parameters)

    We intuitively see how convinient this class can be for optimization algorithms: one has a parametric circuit to which updated sets of parameters can be sent
    obtaining the value of the cost back. Examples applied to optimizations are shown at the `Examples gallery <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_.

    """
    qjobs: "list['QJob']" #: List of jobs that are mapped.

    def __init__(self, qjobs: "list['QJob']"):
        """
        Class constructor.

        Args:
            qjobs (list[~cunqa.qjob.QJob]): list of :py:class:`~cunqa.qjob.QJob` objects to be mapped.

        """
        self.qjobs = qjobs
        logger.debug(f"QJobMapper initialized with {len(qjobs)} QJobs.")

    def __call__(self, func, population):
        """
        Callable method to map the function *func* to the results of assigning *population* to the given jobs.
        Regarding the *population*, each set of parameters will be assigned to each :py:class:`~cunqa.qjob.QJob` object, so the list must
        have size (*N,p*), being *N* the lenght of :py:attr:`~cunqa.mappers.QJobMapper.qjobs` and *p* the number of parameters in the circuit.
        Mainly, this is thought for the function to take a :py:class:`~cunqa.result.Result` object and to return a value.
        For example, the function can evaluate the expected value of an observable from the output of the circuit.

        Args:
            func (callable): function to be passed to the results of the jobs. 

            population (list[list[int | float]]): list of vectors to be mapped to the jobs.
            
        Return:
            List of outputs of the function applied to the results of each job for the given population.
        """
        qjobs_ = []
        for i, params in enumerate(population):
            qjob = self.qjobs[i]
            logger.debug(f"Uptading params for QJob {qjob}...")
            qjob.upgrade_parameters(params.tolist())
            qjobs_.append(qjob)

        logger.debug("About to gather results ...")
        results = gather(qjobs_) # we only gather the qjobs we upgraded.

        return [func(result) for result in results]


class QPUCircuitMapper:
    """
    Class to map the method :py:meth:`~cunqa.qpu.QPU.run` to a list of QPUs.

    The class is initialized with a list of :py:class:`~cunqa.qpu.QPU` objects associated to the virtual QPUs that are intended to work with, toguether
    with the circuit and the simulation instructions needed for its execution.

    Then, its :py:meth:`~cunqa.mappers.QPUCircuitMapper.__call__` method takes a set of parameters as *population* to assing to the circuit.
    Each assembled circuit is sent to each virtual QPU with the instructions provided on the instatiation of the mapper.
    The method returns the value for the provided function *func* for the result of each simulation.

    Its use is pretty similar to :py:class:`~cunqa.mappers.QJobMapper`, but not needing to create the :py:class:`~cunqa.qjob.QJob` objects ahead:

    >>> qpus = get_QPUs(...)
    >>>
    >>> # creating the mapper with the pre-defined parametric circuit and other simulation instructions.
    >>> mapper = QPUCircuitMapper(qpus, circuit, shots = 1000, ...)
    >>>
    >>> # defining the parameters set accordingly to the number of parameters
    >>> # of the circuit and the number of QJobs in the list.
    >>> new_parameters = [...]
    >>> 
    >>> # defining the cost function passed to the result of each QJob
    >>> def cost_function(result):
    >>>     counts = result.counts
    >>>     ...
    >>>     return cost_value
    >>> 
    >>> cost_values = mapper(cost_function, new_parameters)

    For each call of the mapper, circuits are assembled, jobs are sent, results are gathered and cost values are calculated.
    Its implementation for optimization problems is shown at the `Examples gallery <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_.

    """
    qpus: "list['QPU']" #: :py:class:`~cunqa.qpu.QPU` ibjects linked to the virtual QPUs to wich the circuit is mapped.
    circuit: 'QuantumCircuit' #: Circuit to which parameters are assigned at the :py:meth:`QPUCircuitMapper.__call__` method.
    transpile: Optional[bool] #: Weather transpilation is wanted to be done before sending each circuit.
    initial_layout: Optional["list[int]"] #: Transpilation information, qubits of the backend to which the qubits of the circuit are mapped.
    run_parameters: Optional[Any] #: Any other run instructions needed for the simulation.

    def __init__(self, qpus: "list['QPU']", circuit: Union[dict,'QuantumCircuit','CunqaCircuit'], transpile: Optional[bool] = False, initial_layout: Optional["list[int]"] = None, **run_parameters: Any):
        """
        Class constructor.

        Args:
            qpus (list[~cunqa.qpu.QPU]): list of objects linked to the virtual QPUs intended to be used.

            circuit (dict | ~cunqa.circuit.CunqaCircuit | qiskit.QuantumCirucit): circuit to be run in the QPUs.

            transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

            initial_layout (list[int]): Initial position of virtual qubits on physical qubits for transpilation.

            **run_parameters : any other simulation instructions.

        """
        self.qpus = qpus

        if isinstance(circuit, QuantumCircuit):
            self.circuit = circuit

        else:
            logger.error(f"Parametric circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, but {type(circuit)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level

        self.transpile = transpile
        self.initial_layout = initial_layout
        self.run_parameters = run_parameters
        
        logger.debug(f"QPUMapper initialized with {len(qpus)} QPUs.")

    def __call__(self, func, population):
        """
        Callable method to map the function *func* to the results of the circuits sent to the given QPUs after assigning them *population*.
        Regarding the *population*, each set of parameters will be assigned to each circuit, so the list must
        have size (*N,p*), being *N* the lenght of :py:attr:`~cunqa.mappers.QJobMapper.qpus` and *p* the number of parameters in the circuit.
        Mainly, this is thought for the function to take a :py:class:`~cunqa.result.Result` object and to return a value.
        For example, the function can evaluate the expected value of an observable from the output of the circuit.

        Args:
            func (func): function to be mapped to the QPUs. It must take as argument the an object <class 'qjob.Result'>.

            params (list[list[float | int]]): population of vectors to be mapped to the circuits sent to the QPUs.

        Return:
            List of the results of the function applied to the output of the circuits sent to the QPUs.
        """

        qjobs = []

        try:
            tick = time.time()
            for i, params in enumerate(population):
                qpu = self.qpus[i % len(self.qpus)]
                circuit_assembled = self.circuit.assign_parameters(params)

                logger.debug(f"Sending QJob to QPU {qpu._id}...")
                qjobs.append(qpu.run(circuit_assembled, transpile = self.transpile, initial_layout = self.initial_layout, **self.run_parameters))

            
            logger.debug(f"About to gather {len(qjobs)} results ...")
            results = gather(qjobs)
            tack = time.time()
            median = sum([res.time_taken for res in results])/len(results)

            print("Mean simulation time: ", median)
            print("Total distribution and gathering time: ", tack-tick)

            return [func(result) for result in results]

        
        except QiskitError as error:
            logger.error(f"Error while assigning parameters to QuantumCircuit, please check they have the correct size [{type(error).__name__}]: {error}.")
            raise SystemExit # User's level
        
        except Exception as error:
            logger.error(f"Some error occurred with the circuit [{type(error).__name__}]: {error}")
            raise SystemExit # User's level