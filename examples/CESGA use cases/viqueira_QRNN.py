import os, sys
import math
import numpy as np
import matplotlib.pyplot as plt

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

def create_EMC_ansatz(x, theta):
    """
    Function that creates the ansatz for the 

    Args:
        theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Order: see 'encode' and 'evolve' methods.
        x (numpy.array): Input data. Its shape must be (nT, nE), where this is the number of qubits on each register of the EMC QRNN circuit (function below)
    Return:
        ansatz (CunqaCircuit): circuit implementing the given parameters on the ansatz
    """
    nT, nE = np.shape(x)
    ansatz = CunqaCircuit(nT + nE, nT + nE)

    return ansatz

def CircuitEMC(nE, nM, nT):
    """
    Function to create a EMC QRNN circuit. This circuit modifies a time series (sequence of data where each point depends on the ones before it)
    to obtain another time series after executing. Doing this recursively constitutes the Exchange-Memory w Controlled gates algorithm (https://arxiv.org/abs/2310.20671).

    Args:
        nE (int): number of qubits for the Environment/Exchange register
        nM (int): number of qubits for the Memory register
        nT (int): number of time steps of the time series
    Return:
        circuit (CunqaCircuit): circuit implementing the QRNN structure
    """

    circuit = CunqaCircuit(nE + nM, nE + nM)

    for time_step in range(nT):
        blabla

    return circuit