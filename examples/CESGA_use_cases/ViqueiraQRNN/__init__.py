"""Module implementation of a Quantum Recurrent Neural Network model, by default the one from https://arxiv.org/abs/2310.20671"""
from viqueira_QRNN_circuit import CircuitQRNN
from viqueira_gradients_and_costs import GradientMethod, CostFunction
from viqueira_QRNN_model import ViqueiraQRNN

import sys, os
sys.path.append(os.getenv("HOME"))
import cunqa