from qiskit import QuantumCircuit
import numpy as np
from numpy import random
from translator import from_qc_to_json
import json


N_QUBITS = 7
vector = [random.rand() for _ in range(2**N_QUBITS)]
initial_state = vector/np.linalg.norm(vector)
qc = QuantumCircuit(N_QUBITS,N_QUBITS)
qc.initialize(initial_state, [i for i in range(N_QUBITS)])  
qc_decomposed = qc.decompose().decompose().decompose().decompose()

qc_str = from_qc_to_json(qc_decomposed)
with open("prueba.json", "a") as text_file:
    text_file.write(json.dumps(qc_str))