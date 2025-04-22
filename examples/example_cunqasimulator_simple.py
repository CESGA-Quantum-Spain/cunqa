import os
import sys
import time
import random

from qiskit import QuantumCircuit

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
from cunqa.qutils import getQPUs

qc = {
    "instructions": [
    {
        "name":"h",
        "qubits":[0]
    },
    {
        "name":"cx",
        "qubits":[0,1],
    },
    {
        "name": "measure",
        "qubits": [0]
    },
    {
        "name": "measure",
        "qubits": [1]
    }
    ],
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    },
    "exec_type":"dynamic"
}


qc_param = {
    "instructions": [
    {
        "name":"h",
        "qubits":[0]
    },
    {
        "name":"rx",
        "qubits":[1],
        "params":[1.1111]
    },
    {
        "name": "measure",
        "qubits": [0]
    },
    {
        "name": "measure",
        "qubits": [1]
    }
    ],
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    },
    "exec_type":"dynamic"
}


qpus = getQPUs(local=False)

print("QPUs disponibles:")
for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}.")

qpu0 = qpus[-1]

print("QPU endpoint: ", qpu0.endpoint)

job = qpu0.run(qc, shots=19)
print(job.result().get_counts())


# Upgrade parameters
""" for _ in range(10):
    new_params = [random.uniform(0.0, 6.283185)]
    print("Parameter: ", new_params[0])
    param_job = job.upgrade_parameters(new_params)
    print("Counts: ", param_job.result().get_counts()) """


