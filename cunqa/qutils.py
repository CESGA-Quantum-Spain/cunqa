import os
import sys
from subprocess import run
import json
from json import JSONDecodeError, load
from cunqa.qclient import QClient  # importamos api en C++
from cunqa.backend import Backend
from cunqa.logger import logger
from cunqa.qpu import QPU

# Adding pyhton folder path to detect modules
sys.path.insert(0,os.getenv("INSTALL_PATH"))

info_path = os.getenv("INFO_PATH")
if info_path is None:
    STORE = os.getenv("STORE")
    info_path = STORE+"/.cunqa/qpus.json"

class QRaiseError(Exception):
    """Exception for errors during qraise slurm command"""
    pass





def qraise(n, time, flags = ''):
    """
    Raises a QPU and returns its job_id.

    Args
    -----------
    n (int): number of QPUs to be raised
    time (str, format: 'D-HH:MM:SS'): maximun time that the classical resources will be reserved for the QPU
    flags (str): any other flag you want to apply, empty by default. Ex: --fakeqmio, --sim=Munich, --backend="path/to/backend", --family_name="carballido", ...

    """

    try:
        cmd = ["qraise", "-n", str(n), '-t', str(time), str(flags)]
        output = run(cmd, capture_output=True).stdout #run the command on terminal and capture ist output on the variable 'output'

        # job_id = ''.join(e for e in str(output) if e.isdecimal()) #checks the output on the console (looks like 'Submitted batch job 136285') and selects the number
        # return job_id
    
    except Exception as error:
        raise QRaiseError(f"Unable to raise requested QPUs [{error}].")

def qdrop(*families):
    """
    Drops the QPU families corresponding to the the entered QPU objects. By default, all raised QPUs will be dropped.

    Args
    --------
    qpus (tuple(<class cunqa.qpu.QPU>)): list of QPUs to drop. All QPUs that share a qraise will these will drop.
    """
    
    #if no QPU is provided we drop all QPU slurm jobs
    if len( families ) == 0:
        job_id = ['--all'] 


    #access the large dictionary containing all QPU dictionaries
    try:
        with open(info_path, "r") as qpus_json:
            dumps = json.load(qpus_json)

    except Exception as error:
        logger.error(f"Some exception occurred while retrieving the raised QPUs [{type(error).__name__}].")
        raise SystemExit # User's level
    
    logger.debug(f"qpu.json file accessed correctly.")


    #building the terminal command to drop the specified families
    cmd = ['qdrop']
    if len(dumps) != 0:
        for family in families:
            for _, dictionary in dumps.items():
                if dictionary.get("family").get("family_name") == family:
                    job_id=dictionary.get("family").get("slurm_job_id")
                    cmd.append(str(job_id)) 
                    break #pass to the next family name
    else:
        logger.debug(f"qpus.json is empty, the specified families must have.")
        
    
    run(cmd) #run 'qdrop slurm_job_id_1 slurm_job_id_2 etc' on terminal

    return


def nodeswithQPUs():
    """
    Global function to know what nodes of the computer host virtual QPUs.

    Return:
    ---------
    List of the corresponding node names.
    """
    try:
        with open(info_path, "r") as qpus_json:
            dumps = json.load(qpus_json)

        node_names = set()

        for v in dumps.values():
            node_names.add(v["net"]["node_name"])

        return list(node_names)

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level



def infoQPUs(local = True, node_name = None):
    """
    Global function that returns information about the QPUs available either in the local node or globaly.

    It is possible also to filter by `node_names`. If `local = True` and `node_names` provided are different from the local node, only local node will be chosen.
    """

    try:
        with open(info_path, "r") as qpus_json:
            dumps = load(qpus_json)
            if len(dumps) == 0:
                logger.warning(f"No QPUs were found.")
                return
        
        if node_name is not None:
            targets = [{k:q} for k,q in dumps.items() if (q["net"].get("node_name") == node_name ) ]
        
        else:
            if local:
                local_node = os.getenv("SLURMD_NODENAME")
                logger.debug(f"User at node {local_node}.")
                # filtering by node_name to select local node
                targets = [{k:q} for k,q in dumps.items() if (q["net"].get("node_name")==local_node) ]
            else:
                targets =[{k:q} for k,q in dumps.items()]
        
        info = []
        for t in targets:
            key = list(t.keys())[0]
            info.append({
                "QPU":key,
                "node":t[key]["net"]["node_name"],
                "family_name":t[key]["family_name"],
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



def getQPUs(local = True, family_name = None):
    """
    Global function to get the QPU objects corresponding to the virtual QPUs raised.

    Return:
    ---------
    List of QPU objects.
    
    """

    try:
        with open(info_path, "r") as qpus_json:
            dumps = load(qpus_json)
            if len(dumps) == 0:
                logger.error(f"No QPUs were found.")
                raise Exception

    except Exception as error:
        logger.error(f"Some exception occurred [{type(error).__name__}].")
        raise SystemExit # User's level
    
    logger.debug(f"File accessed correctly.")



    if local:
        local_node = os.getenv("SLURMD_NODENAME")
        logger.debug(f"User at node {local_node}.")

        if family_name is not None:
            targets = { k:q for k,q in dumps.items() if (q.get("node_name")==local_node) and (q.get("family_name") == family_name) }
        
        else:
            targets = {k:q for k,q in dumps.items() if (q.get("node_name")==local_node)}

    else:
        if family_name is not None:
            targets = {k:q for k,q in dumps.items() if (q.get("family_name") == family_name) }
        
        else:
            targets = dumps
    
    qpus = []
    i = 0
    for k, v in targets.items():
        client = QClient(info_path)
        qpus.append(  QPU(id = i, qclient = client, backend = Backend(v['backend']), family_name = v["family_name"], port = k  )  ) # errors captured above
        i+=1
    logger.debug(f"{len(qpus)} QPU objects were created.")
    return qpus
