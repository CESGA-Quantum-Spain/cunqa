"""
    Contains the description of the :py:class:`~cunqa.qpu.QPU` class.

    These :py:class:`QPU` objects are the python representation of the virtual QPUs deployed.
    Each one has its :py:class:`QClient` object that communicates with the server of the corresponding virtual QPU.
    Through these objects we are able to send circuits and recieve results from simulations.

    Virtual QPUs
    ============
    Each virtual QPU is described by three elements:

        - **Server**: classical resources intended to communicate with the python API to recieve circuits or quantum tasks and to send results of the simulations.
        - **Backend**: characteristics that define the QPU that is emulated: coupling map, basis gates, noise model, etc.
        - **Simulator**: classical resources intended to simulate circuits accordingly to the backend characteristics.
    
    .. image:: /_static/virtualqpuequal.png
        :align: center
        :width: 400px
        :height: 200px

    In order to stablish communication with the server, in the python API :py:class:`QPU` objects are created, each one of them associated with a virtual QPU.
    Each object will have a :py:class:`QClient` C++ object through which the communication with the classical resoruces is performed.
    
    .. image:: /_static/client-server-comm.png
        :align: center
        :width: 150
        :height: 300px

        
    Connecting to virtual QPUs
    ==========================

    The submodule :py:mod:`~cunqa.qutils` gathers functions for obtaining information about the virtual QPUs available;
    among them, the :py:func:`~cunqa.qutils.get_QPUs` function returns a list of :py:class:`QPU` objects with the desired filtering:

    >>> from cunqa import get_QPUs
    >>> get_QPUs()
    [<cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>]

    When each :py:class:`QPU` is instanciated, the corresponding :py:class:`QClient` is created.
    Nevertheless, it is not until the first job is submited that the client actually connects to the correspoding server.
    Other properties and information gathered in the :py:class:`QPU` class are shown on its documentation.

    Interacting with virtual QPUs
    =============================

    Once we have the :py:class:`QPU` objects created, we can start interacting with them.
    The most important method of the class is :py:meth:`QPU.run`, which allows to send a circuit to the virtual QPU for its simulation,
    returning a :py:class:`~cunqa.qjob.QJob` object associated to the quantum task:

        >>> qpus = get_QPUs()
        >>> qpu = qpus[0]
        >>> qpu.run(circuit)
        <cunqa.qjob.QJob object at XXXX>

    This method takes several arguments for specifications of the simulation such as `shots` or `transpilation`.
    For a larger description of its functionalities checkout its documentation.

"""

import os
from typing import  Union, Any, Optional
import inspect
import zmq 

from qiskit import QuantumCircuit

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit, _is_parametric
from cunqa.backend import Backend
from cunqa.qjob import QJob
from cunqa.logger import logger
from cunqa.transpile import transpiler, TranspileError

# path to access to json file holding information about the raised QPUs
INFO_PATH: Optional[str] = os.getenv("INFO_PATH")
if INFO_PATH is None:
    STORE: Optional[str] = os.getenv("STORE")
    if STORE is not None:
        INFO_PATH = STORE + "/.cunqa/qpus.json"
    else:
        logger.error(f"Cannot find $STORE enviroment variable.")
        raise SystemExit



