""" Holds functions that manage virtual QPUs or provide information about them."""
import os
import sys
from typing import Union
from subprocess import run
from json import load
from cunqa.qclient import QClient  # importamos api en C++
from cunqa.backend import Backend
from cunqa.logger import logger
from cunqa.qpu import QPU

# Adding pyhton folder path to detect modules
sys.path.append(os.getenv("HOME"))

info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.cunqa/qpus.json"

class QRaiseError(Exception):
    """Exception for errors during qraise slurm command"""
    pass


def qraise(n, time, *, 
           simulator = None, 
           fakeqmio = False, 
           family = None, 
           cloud = True, 
           cores = None, 
           mem_per_qpu = None, 
           n_nodes = None, 
           node_list = None, 
           qpus_per_node= None, 
           backend = None) -> Union[tuple, str]:
    """
    Raises a QPU and returns its job_id.

    Args
        n (int): number of QPUs to be raised.
        time (str, format: 'D-HH:MM:SS'): maximun time that the classical resources will be reserved for the QPU.
        
        fakeqmio (bool): if True the raised QPUs will have the fakeqmio backend.
        classical_comm (bool): if True the raised QPUs are communicated classically.
        quantum_comm (bool): if True the raised QPUs have quantum communications.
        simulator (str): name of the desired simulator to use. Default in this branch is Cunqasimulator.
        family (str): name to identify the group of QPUs raised on the specific call of the function.
        cloud (str): with this option multiple QPUs can fall on the same node, in contrast to the deafult 'hpc' mode where QPUs are treated as GPUs each associated to a node.
        cores (str):  number of cores for the SLURM job.
        mem_per_qpu (str): memory to allocate for each QPU, with format "xG", x being your number.
        n_nodes (str): number of nodes for the SLURM job.
        node_list (str): option to select specifically on which nodes the simulation job should run.
        qpus_per_node (str): sets the number of QPUs that should be raised on each requested node.
        backend (str): path to a file containing backend information.

    """

    try:
        cmd = ["qraise", "-n", str(n), '-t', str(time)]

        # Add specified flags
        if fakeqmio:
            cmd.append(f"--fakeqmio")
        if simulator is not None:
            cmd.append(f"--simulator={str(simulator)}")
        if family is not None:
            cmd.append(f"--family={str(family)}")
        if cloud:
            cmd.append(f"--cloud")
        if cores is not None:
            cmd.append(f"--cores={str(cores)}")
        if mem_per_qpu is not None:
            cmd.append(f"--mem_per_qpu={str(mem_per_qpu)}")
        if n_nodes is not None:
            cmd.append(f"--n_nodes={str(n_nodes)}")
        if node_list is not None:
            cmd.append(f"--node_list={str(node_list)}")
        if qpus_per_node is not None:
            cmd.append(f"--qpus_per_node={str(qpus_per_node)}")
        if backend is not None:
            cmd.append(f"--backend={str(backend)}")
        
        output = run(cmd, capture_output=True, text=True).stdout #run the command on terminal and capture ist output on the variable 'output'
        job_id = ''.join(e for e in str(output) if e.isdecimal()) #sees the output on the console (looks like 'Submitted batch job 136285') and selects the number
        
        if 'error' in output: 
            raise QRaiseError
        return family if family is not None else int(job_id)
    
    except Exception as error:
        raise QRaiseError(f"Unable to raise requested QPUs [{error}].")

