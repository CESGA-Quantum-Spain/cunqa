"""
 Title: Gradients and Cost Functions
 Description: class implementing all gradient calculation methods + cost functions of interest

Created 15/07/2025
@author: dexposito (algorithm idea: jdviqueira)
"""

import os, sys
import math
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from typing import  Union, Any, Optional

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from viqueira_EMCZ_circuit import CircuitEMCZ
from cunqa.logger import logger
from cunqa.qutils import getQPUs, qraise, qdrop, QRaiseError
from cunqa.mappers import run_distributed
from cunqa.qjob import QJob, gather
