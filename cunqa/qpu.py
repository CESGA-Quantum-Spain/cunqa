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

    Connecting to virtual QPUs
    ==========================
    The most important function of the submodule, and one of the most important of :py:mod:`cunqa` is the :py:func:`get_QPUs`
    function, since it creates the objects that allow sending circuits and receiving the results of the simulations from the
    virtual QPUs already deployed:

        >>> from cunqa import get_QPUs
        >>> get_QPUs()
        [<cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>]

    The function allows to filter by `family` name or by choosing the virtual QPUs available at the `local` node.

    When each :py:class:`~cunqa.qpu.QPU` is instanciated, the corresponding :py:class:`QClient` is created.
    Nevertheless, it is not until the first job is submited that the client actually connects to the correspoding server.
    Other properties and information gathered in the :py:class:`~cunqa.qpu.QPU` class are shown on its documentation.

    Q-raising and Q-dropping at pyhton
    ==================================
    This submodule allows to raise and drop virtual QPUs, with no need to work in the command line.
    One must provide the neccesary information, analogous to the ``qraise`` command:

        >>> qraise(n = 4, # MANDATORY, number of QPUs to be raised
        >>> ...    t = "2:00:00", # MANDATORY, maximum time until they are automatically dropped
        >>> ...    classical_comm = True, # allow classical communications
        >>> ...    simulator = "Aer", # choosing Aer simulator
        >>> ...    co_located = True, # allowing co-located mode, QPUs can be accessed from any node
        >>> ...    family = "my_family_of_QPUs" # assigning a name to the group of QPUs
        >>> ...    )
        '<job id>'

    The function :py:func:`qraise` returns a string specifying the id of the `SLURM <https://slurm.schedmd.com/documentation.html>`_
    job that deploys the QPUs.

    Once we are finished with our work, we should drop the virtual QPUs in order to release classical resources.
    Knowing the `job id` or the `family` name of the group of virtual QPUs:

        >>> qdrop('<job id>')

    If no argument is passed to :py:func:`qdrop`, all QPUs deployed by the user are dropped.

    .. warning::
        The :py:func:`qraise` function can only be used when the python program is being run at a login node, otherwise an error will be raised.
        This is because SLURM jobs can only be submmited from login nodes, but not from compute sessions or running jobs.

    .. note::
        Even if we did not raise the virtual QPUs by the :py:func:`qraise` function, we can still use :py:func:`qdrop` to cancel them.
        In the same way, if we raise virtual QPUs by the python function, we can still drop them by terminal commands.
    

    Obtaining information about virtual QPUs
    ========================================

    In some cases we might be interested on checking availability of virtual QPUs or getting information about them, but before creating
    the :py:class:`~cunqa.qpu.QPU` objects.

    - To check if certain virtual QPUs are raised, :py:func:`are_QPUs_raised` should be used:
         
        >>> are_QPUs_raised(family = 'my_family_of_QPUs')
        True
        
    - In order to know what nodes have available virtual QPUs deployed:

        >>> nodes_with_QPUs()
        ['c7-3', 'c7-4']

    - For obtaining information about QPUs in the local node or in other nodes:

        >>> info_QPUs(on_node = True)
        [{'QPU':'<id>',
          'node':'<node name>',
          'family':'<family name>',
          'backend':{···}
          }]

