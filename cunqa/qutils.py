import os
import sys
import json
import subprocess
from cunqa.logger import logger
from subprocess import run
from cunqa.qpu import getQPUs

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class QRaiseError(Exception):
    """Exception for errors during qraise slurm command"""
    pass

def qraise(n, time, flags = ''):
    """
    Raises a QPU and returns its job_id

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
        raise QRaiseError(f"Unable to raise requested QPUs [{error}]")

def qdrop(*families):
    """
    Drops the QPU families corresponding to the the entered QPU objects. By default, all raised QPUs will be dropped

    Args
    --------
    qpus (tuple(<class cunqa.qpu.QPU>)): list of QPUs to drop. All QPUs that share a qraise will these will drop
    """
    
    #if no QPU is provided we drop all QPU slurm jobs
    if len( families ) == 0:
        qpus = ['--all'] 


    # path to access to json file holding information about the raised QPUs
    info_path = os.getenv("INFO_PATH")
    if info_path is None:
        STORE = os.getenv("STORE")
        info_path = STORE+"/.api_simulator/qpus.json"


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