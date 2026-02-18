"""
    Contains map-like callables to distribute circuits among vQPUS.

    **Variational Quantum Algorithms** [#]_ require numerous executions of parametric cirucits, 
    where in each step of the optimization process new parameters are assigned to them. This implies 
    that, after parameters are updated, a new circuit must be created, transpiled and then sent to 
    the quantum processor or simulator. For simplifying this process, we have 
    :py:class:`~QJobMapper` and :py:class:`~QPUCircuitMapper` classes. Both classes are conceived to 
    be used with Scipy optimizers [#]_ as the *workers* argument in global optimizers.

    - :py:class:`~QJobMapper` takes a list of existing :py:class:`~cunqa.qjob.QJob` 
      objects, then, the class can be called passing a set of **parameters** and a **cost function**. 
      This callable updates each existing :py:class:`~cunqa.qjob.QJob` object with such 
      **parameters** through the :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` method. Then, it 
      gathers the results of the executions and returns the **value of the cost function** for each.
    
    - :py:class:`~QPUCircuitMapper` is instanciated with a circuit and instructions for its 
      execution, together with a list of the :py:class:`~cunqa.qpu.QPU` objects. The difference 
      with :py:class:`~QJobMapper` is that here the method :py:meth:`~cunqa.qpu.QPU.execute` is mapped 
      to each QPU, passing it the circuit with the given parameters assigned so that for this case 
      several :py:class:`~cunqa.qjob.QJob` objects are created.

    Examples utilizing both classes can be found in the `Examples gallery 
    <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_. 
    These examples focus on optimization of VQAs, using a global optimizer called 
    Differential Evolution [#]_.

    *References*:

    .. [#] `Variational Quantum Algorithms arXiv <https://arxiv.org/abs/2012.09265>`_ .

    .. [#] `scipy.optimize documentation <https://docs.scipy.org/doc/scipy/reference/optimize.html>`_.

    .. [#] Differential Evolution initializes a population of inidividuals that evolve from 
       generation to generation in order to collectively find the lowest value of a given cost 
       function. This optimizer has shown great performance for VQAs 
       [`Reference <https://arxiv.org/abs/2303.12186>`_]. It is well implemented in Scipy through the 
       `scipy.optimize.differential_evolution <https://docs.scipy.org/doc/scipy/reference/generated/
       scipy.optimize.differential_evolution.html#scipy.optimize.differential_evolution>`_ 
       function.
"""
from cunqa.logger import logger
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.qpu import QPU, run
from cunqa.qjob import QJob

from qiskit import QuantumCircuit
from qiskit.exceptions import QiskitError
from typing import  Optional, Union, Any

class QJobMapper:
    """
    Class to map the method :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` to a set of jobs sent to 
    virtual QPUs.

    The core of the class is on its :py:meth:`~cunqa.mappers.QJobMapper.__call__` method, to which 
    parameters that the method :py:meth:`~cunqa.qjob.QJob.upgrade_parameters` takes are passed 
    together with a cost function, so that a the value for this cost for each initial 
    :py:class:`~cunqa.qjob.QJob` is returned.

    An example is shown below, once we have a list of :py:class:`~cunqa.qjob.QJob` objects as 
    *qjobs*:

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

    We intuitively see how convenient this class can be for optimization algorithms: one has a 
    parametric circuit to which updated sets of parameters can be sent, getting back the value of the 
    cost function. Examples applied to optimizations are shown at the 
    `Examples gallery <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_.

    Attributes: 
        qjobs (:py:class:`~cunqa.qjob.QJob`): list of objects to be mapped.

    .. automethod:: __call__

    """
    qjobs: list[QJob]

    def __init__(self, qjobs: list[QJob]):
        self.qjobs = qjobs

    def __call__(self, func, population):
        """
        Callable method to map the function *func* to the results of assigning *population* to the 
        given jobs. Regarding the *population*, each set of parameters will be assigned to each 
        :py:class:`~cunqa.qjob.QJob` object, so the list must have size (*N,p*), being *N* the 
        lenght of :py:attr:`~cunqa.mappers.QJobMapper.qjobs` and *p* the number of parameters in the 
        circuit. Mainly, this is thought for the function to take a :py:class:`~cunqa.result.Result` 
        object and to return a value. For example, the function can evaluate the expected value of 
        an observable from the output of the circuit.

        Args:
            func (callable): function to be passed to the results of the jobs. 

            population (list[list[int | float] | np.array[int | float]]): list of numpy vectors to 
            be mapped to the jobs.
            
        Return:
            List of outputs of the function applied to the results of each job for the given 
            population.
        """
        qjobs_ = []
        for qjob, params in zip(self.qjobs, population):
            qjob.upgrade_parameters(list(params))
            qjobs_.append(qjob)
        results = gather(qjobs_) # we only gather the qjobs we upgraded.
        return [func(result) for result in results]


