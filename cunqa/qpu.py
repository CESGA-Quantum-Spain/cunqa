"""
    Contains our virtual QPU class.
"""

import os
from typing import  Union, Any, Optional
import inspect

from qiskit import QuantumCircuit

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit 
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
    Class to define a QPU.
    """
    _id: int 
    _qclient: 'QClient' 
    _backend: 'Backend' 
    _family: str
    _endpoint: "tuple[str, int]" 
    _connected: bool 
    
    def __init__(self, id: int, qclient: 'QClient', backend: Backend, family: str, endpoint: "tuple[str, int]"):
        """
        Initializes the QPU class.

        Args:
            id (int): Id assigned to the qpu, simply a int from 0 to n_qpus-1.

            qclient (<class 'python.qclient.QClient'>): object that holds the information to communicate with the server
                endpoint for a given QPU.
                
            backend (<class 'backend.Backend'>): object that provides information about the QPU backend.

            endpoint (str): String refering to the endpoint of the server to which the QPU corresponds.
        """
        
        self._id = id
        self._qclient = qclient
        self._backend = backend
        self._family = family
        self._endpoint = endpoint
        self._connected = False
        
        logger.debug(f"Object for QPU {id} created correctly.")

    @property
    def id(self) -> int:
        return self._id
    
    @property
    def backend(self) -> Backend:
        return self._backend

    def run(self, circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit'], transpile: bool = False, initial_layout: Optional["list[int]"] = None, opt_level: int = 1, **run_parameters: Any) -> 'QJob':
        """
        Class method to run a circuit in the QPU.

        It is important to note that  if `transpilation` is set False, we asume user has already done the transpilation, otherwise some errors during the simulation
        can occur, for example if the QPU has a noise model with error associated to specific gates, if the circuit is not transpiled errors might not appear.

        If `transpile` is False and `initial_layout` is provided, it will be ignored.

        Possible instructions to add as `**run_parameters` can be: shots, method, parameter_binds, meas_level, ...

        Args:
            circuit (json dict or <class 'qiskit.circuit.CunqaCircuit'>): circuit to be run in the QPU.

            transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

            initial_layout (list[int]): Initial position of virtual qubits on physical qubits for transpilation.

            opt_level (int): optimization level for transpilation, default set to 1.

            **run_parameters : any other simulation instructions.

        Return:
            <class 'QJob'> object.
        """
        # Disallow execution of distributed circuits
        if inspect.stack()[1].function != "run_distributed": # Checks if the run() is called from run_distributed()
            if isinstance(circuit, CunqaCircuit):
                if circuit.has_cc:
                    logger.error("Distributed circuits can't run using QPU.run(), try run_distributed() instead.")
                    raise SystemExit
            elif isinstance(circuit, dict):
                if 'has_cc' in circuit and circuit["has_cc"]:
                    logger.error("Distributed circuits can't run using QPU.run(), try run_distributed() instead.")
                    raise SystemExit



        # Handle connection to QClient
        if not self._connected:
            ip, port = self._endpoint
            self._qclient.connect(ip, port)
            self._connected = True
            logger.debug(f"QClient connection stabished for QPU {self._id} to endpoint {ip}:{port}.")
            self._connected = True

        if transpile:
            try:
                logger.debug(f"About to transpile: {circuit}")
                circuit = transpiler(circuit, self._backend, initial_layout = initial_layout, opt_level = opt_level)
                logger.debug("Transpilation done.")
            except Exception as error:
                logger.error(f"Transpilation failed [{type(error).__name__}].")
                raise TranspileError # I capture the error in QPU.run() when creating the job

        try:
            qjob = QJob(self._qclient, self._backend, circuit, **run_parameters)
            qjob.submit()
            logger.debug(f"Qjob submitted to QPU {self._id}.")
        except Exception as error:
            logger.error(f"Error when submitting QJob [{type(error).__name__}].")
            raise SystemExit

        return qjob
