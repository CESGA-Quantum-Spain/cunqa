"""
    Contains the description of the :py:class:`~cunqa.qpu.QPU` class and the functions to manage
    the virtual QPUs (vQPUs).

    First, these :py:class:`QPU` objects are the python representation of the vQPUs deployed.
    Each one has its :py:class:`QClient` object that communicates with the server of the 
    corresponding virtual QPU. Through these objects we are able to send circuits and recieve 
    results from simulations.

    Virtual QPUs
    ============
    Each virtual QPU is described by three elements:

    - **Server**: classical resources intended to communicate with the python API to recieve 
      circuits or quantum tasks and to send results of the simulations.
    - **Backend**: characteristics that define the QPU that is emulated: coupling map, 
      basis gates, noise model, etc.
    - **Simulator**: classical resources intended to simulate circuits accordingly to 
      the backend characteristics.
    
    .. image:: /_static/virtualqpuequal.png
        :align: center
        :width: 400px
        :height: 200px

    In order to stablish communication with the server, :py:class:`QPU` objects 
    are created, each one of them associated with a virtual QPU. Each object will have a 
    :py:class:`QClient` C++ object through which the communication with the classical resoruces 
    is performed.
    
    .. image:: /_static/client-server-comm.png
        :align: center
        :width: 150
        :height: 300px

    Connecting to virtual QPUs
    ==========================
    This submodule, as hinted before, gathers the functions related with the management and 
    interaction with the vQPUs. For obtaining information about the available vQPUs the 
    :py:func:`~cunqa.qpu.get_QPUs` function returns a list of :py:class:`QPU` objects with the 
    desired filtering:

    >>> from cunqa import get_QPUs
    >>> get_QPUs()
    [<cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>, <cunqa.qpu.QPU object at XXXX>]

    When each :py:class:`QPU` is instanciated, the corresponding :py:class:`QClient` is created and 
    connected to the endpoint that corresponds to the server of the vQPU. This way, the 
    :py:class:`QPU` is ready to submit jobs and obtain their results from the connected vQPU. 
    Other properties and information gathered in the :py:class:`QPU` class are shown on its 
    documentation.

    It is important to note that the `get_QPUs` function has 

    Interacting with virtual QPUs
    =============================
    Once we have the :py:class:`QPU` objects created, we can start interacting with them. The most 
    important function is :py:func:`~cunqa.qpu.run`, which allows to send a circuit 
    for its simulation into the virtual QPUs indicated, returning a :py:class:`~cunqa.qjob.QJob` 
    object associated with the quantum task:

        >>> qpus = get_QPUs()
        >>> qpu = qpus[0]
        >>> run(circuit, qpu)
        <cunqa.qjob.QJob object at XXXX>

    This method takes several arguments for specifications of the simulation such as `shots`. For a 
    larger description of its functionalities checkout its documentation.

    `qraise` and `qdrop`
    ==================================
    This submodule allows to raise and drop virtual QPUs, with no need to use the command line.
    One must provide the neccesary information, analogous to the ``qraise`` command:

        >>> qraise(n = 4, # MANDATORY, number of QPUs to be raised
        >>> ...    t = "2:00:00", # MANDATORY, maximum time until they are automatically dropped
        >>> ...    classical_comm = True, # allow classical communications
        >>> ...    simulator = "Aer", # choosing Aer simulator
        >>> ...    co_located = True, # allowing co-located mode, QPUs can be accessed from any node
        >>> ...    family = "my_family_of_QPUs" # assigning a name to the group of QPUs
        >>> ...    )
        '<family name>'

    The function :py:func:`qraise` returns a string specifying the family name of the vQPUs deployed.

    Once we are finished with our work, we should drop the virtual QPUs in order to release 
    classical resources. Knowing the `family` name of the group of virtual QPUs:

        >>> qdrop('<family name>')

    If no argument is passed to :py:func:`qdrop`, all QPUs deployed by the user are dropped.

    .. note::
        Even if we did not raise the vQPUs by the :py:func:`qraise` function, we can still use 
        :py:func:`qdrop` to cancel them. In the same way, if we raise virtual QPUs by the python 
        function, we can still drop them by terminal commands. Python and bash functionalities are
        not exclusive.

"""

import os
import time
import json
import subprocess
from typing import Union, Any, Optional

from qiskit import QuantumCircuit

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit
from cunqa.qiskit_deps.cunqabackend import CunqaBackend
from cunqa.circuit import CunqaCircuit, to_ir
from cunqa.backend import Backend
from cunqa.qjob import QJob
from cunqa.logger import logger
from cunqa.constants import QPUS_FILEPATH, REMOTE_GATES

