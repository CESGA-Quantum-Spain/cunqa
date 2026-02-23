"""
    Contains objects that define and manage quantum emulation jobs. The core of this module is the 
    class :py:class:`~cunqa.qjob.QJob`. These objects are created when a quantum job is sent to a 
    vQPU, as a return of the :py:func:`~cunqa.qpu.run` function:

        >>> run(circuit, qpu)
        <cunqa.qjob.QJob object at XXXXXXXX>
        
    In this method, after the :py:class:`~cunqa.qjob.QJob` instance is created, the circuit is 
    submitted for simulation at the vQPU. :py:class:`QJob` is the bridge between sending a 
    circuit with instructions and receiving the results.

    Another functionality described in the submodule is the function :py:func:`~cunqa.qjob.gather`, 
    which receives a list of :py:class:`~QJob` objects and returns their results as 
    :py:class:`~cunqa.result.Result` objects.

        >>> qjob_1 = run(circuit_1, qpu_1)
        >>> qjob_2 = run(circuit_2, qpu_2)
        >>> gather([qjob_1, qjob_2])
        [<cunqa.result.Result object at XXXXXXXX>, <cunqa.result.Result object at XXXXXXXX>]
    """

import json
from typing import  Optional, Any, Union

from cunqa.logger import logger
from cunqa.result import Result
from cunqa.qclient import QClient, FutureWrapper
from sympy import Symbol
from cunqa.circuit.parameter import encoder, Param
from cunqa.real_qpus.qmioclient import QMIOClient, QMIOFuture

