"""
    Contains objects that define and manage quantum emulation jobs.

    The core of this module is the class :py:class:`~cunqa.qjob.QJob`. These objects are created 
    when a quantum job is sent to a virtual QPU, as a return of the :py:meth:`~cunqa.qpu.QPU.run` 
    method:

        >>> qpu.run(circuit)
        <cunqa.qjob.QJob object at XXXXXXXX>
        
    Once it is created, the circuit is being simulated at the virtual QPU.
    :py:class:`QJob` is the bridge between sending a circuit with instructions and recieving the 
    results. Because of this, usually one wants to save this output in a variable:

        >>> qjob = qpu.run(circuit)

    Another functionality described in the submodule is the function :py:func:`~gather`, 
    which recieves a list of :py:class:`~QJob` objects and returns their results as 
    :py:class:`~cunqa.result.Result` objects.

        >>> qjob_1 = qpu_1.run(circuit_1)
        >>> qjob_2 = qpu_2.run(circuit_2)
        >>> results = gather([qjob_1, qjob_2])
    
    For further information on sending and gathering quantum jobs, chekout the 
    `Examples Galery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.
    """

import json
from typing import  Optional, Any, Union

from cunqa.logger import logger
from cunqa.result import Result
from cunqa.qclient import QClient, FutureWrapper


class QJobError(Exception):
    """Exception for error during job submission to virtual QPUs."""
    pass