class QPU:
    """
    Class to represent a virtual QPU deployed for user interaction.

    This class contains the neccesary data for connecting to the virtual QPU's server in order to 
    communicate circuits and results in both ways. This communication is stablished trough 
    the :py:attr:`QPU.qclient`.

    """
    _id: int 
    _qclient: QClient 
    _backend: Backend
    _name: str 
    _family: str
    _endpoint: str 
    
    def __init__(self, id: int, backend: Backend, family: str, endpoint: str):
        """
        Initializes the :py:class:`QPU` class.

        This initialization of the class is normally done by the :py:func:`~cunqa.qpu.get_QPUs` 
        function, which loads the `id`, `family` and `endpoint`, and instanciates the `backend` 
        objects. It could also be manually initialized by the user, but this is not recommended.

        Args:
            id (str): id string assigned to the object.
                
            backend (~cunqa.backend.Backend): object that provides the characteristics that the 
            simulator at the virtual QPU uses to emulate a real device.

            family (str):  name of the family to which the corresponding virtual QPU belongs.
            
            endpoint (str): string refering to the endpoint of the corresponding virtual QPU.
        """
        
        self._id = id
        self._backend = backend
        self._family = family
        
        self._qclient = QClient()
        self._qclient.connect(endpoint)

        logger.debug(f"Object for QPU {id} created and connected to endpoint {endpoint}.")

    @property
    def id(self) -> int:
        """Id string assigned to the QPU."""
        return self._id
    
    @property
    def backend(self) -> Backend:
        """Object that provides the characteristics that the simulator at the virtual QPU uses to 
        emulate a real device."""
        return self._backend

    def execute(self, circuit_ir: dict, **run_parameters: Any) -> QJob:
        """
        Class method to execute a circuit into the corresponding virtual QPU that this class 
        connects to. Possible instructions to add as `**run_parameters` are simulator dependant, 
        but mainly `shots` and `method` are used.

        Args:
            circuit_ir (dict): circuit IR to be simulated at the virtual QPU.

            **run_parameters: any other simulation instructions.

        Return:
            A :py:class:`~cunqa.qjob.QJob` object related to the job sent.
        """
        qjob = QJob(self._qclient, circuit_ir, **run_parameters)
        qjob.submit()
        logger.debug(f"Qjob submitted to QPU {self._id}.")

        return qjob


def run(
        circuits: Union[list[Union[dict, QuantumCircuit, CunqaCircuit]], Union[dict, QuantumCircuit, CunqaCircuit]], 
        qpus: Union[list[QPU], QPU], 
        **run_args: Any
    ) -> Union[list[QJob], QJob]:
    """
    Function to send circuits to several virtual QPUs. Each circuit will be sent to each QPU in 
    order, therefore, both lists should be the same size. If they are not, but the number of circuits
    is less than the numbers of QPUs, the circuit will get executed. In case the number of QPUs is 
    less than the number of circuits is an error will be raised. 

    Args:
        circuits (list[dict | ~cunqa.circuit.CunqaCircuit | ~qiskit.QuantumCircuit] | dict |
                  ~cunqa.circuit.CunqaCircuit | ~qiskit.QuantumCircuit): circuits to be run.

        qpus (list[~cunqa.qpu.QPU] | ~cunqa.qpu.QPU): QPU objects associated to the virtual QPUs in 
        which the circuits want to be run.
    
        run_args: any other run arguments and parameters.

    Return:
        List of :py:class:`~cunqa.qjob.QJob` objects.
    """

    if isinstance(circuits, list):
        circuits_ir = [to_ir(circuit) for circuit in circuits]
    else:
        circuits_ir = [to_ir(circuits)]

    if not isinstance(qpus, list):
        qpus = [qpus]

    # check wether there are enough qpus and create an allocation dict that for every 
    # circuit id has the info of the QPU to which it will be sent
    if len(circuits_ir) > len(qpus):
        raise ValueError(f"There are not enough QPUs: {len(circuits)} circuits were given," 
                         f"but only {len(qpus)} QPUs [{ValueError.__name__}].")
    elif len(circuits_ir) < len(qpus):
        logger.warning("More QPUs provided than the number of circuits. "
                       "Last QPUs will remain unused.")
    
    # translate circuit ids in comm instruction to qpu endpoints
    correspondence = {circuit["id"]: qpu._id for circuit, qpu in zip(circuits_ir, qpus)}
    for circuit in circuits_ir:
        if circuit["has_cc"] or circuit["has_qc"]:
            for instr in circuit["instructions"]:
                if instr["name"] in REMOTE_GATES:
                    instr["qpus"] =  [correspondence[instr["circuits"][0]]]
                    instr.pop("circuits")
            circuit["sending_to"] = [correspondence[target_circuit] 
                                     for target_circuit in circuit["sending_to"]]
            circuit["id"] = correspondence[circuit["id"]]

    run_parameters = {k: v for k, v in run_args.items()}
    qjobs = [qpu.execute(circuit, **run_parameters) for circuit, qpu in zip(circuits_ir, qpus)]

    if len(circuits_ir) == 1:
        return qjobs[0]
    return qjobs
            
