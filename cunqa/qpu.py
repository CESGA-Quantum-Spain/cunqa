"""
    Contains our virtual QPU class.
"""

import os
from typing import  Union, Any

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit 
from cunqa.backend import Backend
from cunqa.qjob import QJob
from cunqa.logger import logger
from cunqa.transpile import transpiler, TranspilerError

# path to access to json file holding information about the raised QPUs
info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.cunqa/qpus.json"


class QPU:
    """
    Class to define a QPU.
    """
    
    def __init__(self, id : int, qclient : QClient, backend : Backend, family : str, endpoint : tuple, node_mode: tuple):
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
        self._node_mode = node_mode
        self._connected = False
        
        logger.debug(f"Object for QPU {id} created correctly.")

    @property
    def id(self):
        return self._id
    
    @property
    def backend(self):
        return self._backend

    def run(self, circuit: Union[dict, CunqaCircuit], transpile: bool = False, initial_layout: list[int] = None, opt_level: int = 1, **run_parameters: Any) -> QJob:
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
        local_node = os.getenv("SLURMD_NODENAME")
        if (self._node_mode[1] == "hpc" and self._node_mode[0] !=local_node):
            logger.error(f"Error while running: trying to run a job from node {local_node} on a QPU in HPC mode connected to node {self._node_mode[0]}." )
            raise SystemExit
    
        if not self._connected:
            ip, port = self._endpoint
            self._qclient.connect(ip, port)
            logger.debug(f"QClient connection stabished for QPU {self._id} to endpoint {ip}:{port}.")
            self._connected = True

        if transpile:
            try:
                circuit = transpiler(circuit, self._backend, initial_layout = initial_layout, opt_level = opt_level)
                logger.debug("Transpilation done.")
            except Exception as error:
                logger.error(f"Transpilation failed [{type(error).__name__}].")
                raise TranspilerError # I capture the error in QPU.run() when creating the job

        try:
            qjob = QJob(self._qclient, self._backend, circuit, **run_parameters)
            qjob.submit()
            logger.debug(f"Qjob submitted to QPU {self._id}.")
        except Exception as error:
            logger.error(f"Error when submitting QJob [{type(error).__name__}].")
            raise SystemExit

        return qjob
