from qiskit import QuantumCircuit
import numpy as np
from numpy import random
from translator import from_qc_to_json
import json

N_QUBITS = 2
vector = [random.rand() for _ in range(2**N_QUBITS)]
initial_state = vector/np.linalg.norm(vector)
qc = QuantumCircuit(N_QUBITS,N_QUBITS)
qc.initialize(initial_state, [i for i in range(N_QUBITS)])  
qc_decomposed = qc.decompose().decompose().decompose().decompose()
 
qc_str = from_qc_to_json(qc_decomposed)

#print(qc_str)

with open("prueba.json", "a") as text_file:
    for gate in qc_str["circuit"]:
        text_file.write(json.dumps(gate))


#print(qc_str['circuit'][1]['params'][0].tobytes())

#print(str(qc_str["circuit"][1]["params"][0], encoding='utf-8'))