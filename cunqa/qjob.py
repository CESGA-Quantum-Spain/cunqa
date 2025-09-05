"""
    Contains objects that define and manage quantum emulation jobs.

    The core of this module is the class :py:class:`~cunqa.qjob.QJob`. These objects are created when a quantum job
    is sent to a virtual QPU, as a return of the :py:meth:`~cunqa.qpu.QPU.run` method:

        >>> qpu.run(circuit)
        <cunqa.qjob.QJob object at XXXXXXXX>
        
    Once it is created, the circuit is being simulated at the virtual QPU.
    :py:class:`QJob` is the bridge between sending a circuit with instructions and recieving the results.
    Because of this, usually one wants to save this output in a variable:

        >>> qjob = qpu.run(circuit)

    Another functionality described in the submodule is the function :py:func:`~gather`, 
    which recieves a list of :py:class:`~QJob` objects and returns their results as :py:class:`~cunqa.result.Result`
    objects.

        >>> qjob_1 = qpu_1.run(circuit_1)
        >>> qjob_2 = qpu_2.run(circuit_2)
        >>> results = gather([qjob_1, qjob_2])
    
    For further information on sending and gathering quantum jobs, chekout the `Examples Galery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.


    """

from typing import  Union, Any
import json, pickle
import zmq
from typing import  Optional, Union, Any
from qiskit import QuantumCircuit
from qiskit.qasm2.exceptions import QASM2Error
from qiskit.exceptions import QiskitError

from cunqa.circuit import CunqaCircuit
from cunqa.converters import convert, _registers_dict
from cunqa.qmio_helpers import _config_builder
from cunqa.logger import logger
from cunqa.backend import Backend
from cunqa.result import Result
from cunqa.qclient import QClient, FutureWrapper


class QJobError(Exception):
    """Exception for error during job submission to virtual QPUs."""
    pass

