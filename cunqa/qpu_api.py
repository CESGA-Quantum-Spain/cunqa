import os
import sys
import json
import subprocess
from subprocess import run
from qiskit import QuantumCircuit
from qiskit.circuit.library import QFT
from cunqa.circuit import qc_to_json
from cunqa.qpu import getQPUs

# Adding pyhton folder path to detect modules
INSTALL_PATH = os.getenv("INSTALL_PATH")
sys.path.insert(0, INSTALL_PATH)

class QRaiseError(Exception):
    """Exception for errors during qraise slurm command"""
    pass

def create_QPU(how_many, time, flags = ''):
    """
    Raises a QPU and returns its job_id

    Args
    -----------
    how_many (int): number of QPUs to be raised
    time (str, format: 'D-HH:MM:SS'): maximun time that the classical resources will be reserved for the QPU
    flags (str): any other flag you want to apply. It's empty by default

    Return
    --------------
    job_id (str): sequence of numbers that identifies the slurm job for the QPU
    """
    
    # assert type(how_many) == int,  f'Number must be int, but {type(how_many)} was provided' 
    # time.replace(" ", "") #remove all whitespaces in time
    # assert all([type(time) == str, time[0].isdecimal(), time[1].isdecimal(), time[2] == ':', time[3].isdecimal(), time[4].isdecimal(),  time[2] == ':', time[6].isdecimal(), time[7].isdecimal(), len(time) == 8]), 'Incorrect time format, it should be D-HH:MM:SS'

    try:
        cmd = ["qraise", "-n", str(how_many), '-t', str(time), str(flags)]
        output = run(cmd, capture_output=True).stdout #run the command on terminal and capture ist output on the variable 'output'

        job_id = ''.join(e for e in str(output) if e.isdecimal()) #checks the output on the console (looks like 'Submitted batch job 136285') and selects the number
        return job_id
    
    except Exception as error:
        raise QRaiseError(f"Unable to raise requested QPUs [{error}]")

def qdrop(*job_ids):
    """
    Drops the QPUs corresponding to the the entered job ids. By default, all raised QPUs will be dropped

    Args
    --------
    job_ids (tuple(str)): slurm job ids that will be cancelled if they are a QPU
    """
    
    if len( job_ids ) == 0:
        job_ids = ['--all'] #if no job_id is provided we drop all QPU slurm jobs

    cmd = ['qdrop']
    for job_id in job_ids:
        cmd.append(str(job_id))
    run(cmd) #run 'qdrop slurm_job_id_1 slurm_job_id_2 etc' on terminal
    return