class QPU:
    """
    Class to represent a virtual QPU deployed for user interaction.

    This class contains the neccesary data for connecting to the virtual QPU's server in order to communicate circuits and results in both ways.
    This communication is stablished trough the :py:attr:`QPU.qclient`.

    """
    _id: int #: Id string assigned to the object.
    _qclient: Union['QClient', 'zmq.Context']  #: Object that holds the information to communicate with the server endpoint of the corresponding virtual QPU.
    _backend: 'Backend' #: Object that provides the characteristics that the simulator at the virtual QPU uses to emulate a real device.
    _family: str #: Name of the family to which the corresponding virtual QPU belongs.
    _endpoint: "tuple[str, int]" #: String refering to the endpoint of the corresponding virtual QPU.
    _connected: bool #: Weather if the :py:class:`QClient` is already connected.
    _real_qpu: bool #: If this QPU is associated to a real QPU device
    
    def __init__(self, id: int, qclient: Union['QClient', 'zmq.Context'], backend: Backend, family: str, endpoint: "tuple[str, int]", real_qpu = False):
        """
        Initializes the :py:class:`QPU` class.

        This initialization of the class is done by the :py:func:`~cunqa.qutils.get_QPUs` function, which loads the `id`,
        `family` and `endpoint`, and instanciates the `qclient` and the `backend` objects.

        Args:
            id (str): id string assigned to the object.

            qclient (QClient): object that holds the information to communicate with the server endpoint of the corresponding virtual QPU.
                
            backend (~cunqa.backend.Backend): object that provides the characteristics that the simulator at the virtual QPU uses to emulate a real device.

            family (str):  name of the family to which the corresponding virtual QPU belongs.
            
            endpoint (str): string refering to the endpoint of the corresponding virtual QPU.
        """
        
        self._id = id
        self._qclient = qclient
        self._backend = backend
        self._family = family
        self._endpoint = endpoint
        self._connected = False
        self._real_qpu = real_qpu
        
        logger.debug(f"Object for QPU {id} created correctly.")

    @property
    def id(self) -> int:
        """Id string assigned to the object."""
        return self._id
    
    @property
    def backend(self) -> Backend:
        """Object that provides the characteristics that the simulator at the virtual QPU uses to emulate a real device."""
        return self._backend

    def run(self, circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit'], transpile: bool = False, initial_layout: Optional["list[int]"] = None, opt_level: int = 1, **run_parameters: Any) -> 'QJob':
        """
        Class method to send a circuit to the corresponding virtual QPU.

        It is important to note that  if `transpile` is set ``False``, we asume user has already done the transpilation, otherwise some errors during the simulation can occur.

        Possible instructions to add as `**run_parameters` depend on the simulator, but mainly `shots` and `method` are used.

        Args:
            circuit (dict | qiskit.QuantumCircuit | ~cunqa.circuit.CunqaCircuit): circuit to be simulated at the virtual QPU.

            transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

            initial_layout (list[int]): Initial position of virtual qubits on physical qubits for transpilation.

            opt_level (int): optimization level for transpilation, default set to 1.

            **run_parameters: any other simulation instructions.

        Return:
            A :py:class:`~cunqa.qjob.QJob` object related to the job sent.


        .. warning::
            If `transpile` is set ``False`` and transpilation instructions (`initial_layout`, `opt_level`) are provided, they will be ignored.
        
        .. note::
            Transpilation is the process of translating circuit instructions into the native gates of the destined backend accordingly to the topology of its qubits.
            If this is not done, the simulatior receives the instructions but associates no error, so simulation outcome will not be correct.

        """

        # Disallow execution of distributed circuits
        if inspect.stack()[1].function != "run_distributed": # Checks if the run() is called from run_distributed()
            if isinstance(circuit, CunqaCircuit):
                if circuit.has_cc or circuit.has_qc:
                    logger.error("Distributed circuits can't run using QPU.run(), try run_distributed() instead.")
                    raise SystemExit
            elif isinstance(circuit, dict):
                if ('has_cc' in circuit and circuit["has_cc"]) or ('has_qc' in circuit and circuit["has_qc"]):
                    logger.error("Distributed circuits can't run using QPU.run(), try run_distributed() instead.")
                    raise SystemExit

        # Handle connection to QClient
        if not self._connected:
            ip, port = self._endpoint
            if self._real_qpu:
                intermediary_endpoint = f"tcp://{ip}:{port}"
                self._qclient.connect(intermediary_endpoint)
            else:
                self._qclient.connect(ip, port)
            self._connected = True
            logger.debug(f"QClient connection stabished for QPU {self._id} to endpoint {ip}:{port}.")

        # Transpilation if requested
        if transpile:
            try:
                logger.debug(f"About to transpile: {circuit}")
                circuit = transpiler(circuit, self._backend, initial_layout = initial_layout, opt_level = opt_level)
                logger.debug("Transpilation done.")
            except Exception as error:
                logger.error(f"Transpilation failed [{type(error).__name__}].")
                raise TranspileError # I capture the error in QPU.run() when creating the job

        try:
            qjob = QJob(self._real_qpu, self._qclient, self._backend, circuit, **run_parameters)
            qjob.submit()
            logger.debug(f"Qjob submitted to QPU {self._id}.")
        except Exception as error:
            logger.error(f"Error when submitting QJob [{type(error).__name__}].")
            raise SystemExit

        return qjob