class QJob:
    """
    Class to handle jobs sent to virtual QPUs.

    A :py:class:`QJob` object is created as the output of the :py:meth:`~cunqa.qpu.QPU.run` method.
    The quantum job not only contains the circuit to be simulated, but also simulation instructions and information of the virtual QPU to which the job is sent.

    One would want to save the :py:class:`QJob` resulting from sending a circuit in a variable.
    Let's say we want to send a circuit to a QPU and get the result and the time taken for the simulation:

        >>> qjob = qpu.run(circuit)
        >>> result = qjob.result
        >>> print(result)
        <cunqa.result.Result object at XXXXXXXX>
        >>> time_taken = qjob.time_taken
        >>> print(time_taken)
        0.876

    Note that the `time_taken` is expressed in seconds. More instructions on obtaining data from :py:class:`~cunqa.result.Result`
    objects can be found on its documentation.

    Handling QJobs sent to the same QPU
    ====================================
    Let's say we are sending two different jobs to the same QPU.
    This would not result on a parallelization, each QPU can only execute one simulation at the time.
    When a virtual QPU recieves a job while simulating one, it would wait in line untill the earlier finishes.
    Because of how the client-server comunication is built, we must be careful and call for the results in the same order in which the jobs where submited.
    The correct workflow would be:

        >>> qjob_1 = qpu.run(circuit_1)
        >>> qjob_2 = qpu.run(circuit_2)
        >>> result_1 = qjob_1.result
        >>> result_2 = qjob_2.result

    This is because the server follows the rule FIFO (*First in first out*), if we want to recieve the second result,
    the first one has to be out.

    .. warning::
        In the case in which the order is not respected, everything would work, but results will not correspond
        to the job. A mix up would happen.

    Handling QJobs sent to different QPUs
    =====================================
    Here we can have parallelization since we are working with more than one virtual QPU.
    Let's send two circuits to two different QPUs:

        >>> qjob_1 = qpu_1.run(circuit_1)
        >>> qjob_2 = qpu_2.run(circuit_2)
        >>> result_1 = qjob_1.result
        >>> result_2 = qjob_2.result

    This way, when we send the first job, then inmediatly the sencond one is sent, because :py:meth:`~cunqa.qpu.QPU.run` does not wait
    for the simulation to finish. In this manner, both jobs are being run in both QPUs simultaneously! Here we do not need to perserve the order,
    since jobs are managed by different :py:class:`QClient` objects, there can be no mix up.

    In fact, the function :py:func:`~cunqa.qjob.gather` is designed for recieving a list of qjobs and return the results; therefore, let's say
    we have a list of circuits that we want to submit to a group of QPUs:

        >>> qjobs = []
        >>> for circuit, qpu in zip(circuits, qpus):
        >>> ... qjob = qpu.run(circuit)
        >>> ... qjobs.append(qjob)
        >>> results = gather(qjobs)

    In this workflow, all circuits are sent to each QPU an simulated at the same time. Then, when calling for the results, the program is blocked
    waiting for all of them to finish. Other examples of classical parallelization of quantum simulation taks can be found at the
    `Examples Gellery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.

    .. note::
        The function :py:func:`~cunqa.qjob.gather` can also handle :py:class:`~QJob` objects sent to the same virtual QPU, but the order must
        be perserved in the list provided.


    Upgrading parameters from QJobs
    ===============================

    In some ocassion, especially working with variational quantum algorithms (VQAs) [#]_, they need of changing the parameters of the gates in a circuit
    arises. These parameters are optimzied in order to get the circuit to output a result that minimizes a problem. In this minimization process,
    parameters are updated on each iteration (in general).
    
    Our first thought can be to update the parameters, build a new circuit with them and send
    it to the QPU. Nevertheless, since the next circuit will have the same data but for the value of the parameters in the gates, a lot of information
    is repeated, so :py:mod:`~cunqa` has a more efficient and simple way to handle this cases: a method to send to the QPU a list with the new parameters
    to be assigned to the circuit, :py:meth:`~QJob.upgrade_parameters`.

    Let's see a simple example: creating a parametric circuit and uptading its parameters:

        >>> # building the parametric circuit
        >>> circuit = CunqaCircuit(3)
        >>> circuit.ry(0.25, 0)
        >>> circuit.rx(0.5, 1)
        >>> circuit.p(0.98, 2)
        >>> circuit.measure_all()
        >>> # sending the circuit to a virtual QPU
        >>> qjob = qpu.run(circuit)
        >>> # defining the new set of parameters
        >>> new_parameters = [1,1,0]
        >>> # upgrading the parameters of the job
        >>> qjob.upgrade_parameters(new_parameters)
        >>> result = qjob.result

    From this simple workflow, we can build loops that update parameters according to some rules, or by a optimizator, and upgrade the circuit until
    some stoping criteria is fulfilled.

    .. warning::
        Before sending the circuit or upgrading its parameters, the result of the prior job must be called. It can be done manually, so that we can
        save it and obtain its information, or it can be done automatically as in the example above, but be aware that once the :py:meth:`upgrade_parameters`
        method is called, this result is discarded.

    References:
    ~~~~~~~~~~~

    .. [#] `Variational Quantum Algorithms arXiv <https://arxiv.org/abs/2012.09265>`_ .


    """
    _backend: 'Backend' 
    _qclient: Union['QClient', 'zmq.Context'] #: Client linked to the server that listens at the virtual QPU.
    _updated: bool
    _future: 'FutureWrapper' 
    _result: Optional['Result']
    _circuit_id: str 
    _sending_to: "list[str]"
    _is_dynamic: bool
    _has_cc:bool
    _has_qc:bool

    def __init__(self, real_qpu: bool, qclient: Union['QClient', 'zmq.Context'], backend: 'Backend', circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit', str], **run_parameters: Any):
        """
        Initializes the :py:class:`QJob` class.

        Possible instructions to add as `**run_parameters` can be: *shots*, *method*, *parameter_binds*, *meas_level*, ...
        For further information, check :py:meth:`~cunqa.qpu.QPU.run` method.

        .. warning::

            At this point, *circuit* is asumed to be translated into the native gates of the *backend*.
            Otherwise, simulation will fail and an error will be returned as the result.
            For further details, checkout :py:mod:`~cunqa.transpile`.

        Args:
            qclient (QClient): client linked to the server that listens at the virtual QPU.

            backend (~cunqa.backend.Backend): gathers necessary information about the simulator.

            circ (qiskit.QuantumCircuit | dict | ~cunqa.circuit.CunqaCircuit): circuit to be run.

            **run_parameters : any other simulation instructions.

        """

        self._backend: 'Backend' = backend
        self._qclient: Union['QClient', 'zmq.Context'] = qclient
        self._updated: bool = False
        self._future: 'FutureWrapper' = None
        self._result: Optional['Result'] = None
        self._circuit_id: str = ""
        self._real_qpu = real_qpu

        self._convert_circuit(circuit)
        self._configure(**run_parameters)
        logger.debug("Qjob configured")

    @property
    def result(self) -> 'Result':
        """
        Result of the job.
        If no error occured during simulation, a :py:class:`~cunqa.result.Result` object is retured.
        Otherwise, :py:class:`~cunqa.result.ResultError` will be raised.

        .. note::
            Since to obtain the result the simulation has to be finished, this method is a blocking call,
            which means that the program will be blocked until the :py:class:`QClient` has recieved from
            the corresponding server the outcome of the job.
            The result is not sent from the server to the :py:class:`QClient` until this method is called.

        """
        if self._real_qpu:
            try:
                serialized_result = self._qclient.recv()
                logger.debug("Result received from QPU")
                deserialized_result = pickle.loads(serialized_result)
                res = json.loads(json.dumps(deserialized_result))
                logger.debug(f"Deserialized result {res}")
                self._result = Result(json.loads(res), circ_id=self._circuit_id, registers=self._cregisters)
                self._updated = True
            except Exception as error:
                    logger.error(f"Error while reading the results {error}")
                    raise SystemExit # User's level
        else:
            try:
                if self._future is not None and self._future.valid():
                    if self._result is not None:
                        if not self._updated: # if the result was already obtained, we only call the server if an update was done
                            res = self._future.get()
                            self._result = Result(json.loads(res), circ_id=self._circuit_id, registers=self._cregisters)
                            self._updated = True
                        else:
                            pass
                    else:
                        res = self._future.get()
                        logger.debug(f"RES: {res}")
                        self._result = Result(json.loads(res), self._circuit_id, registers=self._cregisters)
                        self._updated = True
                else:
                    logger.debug(f"self._future is None or non-valid, None is returned.")
            except Exception as error:
                    logger.error(f"Error while reading the results {error}")
                    raise SystemExit # User's level
            
            if self._backend.simulator == "CunqaSimulator" and self.num_clbits != self.num_qubits:
                logger.warning(f"Be aware that for CunqaSimualtor, number of clbits is required to be equal than the number of qubits of the circuit. Classical bits can appear to be rewritten.")

        return self._result

    @property
    def time_taken(self) -> str:
        """
        Time that the job took.
        """

        if self._future is not None and self._future.valid():
            if self._result is not None:
                try:
                    return self._result.time_taken
                except AttributeError:
                    logger.warning("Time taken not available.")
                    return ""
            else:
                logger.error(f"QJob not finished [{QJobError.__name__}].")
                raise SystemExit # User's level
        else:
            logger.error(f"No QJob submited [{QJobError.__name__}].")
            raise SystemExit # User's level

    def submit(self) -> None:
        """
        Asynchronous method to submit a job to the corresponding :py:class:`QClient`.

        .. note::
            Differently from :py:meth:`~QJob.result`, this is a non-blocking call.
            Once a job is summited, there is no wait, the python program continues at the same time that
            the corresponding server recieves and simualtes the circuit.
        """
        if self._real_qpu:
            data_to_send = (self._circuit, self._execution_config)
            self._qclient.send_pyobj(data_to_send)
        else:
            if self._future is not None:
                logger.warning("QJob has already been submitted.")
            else:
                try:
                    self._future = self._qclient.send_circuit(self._execution_config)
                    logger.debug("Circuit was sent.")
                except Exception as error:
                    logger.error(f"Some error occured when submitting the job [{type(error).__name__}].")
                    raise QJobError # I capture the error in QPU.run() when creating the job
            
    def upgrade_parameters(self, parameters: "list[float | int]") -> None:
        """
        Method to upgrade the parameters in a previously submitted job of parametric circuit.
        By this call, first it is checked weather if the prior simulation's result was called. If not, it calls it but does not store it, then
        sends the new set of parameters to the server to be reasigned to the circuit and to simulate it.

        This method can be used on a loop, always being careful if we want to save the intermediate results.

        Examples of usage are shown above and on the `Examples Gallery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.
        Also, this method is used by the class :py:class:`~cunqa.mappers.QJobMapper`, checkout its documentation for a extensive description.

        .. warning::
            In the current version, parameters will be assigned to **ALL** parametric gates in the circuit. This means that if we want to make
            some parameters fixed, it is on our responsibility to pass them correctly and in the correct order in the list.
            If the number of parameters is less than the number of parametric gates in the circuit, an error will occur at the virtual QPU, on
            the other hand, if more parameters are provided, there will only be used up to the number of parametric gates.
            
            Also, only *rx*, *ry* and *rz* gates are supported for this functionality, that is, they are the only gates considered *parametric* for this functionality.

        Args:
            parameters (list[float | int]): list of parameters to assign to the parametrized circuit.
        """

        if self._result is None:
            self._future.get()

        if isinstance(parameters, list):

            if all(isinstance(param, (int, float)) for param in parameters):  # Check if all elements are real numbers
                message = """{{"params":{} }}""".format(parameters).replace("'", '"')

            else:
                logger.error(f"Parameters must be real numbers [{ValueError.__name__}].")
                raise SystemExit # User's level
        
            try:
                #logger.debug(f"Sending new parameters to circuit {self._circuit_id}.")
                self._future = self._qclient.send_parameters(message)

            except Exception as error:
                logger.error(f"Some error occured when sending the new parameters to circuit {self._circuit_id} [{type(error).__name__}].")
                raise SystemExit # User's level
        else:
            logger.error(f"Ivalid parameter type, list was expected but {type(parameters)} was given. [{TypeError.__name__}].")
            raise SystemExit # User's level            
        
        self._updated = False # We indicate that new results will come, in order to call server

    def _convert_circuit(self, circuit: Union[str, dict, 'CunqaCircuit', 'QuantumCircuit']) -> None:
        if self._real_qpu:
            logger.debug("Converting circuit to qasm for the real QPU")
            self._circuit = convert(circuit, "qasm")
        else:
            try:
                if isinstance(circuit, dict):

                    logger.debug("A circuit dict was provided.")

                    self.num_qubits = circuit["num_qubits"]
                    self.num_clbits = circuit["num_clbits"]
                    self._cregisters = circuit["classical_registers"]
                    if "sending_to" in circuit:
                        self._sending_to = circuit["sending_to"]
                    else:
                        self._sending_to = []
                    if "has_cc" in circuit:
                        self._has_cc = circuit["has_cc"]
                        self._is_dynamic = True
                    elif "has_qc" in circuit:
                        self._has_qc = circuit["has_qc"]
                        self._is_dynamic = True
                    elif "is_dynamic" in circuit:
                        self._is_dynamic = circuit["is_dynamic"]
                    else:
                        self._is_dynamic = False
                        self._has_cc = False

                    # might explode for handmade dicts not design for ditributed execution
                    self._circuit_id = circuit["id"]
                    instructions = circuit['instructions']
                

                elif isinstance(circuit, CunqaCircuit):

                    logger.debug("A CunqaCircuit was provided.")

                    self.num_qubits = circuit.num_qubits
                    self.num_clbits = circuit.num_clbits
                    self._cregisters = circuit.classical_regs
                    self._circuit_id = circuit._id
                    self._sending_to = circuit.sending_to
                    self._is_dynamic = circuit.is_dynamic
                    self._has_cc = circuit.has_cc
                    self._has_qc = circuit.has_qc

                    if circuit.is_parametric:
                        self._param_labels = circuit.param_labels
                        self._current_params = circuit.current_params
                    
                    logger.debug("Translating to dict from CunqaCircuit...")

                    instructions = circuit.instructions


                elif isinstance(circuit, QuantumCircuit):

                    logger.debug("A QuantumCircuit was provided.")

                    self.num_qubits = circuit.num_qubits
                    self.num_clbits = sum([c.size for c in circuit.cregs])
                    self._cregisters = _registers_dict(circuit)[1]
                    self._sending_to = []

                    logger.debug("Translating to dict from QuantumCircuit...")

                    circuit_json = convert(circuit, "dict")
                    instructions = circuit_json["instructions"]
                    self._is_dynamic = circuit_json["is_dynamic"]
                    self._has_cc = False
                    self.has_qc = False

                elif isinstance(circuit, str):

                    logger.debug("A QASM2 circuit was provided.")

                    qc_from_qasm = QuantumCircuit.from_qasm_str(circuit)

                    self.num_qubits = qc_from_qasm.num_qubits
                    self._cregisters = _registers_dict(qc_from_qasm)[1]
                    self.num_clbits = sum(len(k) for k in self._cregisters.values())
                    self._cregisters = _registers_dict(qc_from_qasm)[1]
                    # TODO: ¿self.circuit_id?
                    self._sending_to = []

                    logger.debug("Translating to dict from QASM2 string...")

                    circuit_json = convert(qc_from_qasm, "dict")
                    instructions = circuit_json["instructions"]
                    self._is_dynamic = circuit_json["is_dynamic"]
                    self._has_cc = False
                    self._has_qc = False

                else:
                    logger.error(f"Circuit must be dict, <class 'cunqa.circuit.CunqaCircuit'> or QASM2 str, but {type(circuit)} was provided [{TypeError.__name__}].")
                    raise QJobError # I capture the error in QPU.run() when creating the job

                self._circuit = instructions            
                
            except KeyError as error:
                logger.error(f"Format of the circuit dict not correct, couldn't find 'num_clbits', 'classical_registers' or 'instructions' [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job

            except QASM2Error as error:
                logger.error(f"Error while translating to QASM2 [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
        
            except QiskitError as error:
                logger.error(f"Format of the circuit not correct  [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
        
            except Exception as error:
                logger.error(f"Some error occured with the circuit dict provided [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job

    def _configure(self, **run_parameters: Any) -> None:
        # configuration
        if self._real_qpu:
            if "shots" in run_parameters:
                self._execution_config = _config_builder(run_parameters["shots"])
            else:
                self._execution_config = _config_builder(1024)
        else:
            try:
                # config dict
                run_config = {
                    "shots": 1024, 
                    "method":"statevector", 
                    "num_clbits": self.num_clbits, 
                    "num_qubits": self.num_qubits, 
                    "seed": 123123}

                if (run_parameters == None) or (len(run_parameters) == 0):
                    logger.debug("No run parameters provided, default were set.")
                    pass
                elif (type(run_parameters) == dict): 
                    for k,v in run_parameters.items():
                        run_config[k] = v
                else:
                    logger.warning("Error when reading `run_parameters`, default were set.")
                
                exec_config = {
                    "id": self._circuit_id,
                    "config": run_config, 
                    "instructions": self._circuit,
                    "sending_to": self._sending_to,
                    "is_dynamic": self._is_dynamic,
                    "has_cc": self._has_cc
                    #,"has_qc": self._has_qc # Not needed in C++ 
                }
                self._execution_config = json.dumps(exec_config)

                logger.debug("QJob created.")

            except KeyError as error:
                logger.error(f"Format of the cirucit not correct, couldn't find 'instructions' [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
            
            except Exception as error:
                logger.error(f"Some error occured when generating configuration for the simulation [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
        


def gather(qjobs: "list['QJob']") -> "list['Result']":
    """
        Function to get the results of several :py:class:`QJob` objects.

        Once the jobs are running:

            >>> results = gather(qjobs)

        This is a blocking call, results will be called sequentialy in . Since they are being run simultaneously,
        even if the first one on the list takes the longest, when it finishes the rest would have been done, so
        just the small overhead from calling them will be added.

        .. warning::
            Since this is mainly a for loop, the order must be respected when submiting jobs to the same virtual QPU.

        Args:
            qjobs (list[QJob]): list of objects to get the result from.

        Return:
            List of :py:class:`~cunqa.result.Result` objects.
    """
    if isinstance(qjobs, list):
        if all([isinstance(q, QJob) for q in qjobs]):
            return [q.result for q in qjobs]
        else:
            logger.error(f"Objects of the list must be <class 'cunqa.qjob.QJob'> [{TypeError.__name__}].")
            raise SystemExit # User's level

    else:
        logger.error(f"qjobs must be list, but {type(qjobs)} was provided [{TypeError.__name__}].")
        raise SystemError # User's level