class QJob:
    """
    Class to handle jobs sent to vQPUs. A :py:class:`QJob` object is created as the output 
    of the :py:meth:`~cunqa.qpu.run` method. The quantum job not only contains the circuit to 
    be simulated, but also simulation instructions and information of the vQPU to which the job 
    is sent. This class has a main attribute: :py:attr:`QJob.result` which stores the information 
    about the execution. 

    .. autoattribute:: result

    But first, in order to be able to call the attribute :py:attr:`QJob.result`, it is necessary to 
    submit the job for execution. To do so, we will use the method :py:meth:`QJob.submit`.

    .. automethod:: submit

    But the objective of the :py:class:`QJob` class is not only to retrieve the result. It also 
    allows an easy updating of the quantum task sent without the need of resend the whole circuit. 
    This is really useful, especially working with variational quantum algorithms (VQAs) [#]_, which 
    need to change the parameters of the gates in a circuit as they are optimized in each iteration. 
    This parameter update is done using the :py:meth:`~QJob.upgrade_parameters` method.

    .. automethod:: upgrade_parameters

    *References*:

    .. [#] `Variational Quantum Algorithms arXiv <https://arxiv.org/abs/2012.09265>`_ .


    """
    qclient: Union[QClient, QMIOClient]
    _circuit_id: str
    _updated: bool
    _device: dict
    _future: Union[FutureWrapper, QMIOFuture] 
    _result: Optional[Result]
    _quantum_task: dict
    _params: list[Param]

    def __init__(
            self, 
            qclient: Union[QClient, QMIOClient], 
            device: dict, 
            circuit_ir: dict, 
            **run_parameters: Any
    ):
        self._qclient = qclient
        self._device = device
        self._circuit_id = circuit_ir["id"]
        self._cregisters = circuit_ir["classical_registers"]
        self._params = circuit_ir["params"]
        self._updated = False
        self._future = None
        self._result = None

        run_config = {
            "shots": 1024, 
            "method":"automatic", 
            "avoid_parallelization": False,
            "num_clbits": circuit_ir["num_clbits"], 
            "num_qubits": circuit_ir["num_qubits"], 
            "seed": 123123,
            "device": self._device
        }

        if (run_parameters == None) or (len(run_parameters) == 0):
            logger.warning("No run parameters provided, default were set.")
        elif (type(run_parameters) == dict): 
            for k,v in run_parameters.items():
                run_config[k] = v
        else:
            logger.warning("Error when reading `run_parameters`, default were set.")
        
        self._quantum_task = {
            "config": run_config, 
            "instructions": circuit_ir["instructions"],
            "sending_to": circuit_ir["sending_to"],
            "is_dynamic": circuit_ir["is_dynamic"],
            "id": circuit_ir["id"]
        }
      
        logger.debug("Qjob configured")

    @property
    def result(self) -> Result:
        """
        Result of the job. If no error occured during simulation, a :py:class:`~cunqa.result.Result` 
        object is retured.

            >>> qjob = qpu.run(circuit)
            >>> result = qjob.result
            >>> print(result.counts)
            {'00': 524, '11': 500}

        .. note::
            Since to obtain the result the simulation has to be finished, this method is a blocking 
            call, which means that the program will be blocked until the :py:class:`QClient` has 
            recieved from the corresponding server the outcome of the job. The result is not sent 
            from the server to the :py:class:`QClient` until this method is called.
        
        .. warning::
            Because of how the client-server comunication is built, the user must be careful and call 
            for the results in the same order in which the jobs where submited. If the order is not 
            respected, no errors would be raised but results will not correspond to the job -
            a mix up would happen. This is because the server follows the FIFO rule (*First in 
            first out*): if we want to receive the second result, the first one has to be out.

        """
        if self._future is not None:
            if (self._result is not None and not self._updated) or (self._result is None):
                res = self._future.get()
                self._result = Result(
                    json.loads(res), 
                    circ_id=self._circuit_id, 
                    registers=self._cregisters
                )
                self._updated = True
        else:
            raise RuntimeError("self._future is None which means that the QJob has not "
                               "been submitted.")
        return self._result

    def submit(
        self, 
        param_values: Union[dict[Symbol, Union[float, int]], list[Union[float, int]]] = None
    ) -> None:
        """
        Asynchronous method to submit a job to the corresponding :py:class:`QClient`.

            >>> qjob = QJob(qclient, circuit_ir, **run_parameters)
            >>> qjob.submit() # Already has all the info of where and what to send

        In case the circuit is parametric it needs to be called with the value of its free 
        parameters set with the :py:attr:`param_values`.
        
        .. note::
            Opposite to :py:attr:`~cunqa.qjob.QJob.result`, this is a non-blocking call.
            Once a job is submitted, the python program continues without waiting while  
            the corresponding server receives and simulates the circuit.
        
        param_values (dict | list): either a list of ordered parameters to assign to the 
                                    parametrized circuit or a dictionary with keys being the 
                                    free parameters' names and its values being its 
                                    corresponding new values.
        """
        if self._future is not None:
            logger.error("QJob has already been submitted.")
        else:
            if param_values is not None:
                self.assign_parameters_(param_values)
            
            self._future = self._qclient.send_circuit(
                json.dumps(
                    self._quantum_task,
                    default=encoder
                )
            )
            
            logger.debug("Circuit was sent.")
            
    def upgrade_parameters(
        self, 
        param_values: Union[dict[Symbol, Union[float, int]], list[Union[float, int]]]
    ) -> None:
        """
        Method to upgrade the parameters in a previously submitted job of parametric circuit.
        First it checks weather the prior simulation's result was retrieved. If not, it is discarded,
        and the new set of parameters is sent to the server to be reassigned to the circuit for 
        simulation. This method can be used on a loop, carefully saving the intermediate results. 
        Also, this method is used by the class :py:class:`~cunqa.mappers.QJobMapper`, check out its 
        documentation for a extensive description.

        There are two ways of passing new parameters. First, as a **list** with the corresponding 
        values in the order of the gates in the circuit, in which case missing parameters will
        result in an error. On the other hand, as a **dict** where the keys are the free 
        parameters names and the values the corresponding new value to that free parameter. Not 
        all parameters need to be updated, but they must have been given a value at 
        least once, because its last value would be used.

        .. warning::
            Before sending the circuit or upgrading its parameters, the result of the prior job must be 
            called. It can be done manually, so that we can save it and obtain its information, or it 
            can be done automatically as in the example above, but be aware that once the 
            :py:meth:`upgrade_parameters` method is called, this result is discarded.

        Args:
            param_values (dict | list): either a list of ordered parameters to assign to the 
                                        parametrized circuit or a dictionary with keys being the 
                                        free parameters' names and its values being its 
                                        corresponding new values.
        """

        if self._result is None: 
            if self._future is not None:
                # TODO: Improve this by having a queue of results
                logger.warning("You have not obtained the previous results. They will be discarded.")
                self._future.get() # we get the previous result because if not it stays in queue
            else:
                raise RuntimeError("No circuit was sent before calling update_parameters().")

        if not len(param_values):
            raise AttributeError("No parameter list has been provided to the upgrade_parameters "
                                 "method.")

        self.assign_parameters_(param_values)
              
        try:
            premessage = json.dumps(self._params, default=encoder)
            message = """{{"params":{}}}""".format(premessage).replace("'", '"')
            self._future = self._qclient.send_parameters(message)
            self._updated = False
        except Exception as error:
            logger.error(f"Some error occured when sending the new parameters to "
                         f"circuit {self._circuit_id} [{type(error).__name__}].")
            self._updated = True
            
    def assign_parameters_(
        self, 
        param_values: Union[dict[Symbol, Union[float, int]], list[Union[float, int]]]
    ):
        """Fuction responsible of assigning the values to the circuit parameter."""    
        if isinstance(param_values, dict):
            for param in self._params:
                # I filter the free parameters that are employed in the symbolic expression 
                values_i = {k.name: param_values.get(k.name) 
                            for k in param.variables 
                            if param_values.get(k.name) is not None}

                if len(values_i) != len(param.variables):
                    if param.value is None:
                        raise ValueError("Cannot update the param value and it is None, cannot execute.")
                    else:
                        logger.debug(f"{param} value remains the same due to lack of variables")
                else:
                    param.eval(values_i)
        elif isinstance(param_values, list):
            if len(param_values) != len(self._params):
                raise ValueError("List of parameter values is not the same as the number of "
                                 "parameters.")
            else:
                for param, value in zip(self._params, param_values):
                    param.assign_value(value)


def gather(qjobs: list[QJob]) -> list[Result]:
    """
        Function to get the results of several :py:class:`QJob` objects.

        Once the jobs are running:

            >>> results = gather(qjobs)

        This is a blocking call, results will be called sequentialy in . Since they are being run 
        simultaneously, even if the first one on the list takes the longest, when it finishes the 
        rest would have been done, so just the small overhead from calling them will be added.

        .. warning::
            Since this is mainly a for loop, the order must be respected when submiting jobs to the 
            same vQPU.

        Args:
            qjobs (list[QJob]): list of objects to get the result from.

        Return:
            List of :py:class:`~cunqa.result.Result` objects.
    """

    if(qjobs):
        return [q.result for q in qjobs]
    else: 
        raise AttributeError("qjobs in gather cannot be none.")    
