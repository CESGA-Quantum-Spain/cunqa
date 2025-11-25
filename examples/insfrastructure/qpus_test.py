import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.mappers import run_distributed
from cunqa.qjob import gather

qpus  = get_QPUs(on_node=False)

print([qpu.name for qpu in qpus])