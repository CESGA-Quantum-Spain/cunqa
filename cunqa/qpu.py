import os
from json import JSONDecodeError, load
from cunqa.qclient import QClient  # importamos api en C++
from cunqa.backend import Backend
from cunqa.qjob import QJob
from cunqa.logger import logger

# path to access to json file holding information about the raised QPUs
info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.cunqa/qpus.json"


class QPU:
    """
    Class to define a QPU.
    ----------------------
    """
    
    def __init__(self, id=None, qclient=None, backend=None, family_name = None, port = None):
        """
        Initializes the QPU class.

        Args:
        -----------
        id (int): Id assigned to the qpu, simply a int from 0 to n_qpus-1.

        qclient (<class 'python.qclient.QClient'>): object that holds the information to communicate with the server
            endpoint for a given QPU.
            
        backend (<class 'backend.Backend'>): object that provides information about the QPU backend.

        port (str): String refering to the port of the server to which the QPU corresponds.
        """
        
        if id == None:
            logger.error(f"QPU id not provided [{TypeError.__name__}].")
            raise SystemExit # User's level
            
        elif type(id) == int:
            self.id = id

        else:
            logger.error(f"QPU id must be int, but {type(id)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level


        if qclient == None:
            logger.error(f"QPU client not assigned [{TypeError.__name__}].")
            raise SystemExit # User's level
            
        elif isinstance(qclient, QClient):
            self._qclient = qclient

        else:
            logger.error(f"QPU qclient must be <class 'python.qclient.QClient'>, but {type(qclient)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level


        if backend == None:
            logger.error(f"QPU backend not provided [{TypeError.__name__}].")
            raise SystemExit # User's level
            
        elif isinstance(backend, Backend):
            self.backend = backend

        else:
            logger.error(f"QPU backend must be <class 'backend.Backend'>, but {type(backend)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level
        
        if port == None:
            logger.error(f"QPU client not assigned [{TypeError.__name__}].") # for staters we raise the same error as if qclient was not provided
            raise SystemExit # User's level
        
        elif isinstance(port, str):
            self._port = port

        else:
            logger.error(f"QClient port must be str, but {type(port)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level
        
        if family_name == None:
            logger.error(f"Please provide QPU family name [{TypeError.__name__}].") # for staters we raise the same error as if qclient was not provided
            raise SystemExit # User's level
        
        elif isinstance(port, str):
            self._family_name = family_name

        else:
            logger.error(f"Family name must be str, but {type(family_name)} was provided [{TypeError.__name__}].")
            raise SystemExit # User's level


        # argument to track weather the QPU is connected. It will be connected at `run` method.
        self.connected = False
        
        logger.debug(f"Object for QPU {id} created correctly.")


    def run(self, circuit, transpile = False, initial_layout = None, opt_level = 1, **run_parameters):
        """
        Class method to run a circuit in the QPU.

        It is important to note that  if `transpilation` is set False, we asume user has already done the transpilation, otherwise some errors during the simulation
        can occur, for example if the QPU has a noise model with error associated to specific gates, if the circuit is not transpiled errors might not appear.

        If `transpile` is False and `initial_layout` is provided, it will be ignored.

        Possible instructions to add as `**run_parameters` can be: shots, method, parameter_binds, meas_level, ...

        Args:
        --------
        circuit (json dict, <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or QASM2 str): circuit to be run in the QPU.

        transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

        initial_layout (list[int]): Initial position of virtual qubits on physical qubits for transpilation.

        opt_level (int): optimization level for transpilation, default set to 1.

        **run_parameters : any other simulation instructions.

        Return:
        --------
        <class 'qjob.Result'> object.
        """

        if not self.connected:
            self._qclient.connect(self._port)
            logger.debug(f"QClient connection stabished for QPU {self.id} to port {self._port}.")
        else:
            logger.debug(f"QClient already connected for QPU {self.id} to port {self._port}.")

        try:
            qjob = QJob(self, circuit, transpile = transpile, initial_layout = initial_layout, opt_level = opt_level, **run_parameters)
            qjob.submit()
            logger.debug(f"Qjob submitted to QPU {self.id}.")
        except Exception as error:
            logger.error(f"Error when submitting QJob [{type(error).__name__}].")
            raise SystemExit # User's level

        return qjob


class QFamily:
    """
    Class to represent a group of QPUs that where raised in the same job.
    """
    def __init__(self, name = None, jobid = None):
        """
        Initializes the QFamily class.
        """

        if name is None:
            logger.error("No family name provided.")
            raise ValueError # capture this in qraise
        
        elif isinstance(name, str):
            self.name = name

        else:
            logger.error(f"QFamily name must be str, but {type(name)} was provided [{TypeError.__name__}].")
        
        if jobid is None:
            logger.error("No family name provided.")
            raise ValueError # capture this in qraise
        
        elif isinstance(name, str):
            try:
                self.jobid = int(jobid)
            except Exception as error:
                logger.error(f"Incorrect format for jobid [{type(error).__name__}].")

        else:
            logger.error(f"QFamily name must be str, but {type(name)} was provided [{TypeError.__name__}].")

        