def qraise(n, t, *, 
           classical_comm = False, 
           quantum_comm = False,  
           simulator = None, 
           backend = None, 
           fakeqmio = False, 
           family = None, 
           co_located = True, 
           cores = None, 
           mem_per_qpu = None, 
           n_nodes = None, 
           node_list = None, 
           qpus_per_node= None,
           partition=None
        ) -> str:
    """
    Raises virtual QPUs and returns the family name associated them. This function allows to raise 
    QPUs from the python API, what can also be done at terminal by ``qraise`` command.

    Args:
        n (int): number of virtual QPUs to be raised in the job.

        t (str): maximun time that the classical resources will be reserved for the job. Format: 
                 'D-HH:MM:SS'.

        classical_comm (bool): if ``True``, virtual QPUs will allow classical communications.

        quantum_comm (bool): if ``True``, virtual QPUs will allow quantum communications.

        simulator (str): name of the desired simulator to use. Default is `Aer 
                         <https://github.com/Qiskit/qiskit-aer>`_.

        backend (str): path to a file containing the backend information.

        fakeqmio (bool): ``True`` for raising `n` virtual QPUs with FakeQmio backend. Only available 
                         at CESGA.

        family (str): name to identify the group of virtual QPUs raised.

        co_located (bool): if ``True``, `co-located` mode is set, otherwise `hpc` mode is set. In 
                           `hpc` mode, virtual QPUs can only be accessed from the node in which they 
                           are deployed. In `co-located` mode, they can be accessed from other nodes.

        cores (str): number of cores per virtual QPU, the total for the SLURM job will be 
                     `n*cores`.

        mem_per_qpu (str): memory to allocate for each virtual QPU in GB, format to use is "XXG".

        n_nodes (str): number of nodes for the SLURM job.

        node_list (str): list of nodes in which the virtual QPUs will be deployed.

        qpus_per_node (str): sets the number of virtual QPUs deployed on each node.

        partition (str): partition of the nodes in which the QPUs are going to be executed.
    
    Returns:
        The family name of the job deployed.
    """
    logger.debug("Setting up the requested QPUs...")
    command = f"qraise -n {n} -t {t}"

    try:
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
        if partition is not None:
            command = command + f" --partition={str(partition)}"

        if not os.path.exists(QPUS_FILEPATH):
           with open(QPUS_FILEPATH, "w") as file:
                file.write("{}")

        print(f"Requested QPUs with command:\n\t{command}")

        #run the command on terminal and capture its output on the variable 'output'
        output = subprocess.run(command, capture_output=True, shell=True, text=True).stdout.rstrip("\n")

        #sees the output on the console and selects the job_id
        job_id = output.split(";", 1)[0]

        cmd_getstate = ["squeue", "-h", "-j", job_id, "-o", "%T"]
        
        i = 0
        while True:
            state = subprocess.run(
                cmd_getstate, 
                capture_output = True, 
                text = True, 
                check = True
            ).stdout.strip()
            if state == "RUNNING":
                try:     
                    with open(QPUS_FILEPATH, "r") as file:
                        data = json.load(file)
                except json.JSONDecodeError:
                    continue
                count = sum(1 for key in data if key.startswith(job_id))
                if count == n:
                    break
            # We do this to prevent an overload of the Slurm deamon 
            if i == 500:
                time.sleep(2)
            else:
                i += 1

        # Wait for QPUs to be raised, so that get_QPUs can be executed inmediately
        print("QPUs ready to work \U00002705")

        return family if family is not None else str(job_id)
    
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"An error was encoutered while qraising:\n {error.stderr}.")

def qdrop(*families: str):
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
        cmd.append('--fam')
        for family in families:
            cmd.append(family)
 
    subprocess.run(cmd) #run 'qdrop slurm_jobid_1 slurm_jobid_2 etc' on terminal

def get_QPUs(co_located: bool = False, family: Optional[str] = None) -> list[QPU]:
    """
    Returns :py:class:`~cunqa.qpu.QPU` objects corresponding to the virtual QPUs raised by the user.

    Args:
        co_located (bool): if ``False``, filters by the virtual QPUs available at the local node.
        family (str): filters virtual QPUs by their family name.

    Return:
        List of :py:class:`~cunqa.qpu.QPU` objects.
    
    """
    # access raised QPUs information on qpu.json file
    with open(QPUS_FILEPATH, "r") as f:
        qpus_json = json.load(f)
        if len(qpus_json) == 0:
            logger.warning(f"No QPUs were found.")
            return None

    # extract selected QPUs from qpu.json information 
    local_node = os.getenv("SLURMD_NODENAME")
    if co_located:
        targets = {
            qpu_id: info
            for qpu_id, info in qpus_json.items()
            if ((info["net"].get("nodename") == local_node or info["net"].get("mode") == "co_located") and 
                (family is None or info.get("family") == family))
        }
    else:
        if local_node is None:
            logger.warning("You are searching for QPUs in a login node, none are found.")
            return None
        else:
            targets = {
                qpu_id: info
                for qpu_id, info in qpus_json.items()
                if ((info["net"].get("nodename") == local_node) and 
                    (family is None or info.get("family") == family))
            }
    
    qpus = [
        QPU(
            id = id,
            backend = Backend(info['backend']),
            family = info["family"],
            endpoint = info["net"]["endpoint"]
        ) for id, info in targets.items()
    ]
        
    if len(qpus) != 0:
        logger.debug(f"{len(qpus)} QPU objects were created.")
        return qpus
    else:
        logger.warning(f"No QPUs where found with the characteristics provided: "
                       f"co_located={co_located}, family_name={family}.")
        return None



