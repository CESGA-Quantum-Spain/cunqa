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
from collections import Counter, defaultdict, deque
from typing import  Optional, Any, Union

from cunqa.utils.logger import logger
from cunqa.result import Result
from cunqa.qclient import QClient, FutureWrapper
from sympy import Symbol
from cunqa.circuit.parameter import encoder, Param
from cunqa.real_qpus.qmioclient import QMIOClient, QMIOFuture


class ResultBuffer:
    """
    Router of the results coming from a vQPU that is shared by several :py:class:`QJob` objects.

    Every :py:class:`~cunqa.qpu.QPU` holds a single :py:class:`~cunqa.qclient.QClient`, therefore
    all the jobs sent to that vQPU read from the same socket: the result that arrives first is
    handed to whoever asks first, no matter which job it belongs to. To avoid such a mix up, the
    quantum tasks travel with the id of their circuit and the vQPU stamps that id back on the
    result. This class reads the results as they arrive and, when the one received is not the one
    being asked for, it stores it so that the :py:class:`QJob` that owns it can take it directly
    instead of going to the socket again.

    Results that carry no id (as the ones coming from QMIO) cannot be routed, so they are
    given to the caller, falling back to the FIFO behaviour.

    .. note::
        One buffer is created per :py:class:`~cunqa.qpu.QPU`, since the ordering problem it solves
        only exists among the jobs that share a connection.
    """
    _stored: dict[str, deque[str]]
    _outstanding: Counter

    def __init__(self):
        self._stored = defaultdict(deque)
        self._outstanding = Counter()

    def register(self, circuit_id: str) -> None:
        """
        Announces that a quantum task was submitted and that its result is still to be collected.
        Keeping track of what is in flight is what allows :py:meth:`~ResultBuffer.get` to refuse
        to read a result that nobody is waiting for, instead of blocking forever on the socket.

        Args:
            circuit_id (str): identificator of the circuit whose result is expected.
        """
        self._outstanding[circuit_id] += 1

    def get(self, future: Union[FutureWrapper, QMIOFuture], circuit_id: str) -> str:
        """
        Returns the result of the circuit `circuit_id` as the raw string sent by the vQPU. If the
        result was already received while waiting for another job, it is taken from the store.
        If not, results are read from the connection - and stored as they come - until the
        expected one shows up.

        Since all the jobs sent to the same vQPU share the connection, any of their futures reads
        from the same socket, so the `future` given is just the handle used to pull the results.

        Args:
            future (~cunqa.qclient.FutureWrapper | ~cunqa.real_qpus.qmioclient.QMIOFuture): handle
                to the connection with the vQPU.

            circuit_id (str): identificator of the circuit whose result is expected.

        Return:
            The result of the simulation as the raw string sent by the vQPU.
        """
        if self._outstanding[circuit_id] <= 0:
            raise RuntimeError(f"No result is pending for circuit {circuit_id}, either it was "
                               f"already collected or the job was never submitted.")

        if self._stored[circuit_id]:
            return self._collect_(circuit_id, self._stored[circuit_id].popleft())

        # Ours has not arrived yet, so results are read until it does. Everything received in the
        # meantime belongs to another job and is stored for when that job asks for it.
        while True:
            raw_result = future.get()
            result_id = self._id_of_(raw_result)

            if result_id is None:
                logger.debug(f"A result with no id was received, it is assumed to be the one of "
                             f"circuit {circuit_id}.")
                return self._collect_(circuit_id, raw_result)

            if result_id == circuit_id:
                return self._collect_(circuit_id, raw_result)

            logger.debug(f"Result of circuit {result_id} was received while waiting for "
                         f"{circuit_id}, it is stored.")
            self._stored[result_id].append(raw_result)

    def _collect_(self, circuit_id: str, raw_result: str) -> str:
        """Marks the result of `circuit_id` as collected and hands it to the caller."""
        self._outstanding[circuit_id] -= 1
        if not self._stored[circuit_id]:
            self._stored.pop(circuit_id, None)
        return raw_result

    @staticmethod
    def _id_of_(raw_result: str) -> Optional[str]:
        """
        Extracts the circuit id stamped by the vQPU on the result. ``None`` is returned when the
        result carries no id, which is also the case of a result that cannot be parsed: it is
        given to the caller so that the error is raised where it can be understood.
        """
        try:
            return json.loads(raw_result).get("id")
        except (ValueError, TypeError, AttributeError):
            return None