class QPUCircuitMapper:
    """
    Class to map the method :py:meth:`~cunqa.qpu.QPU.execute` to a list of QPUs.

    The class is initialized with a list of :py:class:`~cunqa.qpu.QPU` objects associated to the 
    virtual QPUs that the optimization will require, together with the circuit and the simulation 
    instructions needed for its execution.

    Then, its :py:meth:`~cunqa.mappers.QPUCircuitMapper.__call__` method takes a set of parameters 
    as *population* to assign to the circuit. Each assembled circuit is sent to each virtual QPU 
    with the instructions provided on the instatiation of the mapper. The method returns the value 
    for the provided function *func* for the result of each simulation.

    Its use is pretty similar to :py:class:`~cunqa.mappers.QJobMapper`, though creating 
    :py:class:`~cunqa.qjob.QJob` objects ahead is not needed.

    >>> qpus = get_QPUs(...)
    >>>
    >>> # Creating the mapper with the pre-defined parametric circuit and other simulation instructions.
    >>> mapper = QPUCircuitMapper(qpus, circuit, shots = 1000, ...)
    >>>
    >>> # Defining the parameters set according to the number of parameters
    >>> # of the circuit and the number of QJobs in the list.
    >>> new_parameters = [...]
    >>> 
    >>> # Defining the cost function passed to the result of each QJob
    >>> def cost_function(result):
    >>>     counts = result.counts
    >>>     ...
    >>>     return cost_value
    >>> 
    >>> cost_values = mapper(cost_function, new_parameters)

    For each call of the mapper, circuits are assembled, jobs are sent, results are gathered and 
    cost values are calculated. Its implementation for optimization problems is shown at the 
    `Examples gallery <https://cesga-quantum-spain.github.io/cunqa/_examples/Optimizers_II_mapping.html>`_.

    Attributes: 
        qpus (list[:py:class:`~cunqa.qpu.QPU`]): Objects linked to the virtual QPUs to wich the 
                                                 circuit is mapped.
        circuit (QuantumCircuit): Circuit to which parameters are assigned at the 
                                  :py:meth:`QPUCircuitMapper.__call__` method.
        run_parameters (Optional[Any]) : Any other run instructions needed for the simulation.

    .. automethod:: __call__

    """
    qpus: list[QPU]
    circuit: QuantumCircuit
    run_parameters: Optional[Any]

    def __init__(
        self, 
        qpus: list[QPU], 
        circuit: Union[dict, QuantumCircuit, CunqaCircuit], 
        **run_parameters: Any
    ):
        """
        Class constructor.

        Args:
            qpus (list[~cunqa.qpu.QPU]): list of objects linked to the virtual QPUs intended to be 
                                         used.

            circuit (dict | ~cunqa.circuit.CunqaCircuit | qiskit.QuantumCirucit): circuit to be run 
                                                                                  in the QPUs.

            **run_parameters : any other simulation instructions.

        """
        self.qpus = qpus

        # TODO: Check when parameters get updated
        if isinstance(circuit, QuantumCircuit):
            self.circuit = circuit
        else:
            raise TypeError(f"Parametric circuit must be <class "
                            f"'qiskit.circuit.quantumcircuit.QuantumCircuit'>, but {type(circuit)} "
                            f"was provided [{TypeError.__name__}].")
        
        self.run_parameters = run_parameters

    def __call__(self, func, population):
        """
        Callable method to map the function *func* to the results of the circuits sent to the given 
        QPUs after assigning them *population*. Regarding the *population*, each set of parameters 
        will be assigned to each circuit, so the list must have size (*N,p*), being *N* the lenght 
        of :py:attr:`~cunqa.mappers.QJobMapper.qpus` and *p* the number of parameters in the circuit.
        Mainly, this is thought for the function to take a :py:class:`~cunqa.result.Result` object 
        and to return a value. For example, the function can evaluate the expected value of an 
        observable from the output of the circuit.

        Args:
            func (func): function to be mapped to the QPUs. It must take as argument a 
                         :py:class:`~cunqa.result.Result` instance.

            params (list[list[float | int]]): population of vectors to be mapped to the circuits 
                                              sent to the QPUs.

        Return:
            List of the results of the function applied to the output of the circuits sent to the 
            QPUs.
        """

        qjobs = []
        try:
            for i, params in enumerate(population):
                qpu = self.qpus[i % len(self.qpus)]
                circuit_assembled = self.circuit.assign_parameters(params)
                qjobs.append(run(circuit_assembled, qpu, **self.run_parameters))
            results = gather(qjobs)
            return [func(result) for result in results]
        except QiskitError as error:
            raise RuntimeError(f"Error while assigning parameters to Qiskit's QuantumCircuit: {error}.")