"""

import os
import time
import inspect
import json

from typing import Union, Any, Optional
from subprocess import run

from qiskit import QuantumCircuit

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit
from cunqa.backend import Backend
from cunqa.qjob import QJob
from cunqa.logger import logger
from cunqa.transpile import transpiler, TranspileError
from cunqa.constants import QPUS_FILEPATH

class QPU:
    """
    Class to represent a virtual QPU deployed for user interaction.

    This class contains the neccesary data for connecting to the virtual QPU's server in order to communicate circuits and results in both ways.
    This communication is stablished trough the :py:attr:`QPU.qclient`.

    """
    _id: int 
    _qclient: 'QClient' 
    _backend: 'Backend'
    _name: str 
    _family: str
    _endpoint: str 
    _connected: bool 
    
    def __init__(self, id: int, qclient: 'QClient', backend: Backend, name: str, family: str, endpoint: str):
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
        self._name = name
        self._family = family
        self._endpoint = endpoint
        self._connected = False
        
        logger.debug(f"Object for QPU {id} created correctly.")

    @property
    def id(self) -> int:
        """Id string assigned to the object."""
        return self._id
    
    @property
    def name(self) -> str:
        return self._name
    
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
            self._qclient.connect(self._endpoint)
            self._connected = True
            logger.debug(f"QClient connection stabished for QPU {self._id} to endpoint {self._endpoint}.")
            self._connected = True

        # Transpilation if requested
        if transpile:
            try:
                #logger.debug(f"About to transpile: {circuit}")
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


class QRaiseError(Exception):
    """Exception for errors during qraise slurm command."""
    pass
               
               
def qraise(n, t, *, 
           classical_comm = False, 
           quantum_comm = False,  
           simulator = None, 
           backend = None, 
           fakeqmio = False, 
           calibrations = None,
           family = None, 
           co_located = True, 
           cores = None, 
           mem_per_qpu = None, 
           n_nodes = None, 
           node_list = None, 
           qpus_per_node= None) -> Union[tuple, str]:
    """
    Raises virtual QPUs and returns the job id associated to its SLURM job.

    This function allows to raise QPUs from the python API, what can also be done at terminal by ``qraise`` command.

    Args:
        n (int): number of virtual QPUs to be raised in the job.

        t (str): maximun time that the classical resources will be reserved for the job. Format: 'D-HH:MM:SS'.

        classical_comm (bool): if ``True``, virtual QPUs will allow classical communications.

        quantum_comm (bool): if ``True``, virtual QPUs will allow quantum communications.

        simulator (str): name of the desired simulator to use. Default is `Aer <https://github.com/Qiskit/qiskit-aer>`_.

        backend (str): path to a file containing backend information.

        fakeqmio (bool): ``True`` for raising `n` virtual QPUs with FakeQmio backend.

        family (str): name to identify the group of virtual QPUs raised.

        co_located (bool): if ``True``, `co-located` mode is set, otherwise `hpc` mode is set. In `hpc` mode, virtual QPUs can only be accessed from the node in which they are deployed. In `co-located` mode, they can be accessed from other nodes.

        cores (str):  number of cores per virtual QPU, the total for the SLURM job will be `n*cores`.

        mem_per_qpu (str): memory to allocate for each virtual QPU in GB, format to use is  "xG".

        n_nodes (str): number of nodes for the SLURM job.

        node_list (str): list of nodes in which the virtual QPUs will be deployed.

        qpus_per_node (str): sets the number of virtual QPUs deployed on each node.
    
    Returns:
        The SLURM job id of the job deployed. If `family` was provided, a tuple (`family`, `job id`).

    .. warning::
        The :py:func:`qraise` function can only be used when the python program is being run at a login node, otherwise an error will be raised.
        This is because SLURM jobs can only be submmited from login nodes, but not from compute sessions or running jobs.
    """
    logger.debug("Setting up the requested QPUs...")

    SLURMD_NODENAME = os.getenv("SLURMD_NODENAME")

    if SLURMD_NODENAME == None:
        command = f"qraise -n {n} -t {t}"
    else: 
        logger.warning("Be careful, you are deploying QPUs from an interactive session.")
        HOSTNAME = os.getenv("HOSTNAME")
        command = f"qraise -n {n} -t {t}"

    try:
        # Add specified flags
        if fakeqmio:
            command = command + " --fakeqmio"
        if classical_comm:
            command = command + " --classical_comm"
        if quantum_comm:
            command = command + " --quantum_comm"
        if simulator is not None:
            command = command + f" --simulator={str(simulator)}"
        if family is not None:
            command = command + f" --family_name={str(family)}"
        if co_located:
            command = command + " --co-located"
        if cores is not None:
            command = command + f" --cores={str(cores)}"
        if mem_per_qpu is not None:
            command = command + f" --mem-per-qpu={str(mem_per_qpu)}G"
        if n_nodes is not None:
            command = command + f" --n_nodes={str(n_nodes)}"
        if node_list is not None:
            command = command + f" --node_list={str(node_list)}"
        if qpus_per_node is not None:
            command = command + f" --qpus_per_node={str(qpus_per_node)}"
        if backend is not None:
            command = command + f" --backend={str(backend)}"

        if not os.path.exists(QPUS_FILEPATH):
           with open(QPUS_FILEPATH, "w") as file:
                file.write("{}")

        print(f"Command: {command}")

        output = run(command, capture_output=True, shell=True, text=True).stdout #run the command on terminal and capture its output on the variable 'output'
        logger.info(output)
        job_id = ''.join(e for e in str(output) if e.isdecimal()) #sees the output on the console (looks like 'Submitted sbatch job 136285') and selects the number
        
        cmd_getstate = ["squeue", "-h", "-j", job_id, "-o", "%T"]
        
        i = 0
        while True:
            state = run(cmd_getstate, capture_output=True, text=True, check=True).stdout.strip()
            if state == "RUNNING":
                try:     
                    with open(QPUS_FILEPATH, "r") as file:
                        data = json.load(file)
                except json.JSONDecodeError:
                    continue
                count = sum(1 for key in data if key.startswith(job_id))
                if count == n:
                    break
            # We do this to prevent an overload to the Slurm deamon through the 
            if i == 500:
                time.sleep(2)
            else:
                i += 1

        # Wait for QPUs to be raised, so that get_QPUs can be executed inmediately
        print("QPUs ready to work \U00002705")

        return (family, str(job_id)) if family is not None else str(job_id)
    
    except subprocess.CalledProcessError as error:
        logger.error(f"An error was encoutered while qraising:\n {error.stderr}.")
        raise QRaiseError

def qdrop(*families: Union[tuple[str], str]):
    """
    Drops the virtual QPU families corresponding to the the input family names.
    If no families are provided, all virtual QPUs deployed by the user will be dropped.

    Args:
        families (str): family names of the groups of virtual QPUs to be dropped.
    """
    
    # Building the terminal command to drop the specified families
    cmd = ['qdrop']

    # If no QPU is provided we drop all QPU slurm jobs
    if len( families ) == 0:
        cmd.append('--all') 
    else:
        for family in families:
            if isinstance(family, str):
                cmd.append(family)

            elif isinstance(family, tuple):
                cmd.append(family[1])
            else:
                logger.error(f"Invalid type for qdrop.")
                raise SystemExit
    
 
    run(cmd) #run 'qdrop slurm_jobid_1 slurm_jobid_2 etc' on terminal

def nodes_with_QPUs() -> "list[str]":
    """
    Provides information about the nodes in which virtual QPUs are available.

    Return:
        List of the corresponding node names.
    """
    try:
        with open(QPUS_FILEPATH, "r") as f:
            qpus_json = json.load(f)

        node_names = set()
        for info in qpus_json.values():
            node_names.add(info["net"]["node_name"])

        return list(node_names)

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level

def info_QPUs(on_node: bool = True, node_name: Optional[str] = None) -> "list[dict]":
    """
    Provides information about the virtual QPUs available either in the local node, an specific node or globally.

    If `on_node` is ``True`` and `node_name` provided is different from the local node, only information at local node will be displayed.
    
    Args:
        on_node (bool): if ``True`` information at local node is displayed, else all information is displayed.

        node_name (str): filters the displayed information by an specific node.

    Returns:
        A list with :py:class:`dict` objects that display the information of the virtual QPUs.
    
    """

    try:
        with open(QPUS_FILEPATH, "r") as f:
            qpus_json = json.load(f)
            if len(qpus_json) == 0:
                logger.warning(f"No QPUs were found.")
                return [{}]
        
        if node_name is not None:
            targets = [{qpu_id:info} for qpu_id,info in qpus_json.items() if (info["net"].get("node_name") == node_name ) ]
        
        else:
            if on_node:
                local_node = os.getenv("SLURMD_NODENAME")
                if local_node != None:
                    logger.debug(f"User at node {local_node}.")
                else:
                    logger.debug(f"User at a login node.")
                targets = [{qpu_id:info} for qpu_id,info in qpus_json.items() if (info["net"].get("node_name")==local_node) ]
            else:
                targets =[{qpu_id:info} for qpu_id,info in qpus_json.items()]
        
        info = []
        for t in targets:
            key = list(t.keys())[0]
            info.append({
                "QPU":key,
                "node":t[key]["net"]["node_name"],
                "family":t[key]["family"],
                "backend":{
                    "name":t[key]["backend"]["name"],
                    "simulator":t[key]["backend"]["simulator"],
                    "version":t[key]["backend"]["version"],
                    "description":t[key]["backend"]["description"],
                    "n_qubits":t[key]["backend"]["n_qubits"],
                    "basis_gates":t[key]["backend"]["basis_gates"],
                    "coupling_map":t[key]["backend"]["coupling_map"],
                    "custom_instructiona":t[key]["backend"]["custom_instructions"]
                }
            })
        return info

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise error # User's level

def get_QPUs(on_node: bool = True, family: Optional[Union[tuple, str]] = None) -> "list['QPU']":
    """
    Returns :py:class:`~cunqa.qpu.QPU` objects corresponding to the virtual QPUs raised by the user.

    Args:
        on_node (bool): if ``True``, filters by the virtual QPUs available at the local node.
        family (str): filters virtual QPUs by their family name.

    Return:
        List of :py:class:`~cunqa.qpu.QPU` objects.
    
    """

    if isinstance(family, tuple): family = family[0]

    # access raised QPUs information on qpu.json file
    try:
        with open(QPUS_FILEPATH, "r") as f:
            qpus_json = json.load(f)
            if len(qpus_json) == 0:
                logger.error(f"No QPUs were found.")
                raise SystemExit

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level
    
    logger.debug(f"File accessed correctly.")

    # extract selected QPUs from qpu.json information 
    local_node = os.getenv("SLURMD_NODENAME")
    if local_node != None:
        logger.debug(f"User at node {local_node}.")
    else:
        logger.debug(f"User at a login node.")
    if on_node:
        if family is not None:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info["net"].get("nodename") == local_node) and (info.get("family") == family)}
        else:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info["net"].get("nodename") == local_node)}
    else:
        if family is not None:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if ((info["net"].get("nodename") == local_node) or (info["net"].get("nodename") != local_node and info["net"].get("mode") == "co_located")) and (info.get("family") == family)}
        else:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info["net"].get("nodename") == local_node) or (info["net"].get("nodename") != local_node and info["net"].get("mode") == "co_located")}
    
    # create QPU objects from the dictionary information + return them on a list
    qpus = []
    for id, info in targets.items():
        client = QClient()
        endpoint = info["net"]["endpoint"]
        name = info["name"]
        qpus.append(QPU(id = id, qclient = client, backend = Backend(info['backend']), name = name, family = info["family"], endpoint = endpoint))
    if len(qpus) != 0:
        logger.debug(f"{len(qpus)} QPU objects were created.")
        return qpus
    else:
        logger.error(f"No QPUs where found with the characteristics provided: on_node={on_node}, family_name={family}.")
        raise SystemExit


