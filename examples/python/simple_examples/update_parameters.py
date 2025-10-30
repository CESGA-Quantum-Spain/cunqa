import os
import sys
import time
import random

from qiskit import QuantumCircuit

sys.path.append(os.getenv("HOME"))
from cunqa.qutils import get_QPUs



qc = {
    "id":"quantumcircuit",
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
        "name":"unitary",
        "params": [[[0.0, 0.0], [0.0, -1.0]], [[0.0, 1.0], [0.0, 0.0]]],
        "qubits": [0]

    },
    {
        "name": "measure",
        "qubits": [0], 
        "clreg":[]
    },
    {
        "name": "measure",
        "qubits": [1], 
        "clreg":[]
    }
    ],
    "has_cc": False,
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    }
}


qc_param = {
    "id":"parametricquantumcircuit",
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
        "qubits": [0], 
        "clreg":[]
    },
    {
        "name": "measure",
        "qubits": [1], 
        "clreg":[]
    }
    ],
    "has_cc": False,
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    }
}


qpus = get_QPUs(on_node=False)

print("QPUs disponibles:")
for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}.")

qpu0 = qpus[-1]

job = qpu0.run(qc_param, shots=19)
print(job.result.counts)


# Upgrade parameters
for _ in range(10):
    new_params = [random.uniform(0.0, 6.283185)]
    print("Parameter: ", new_params[0])
    job.upgrade_parameters(new_params)
    print("Counts: ", job.result.counts)


