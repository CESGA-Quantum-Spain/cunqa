"""
    Holds functions to manage and get infomation about the virtual QPUs displayed by the user.

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
import sys
import time
from typing import Union, Optional
from subprocess import run
from json import load
import json

from cunqa.qclient import QClient
from cunqa.real_qpus.qmioclient import QMIOClient
from cunqa.backend import Backend
from cunqa.logger import logger
from cunqa.qpu import QPU
from cunqa.constants import QPUS_FILEPATH

class QRaiseError(Exception):
    """Exception for errors during qraise slurm command."""
    pass
               
               
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
           partition = None,
           qmio = None) -> Union[tuple, str]:
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
        if partition is not None:
            command = command + f" --partition={str(partition)}"
        if qmio is not None:
            command = command + " --qmio"


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
    
    except Exception as error:
        raise QRaiseError(f"Unable to raise requested QPUs [{error}].")

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
            qpus_json = load(f)

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
            qpus_json = load(f)
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
            qpus_json = load(f)
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
        is_real_qpu = False
        if "real_qpu" in info:
            logger.debug("Real QPU found")
            client = QMIOClient()
            is_real_qpu = True
        else:
            logger.debug("Virtual QPU found") 
            client = QClient()
        
        qpus.append(QPU(id = id, qclient = client, backend = Backend(info['backend']), name = info["name"], family = info["family"], endpoint = info["net"]["endpoint"], real_qpu = is_real_qpu))
    if len(qpus) != 0:
        return qpus
    else:
        logger.error(f"No QPUs where found with the characteristics provided: on_node={on_node}, family_name={family}.")
        raise SystemExit