def qdrop(*families: Union[tuple, str]):
    """
    Drops the QPU families corresponding to the the entered QPU objects. By default, all raised QPUs will be dropped.

    Args
        qpus (tuple(<class cunqa.qpu.QPU>)): list of QPUs to drop. All QPUs that share a qraise will these will drop.
    """
    
    #if no QPU is provided we drop all QPU slurm jobs
    if len( families ) == 0:
        job_id = ['--all'] 

    #access the large dictionary containing all QPU dictionaries
    try:
        with open(info_path, "r") as f:
            qpus_json = load(f)

    except Exception as error:
        logger.error(f"Some exception occurred while retrieving the raised QPUs [{type(error).__name__}].")
        raise SystemExit # User's level
    
    logger.debug(f"qpu.json file accessed correctly.")

    #building the terminal command to drop the specified families (using family names or QFamilies)
    cmd = ['qdrop']

    if len(qpus_json) != 0:
        for family in families:
            if isinstance(family, str):
                for _, dictionary in qpus_json.items():
                    if dictionary.get("family") == family:
                        job_id=dictionary.get("slurm_job_id")   
                        cmd.append(str(job_id)) 
                        break #pass to the next family name (two qraises must have different family names)

            elif isinstance(family, int):
                cmd.append(str(family))
            else:
                logger.error(f"Arguments for qdrop must be strings or QFamilies.")
                raise SystemExit
    else:
        logger.debug(f"qpus.json is empty, the specified families must have reached the time limit.")
 
    run(cmd) #run 'qdrop slurm_jobid_1 slurm_jobid_2 etc' on terminal


def nodeswithQPUs() -> list[set]:
    """
    Global function to know what nodes of the computer host virtual QPUs.

    Return:
        List of the corresponding node names.
    """
    try:
        with open(info_path, "r") as f:
            qpus_json = load(f)

        node_names = set()
        for info in qpus_json.values():
            node_names.add(info["net"]["node_name"])

        return list(node_names)

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level



def infoQPUs(local: bool = True, node_name: str = None) -> list[dict]:
    """
    Global function that returns information about the QPUs available either in the local node or globaly.

    It is possible also to filter by `node_names`. If `local = True` and `node_names` provided are different from the local node, only local node will be chosen.
    """

    try:
        with open(info_path, "r") as f:
            qpus_json = load(f)
            if len(qpus_json) == 0:
                logger.warning(f"No QPUs were found.")
                return
        
        if node_name is not None:
            targets = [{qpu_id:info} for qpu_id,info in qpus_json.items() if (info["net"].get("node_name") == node_name ) ]
        
        else:
            if local:
                local_node = os.getenv("SLURMD_NODENAME")
                logger.debug(f"User at node {local_node}.")
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



def getQPUs(local: bool = True, family: str = None) -> list[QPU]:
    """
    Global function to get the QPU objects corresponding to the virtual QPUs raised.

    Args:
        local (bool): option to return only the QPUs in the current node (True, default option) or in all nodes (False).
        family (str): option to return only the QPUs from the selected family (group of QPUs allocated in the same qraise)

    Return:
        List of QPU objects.
    
    """

    #Access raised QPUs information on qpu.json file
    try:
        with open(info_path, "r") as f:
            qpus_json = load(f)
            if len(qpus_json) == 0:
                logger.error(f"No QPUs were found.")
                raise Exception

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level
    
    logger.debug(f"File accessed correctly.")

    #Extract selected QPUs from qpu.json information 
    if local:
        local_node = os.getenv("SLURMD_NODENAME")
        logger.debug(f"User at node {local_node}.")

        if family is not None:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info["net"].get("nodename") == local_node) and (info.get("family") == family)}
        else:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info["net"].get("nodename") == local_node)}
    else:
        if family is not None:
            targets = {qpu_id:info for qpu_id, info in qpus_json.items() if (info.get("family") == family)}
        else:
            targets = qpus_json
    
    # Create QPU objects from the dictionary information + return them on a list
    qpus = []
    i = 0
    for _, info in targets.items():
        client = QClient()
        endpoint = (info["net"]["ip"], info["net"]["port"])
        qpus.append(QPU(id = i, qclient = client, backend = Backend(info['backend']), family = info["family"], endpoint = endpoint))
        i+=1
    logger.debug(f"{len(qpus)} QPU objects were created.")
    return qpus