class QJob:
    """
    Class to handle jobs sent to vQPUs. A :py:class:`QJob` object is created as the output
    of the :py:func:`~cunqa.qpu.run` function. The quantum job not only contains the circuit to
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
    _id: str
    _updated: bool
    _device: dict
    _future: Union[FutureWrapper, QMIOFuture]
    _result: Optional[Result]
    _result_buffer: ResultBuffer
    _quantum_task: dict
    _params: list[Param]

    def __init__(
            self,
            qclient: Union[QClient, QMIOClient],
            device: dict,
            circuit_ir: dict,
            result_buffer: Optional[ResultBuffer] = None,
            **run_parameters: Any
    ):
        self._qclient = qclient
        self._device = device
        self._circuit_id = circuit_ir["id"]
        self._id = circuit_ir["id"][0]
        self._cregisters = circuit_ir["classical_registers"]
        self._params = circuit_ir["params"]
        self._updated = False
        self._future = None
        self._result = None
        # Without a buffer shared with the rest of the jobs sent to the same vQPU there is nobody
        # to route the results to, so a private one is enough.
        self._result_buffer = result_buffer if result_buffer is not None else ResultBuffer()

        run_config = {
            "shots": 1024, 
            "method":"automatic", 
            "avoid_parallelization": False,
            "num_clbits": circuit_ir["num_clbits"], 
            "num_qubits": circuit_ir["num_qubits"], 
            "device": self._device,
            "sending_to": circuit_ir["sending_to"],
            "is_dynamic": circuit_ir["is_dynamic"],
            "qpu_id": self._circuit_id[1]
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
            "id": self._id
        }
      
        logger.debug("Qjob configured")

    @property
    def result(self) -> Result:
        """
        Result of the job. If no error occured during simulation, a :py:class:`~cunqa.result.Result` 
        object is retured.

            >>> qjob = run(circuit, qpu)
            >>> result = qjob.result
            >>> print(result.counts)
            {'00': 524, '11': 500}

        .. note::
            Since to obtain the result the simulation has to be finished, this method is a blocking 
            call, which means that the program will be blocked until the :py:class:`QClient` has 
            recieved from the corresponding server the outcome of the job. The result is not sent 
            from the server to the :py:class:`QClient` until this method is called.
        
        .. note::
            Results can be called in any order, no matter the order in which the jobs were
            submitted. Every quantum task carries the id of its circuit and the vQPU stamps it
            back on the result, so when the result received is not the one being asked for, it is
            kept by the :py:class:`ResultBuffer` of the vQPU and delivered to the
            :py:class:`QJob` that owns it as soon as that job asks for it. Note that results
            coming from a real QPU carry no id, and so for them the *first in first out* rule
            still applies.

        """
        if self._future is not None:
            if (self._result is not None and not self._updated) or (self._result is None):
                res = self._result_buffer.get(self._future, self._id)
                self._result = Result(
                    json.loads(res),
                    circ_id=self._circuit_id[0],
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
            self._result_buffer.register(self._id)

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
        Also, this method is used by the class :py:class:`~cunqa.tools.mappers.QJobMapper`, check out its 
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
                logger.warning("You have not obtained the previous results. They will be discarded.")
                # We get the previous result because if not it stays in queue. It goes through the
                # buffer so that only ours is discarded, keeping the ones of the other jobs.
                self._result_buffer.get(self._future, self._id)
            else:
                raise RuntimeError("No circuit was sent before calling update_parameters().")

        if not len(param_values):
            raise AttributeError("No parameter list has been provided to the upgrade_parameters "
                                 "method.")

        self.assign_parameters_(param_values)
              
        try:
            params_str = json.dumps(self._params, default=encoder)
            config_str = json.dumps(self._quantum_task["config"], default=encoder)
            # The id travels with the update as it does with the circuit, so that the vQPU can
            # stamp it on the result and it can be routed back to this job.
            message = """{{"params": {}, "config": {}, "id": "{}"}}""".format(
                params_str.replace("'", '"'),
                config_str.replace("'", '"'),
                self._id
            )
            self._future = self._qclient.send_parameters(message)
            self._result_buffer.register(self._id)
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

        .. note::
            The jobs can be given in any order, even if several of them were sent to the same
            vQPU, since each result is identified by the id of its circuit. Have a look at
            :py:attr:`QJob.result` for the details.

        Args:
            qjobs (list[QJob]): list of objects to get the result from.

        Return:
            List of :py:class:`~cunqa.result.Result` objects.
    """

    if(qjobs):
        return [q.result for q in qjobs]
    else: 
        raise AttributeError("qjobs in gather cannot be none.")    