class QJob:
    """
    Class to handle jobs sent to virtual QPUs.

    A :py:class:`QJob` object is created as the output of the :py:meth:`~cunqa.qpu.QPU.run` method.
    The quantum job not only contains the circuit to be simulated, but also simulation instructions 
    and information of the virtual QPU to which the job is sent.

    One would want to save the :py:class:`QJob` resulting from sending a circuit in a variable.
    Let's say we want to send a circuit to a QPU and get the result and the time taken for the 
    simulation:

        >>> qjob = qpu.run(circuit)
        >>> result = qjob.result
        >>> print(result)
        <cunqa.result.Result object at XXXXXXXX>
        >>> time_taken = qjob.time_taken
        >>> print(time_taken)
        0.876

    Note that the `time_taken` is expressed in seconds. More instructions on obtaining data from 
    :py:class:`~cunqa.result.Result` objects can be found on its documentation.

    Handling QJobs sent to the same QPU
    ====================================
    Let's say we are sending two different jobs to the same QPU.
    This would not result on a parallelization, each QPU can only execute one simulation at the 
    time. When a virtual QPU recieves a job while simulating one, it would wait in line untill the 
    earlier finishes. Because of how the client-server comunication is built, we must be careful and 
    call for the results in the same order in which the jobs where submited. The correct workflow 
    would be:

        >>> qjob_1 = qpu.run(circuit_1)
        >>> qjob_2 = qpu.run(circuit_2)
        >>> result_1 = qjob_1.result
        >>> result_2 = qjob_2.result

    This is because the server follows the rule FIFO (*First in first out*), if we want to recieve 
    the second result, the first one has to be out.

    .. warning::
        In the case in which the order is not respected, everything would work, but results will not 
        correspond to the job. A mix up would happen.

    Handling QJobs sent to different QPUs
    =====================================
    Here we can have parallelization since we are working with more than one virtual QPU.
    Let's send two circuits to two different QPUs:

        >>> qjob_1 = qpu_1.run(circuit_1)
        >>> qjob_2 = qpu_2.run(circuit_2)
        >>> result_1 = qjob_1.result
        >>> result_2 = qjob_2.result

    This way, when we send the first job, then inmediatly the sencond one is sent, because 
    :py:meth:`~cunqa.qpu.QPU.run` does not wait for the simulation to finish. In this manner, both 
    jobs are being run in both QPUs simultaneously! Here we do not need to perserve the order, since 
    jobs are managed by different :py:class:`QClient` objects, there can be no mix up.

    In fact, the function :py:func:`~cunqa.qjob.gather` is designed for recieving a list of qjobs 
    and return the results; therefore, let's say we have a list of circuits that we want to submit 
    to a group of QPUs:

        >>> qjobs = []
        >>> for circuit, qpu in zip(circuits, qpus):
        >>> ... qjob = qpu.run(circuit)
        >>> ... qjobs.append(qjob)
        >>> results = gather(qjobs)

    In this workflow, all circuits are sent to each QPU an simulated at the same time. Then, when 
    calling for the results, the program is blocked waiting for all of them to finish. Other 
    examples of classical parallelization of quantum simulation taks can be found at the
    `Examples Gellery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.

    .. note::
        The function :py:func:`~cunqa.qjob.gather` can also handle :py:class:`~QJob` objects sent to 
        the same virtual QPU, but the order must be perserved in the list provided.


    Upgrading parameters from QJobs
    ===============================

    In some ocassion, especially working with variational quantum algorithms (VQAs) [#]_, they need 
    of changing the parameters of the gates in a circuit arises. These parameters are optimzied in 
    order to get the circuit to output a result that minimizes a problem. In this minimization 
    process, parameters are updated on each iteration (in general).
    
    Our first thought can be to update the parameters, build a new circuit with them and send it to 
    the QPU. Nevertheless, since the next circuit will have the same data but for the value of the 
    parameters in the gates, a lot of information is repeated, so :py:mod:`~cunqa` has a more 
    efficient and simple way to handle this cases: a method to send to the QPU a list with the new 
    parameters to be assigned to the circuit, :py:meth:`~QJob.upgrade_parameters`.

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

    From this simple workflow, we can build loops that update parameters according to some rules, or 
    by a optimizator, and upgrade the circuit until some stoping criteria is fulfilled.

    .. warning::
        Before sending the circuit or upgrading its parameters, the result of the prior job must be 
        called. It can be done manually, so that we can save it and obtain its information, or it 
        can be done automatically as in the example above, but be aware that once the 
        :py:meth:`upgrade_parameters` method is called, this result is discarded.

    References:
    ~~~~~~~~~~~

    .. [#] `Variational Quantum Algorithms arXiv <https://arxiv.org/abs/2012.09265>`_ .


    """
    qclient: QClient #: Client linked to the server that listens at the virtual QPU.
    _circuit_id: str
    _updated: bool
    _future: 'FutureWrapper' 
    _result: Optional['Result']
    _quantum_task: str

    def __init__(self, qclient: QClient, circuit_ir: dict, **run_parameters: Any):
        """
        Initializes the :py:class:`QJob` class.

        Possible instructions to add as `**run_parameters` can be: *shots*, *method*, 
        *parameter_binds*, *meas_level*, ... For further information, check 
        :py:meth:`~cunqa.qpu.QPU.run` method.

        .. warning::

            At this point, *circuit* is asumed to be translated into the native gates of the 
            *backend*. Otherwise, simulation will fail and an error will be returned as the result.
            For further details, checkout :py:mod:`~cunqa.transpile`.

        Args:
            qclient (QClient): client linked to the server that listens at the virtual QPU.

            backend (~cunqa.backend.Backend): gathers necessary information about the simulator.

            circuit_ir (dict): circuit to be run.

            **run_parameters : any other simulation instructions.

        """
        self._qclient = qclient
        self._circuit_id = circuit_ir["id"]
        self._cregisters = circuit_ir["classical_registers"]
        self._updated = False
        self._future = None
        self._result = None

        run_config = {
            "shots": 1024, 
            "method":"automatic", 
            "avoid_parallelization": False,
            "num_clbits": circuit_ir["num_clbits"], 
            "num_qubits": circuit_ir["num_qubits"], 
            "seed": 123123
        }

        if (run_parameters == None) or (len(run_parameters) == 0):
            logger.warning("No run parameters provided, default were set.")
        elif (type(run_parameters) == dict): 
            for k,v in run_parameters.items():
                run_config[k] = v
        else:
            logger.warning("Error when reading `run_parameters`, default were set.")
        
        self._quantum_task = json.dumps({
            "config": run_config, 
            "instructions": circuit_ir["instructions"],
            "sending_to": circuit_ir["sending_to"],
            "is_dynamic": circuit_ir["is_dynamic"],
            "has_cc": circuit_ir["has_cc"]
        })
    
        logger.debug("Qjob configured")

    @property
    def result(self) -> 'Result':
        """
        Result of the job.
        If no error occured during simulation, a :py:class:`~cunqa.result.Result` object is retured.
        Otherwise, :py:class:`~cunqa.result.ResultError` will be raised.

        .. note::
            Since to obtain the result the simulation has to be finished, this method is a blocking 
            call, which means that the program will be blocked until the :py:class:`QClient` has 
            recieved from the corresponding server the outcome of the job. The result is not sent 
            from the server to the :py:class:`QClient` until this method is called.

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

    @property
    def time_taken(self) -> str:
        """
        Time that the job took.
        """

        if self._future is not None:
            if self._result is not None:
                try:
                    return self._result.time_taken
                except AttributeError:
                    logger.error("Time taken not available.")
                    return ""
            else:
                raise RuntimeError("The QJob has been submitted, but the result has not "
                                   "been retrieved.")
        else:
            raise RuntimeError("The QJob has not been submitted.")

    def submit(self) -> None:
        """
        Asynchronous method to submit a job to the corresponding :py:class:`QClient`.

        .. note::
            Differently from :py:meth:`~QJob.result`, this is a non-blocking call.
            Once a job is summited, there is no wait, the python program continues at the same time 
            that the corresponding server recieves and simualtes the circuit.
        """
        if self._future is not None:
            logger.error("QJob has already been submitted.")
        else:
            try:
                self._future = self._qclient.send_circuit(self._quantum_task)
                logger.debug("Circuit was sent.")
            except Exception as error:
                raise RuntimeError((f"Some error occured when submitting the "
                                    f"job [{type(error).__name__}]."))
            
    def upgrade_parameters(self, parameters: list[Union[float, int]]) -> None:
        """
        Method to upgrade the parameters in a previously submitted job of parametric circuit.
        By this call, first it is checked weather if the prior simulation's result was called. If 
        not, it calls it but does not store it, then sends the new set of parameters to the server 
        to be reasigned to the circuit and to simulate it.

        This method can be used on a loop, always being careful if we want to save the intermediate 
        results.

        Examples of usage are shown above and on the 
        `Examples Gallery <https://cesga-quantum-spain.github.io/cunqa/examples_gallery.html>`_.
        Also, this method is used by the class :py:class:`~cunqa.mappers.QJobMapper`, checkout its 
        documentation for a extensive description.

        .. warning::
            In the current version, parameters will be assigned to **ALL** parametric gates in the 
            circuit. This means that if we want to make some parameters fixed, it is on our 
            responsibility to pass them correctly and in the correct order in the list. If the 
            number of parameters is less than the number of parametric gates in the circuit, an 
            error will occur at the virtual QPU, on the other hand, if more parameters are provided, 
            there will only be used up to the number of parametric gates.
            
            Also, only *rx*, *ry* and *rz* gates are supported for this functionality, that is, they 
            are the only gates considered *parametric* for this functionality.

        Args:
            parameters (list[float | int]): list of parameters to assign to the parametrized 
            circuit.
        """

        if self._result is None: 
            if self._future is not None:
                self._future.get()
            else:
                raise RuntimeError("No circuit was sent before calling update_parameters().")

        if not len(parameters):
            raise AttributeError("No parameter list has been provided to the upgrade_parameters "
                                 "method.")

        try:
            message = """{{"params":{} }}""".format(parameters).replace("'", '"')
            self._future = self._qclient.send_parameters(message)
            self._updated = False
        except Exception as error:
            logger.error(f"Some error occured when sending the new parameters to "
                         f"circuit {self._circuit_id} [{type(error).__name__}].")
            self._updated = True


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
            same virtual QPU.

        Args:
            qjobs (list[QJob]): list of objects to get the result from.

        Return:
            List of :py:class:`~cunqa.result.Result` objects.
    """

    try:    
        return [q.result for q in qjobs]
    except Exception:
        logger.error("gather needs a list of <class 'cunqa.qjob.QJob'>, but this was not provided")    
