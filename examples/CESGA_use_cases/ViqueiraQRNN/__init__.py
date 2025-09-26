"""Module implementation of a Quantum Recurrent Neural Network model, by default the one from https://arxiv.org/abs/2310.20671"""
import sys, os
sys.path.append("/mnt/netapp1/Store_CESGA/home/cesga/dexposito/repos/CUNQA/examples/CESGA_use_cases/")

from ViqueiraQRNN.circuit import CircuitQRNN
from ViqueiraQRNN.gradients_and_costs import GradientMethod, CostFunction
from ViqueiraQRNN.model import ViqueiraQRNN
from ViqueiraQRNN.ansatz import AnsatzQRNN, EMCZ2, EMCZ3


sys.path.append(os.getenv("HOME"))
import cunqa