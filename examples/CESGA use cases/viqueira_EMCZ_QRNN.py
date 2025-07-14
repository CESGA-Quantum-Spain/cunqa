import os, sys
import math
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from viqueira_EMCZ_circuit import CircuitEMCZ
from cunqa.logger import logger
from cunqa.qutils import getQPUs, qraise, qdrop
from cunqa.mappers import run_distributed
from cunqa.qjob import QJob, gather

class ViqueiraEMCZModel:
    """
    Implementation using CUNQA of the Exchange-Memory with Controlled Z-gates model from the paper https://arxiv.org/abs/2310.20671 .
    """

    def __init__(self, x_init: np.array, theta_init: np.array, nE: int, nM: int, nT: int, repeat_encode: int, repeat_evolution: int, shots: Optional[int] = 1000, rseed: Optional[int] = None):
        self.circuit = CircuitEMCZ(nE, nM, nT, repeat_encode, repeat_evolution)