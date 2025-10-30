import os
import sys
import json
import time

sys.path.append(os.getenv("HOME"))
from cunqa.qutils import get_QPUs, qraise, qdrop
family = qraise(3, "00:10:00", simulator="Munich", classical_comm=True, co_located = True)

qpus = get_QPUs(on_node=False, family = family)

print("QPUs disponibles:")
for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}")

qpu0 = qpus[0]
qpu1 = qpus[1]
qpu2 = qpus[2]

# TODO: remote_c_if_GATE is no longer an instruction
d_qc_0_zmq = {
    "id": "carballido",
    "instructions": [
        
    {
        "name":"h",
        "qubits":[0],
        "qubits":[0],
    },
    {
        "name":"measure_and_send",
        "qubits":[0],
        "circuits":["losada"]
    },
    {
        "name":"measure_and_send",
        "qubits":[1],
        "circuits":["exposito"]
    },
    {
        "name": "measure",
        "qubits": [0],
        "clbits":[0], 
        "clreg":[]
    },
    {
        "name": "measure",
        "qubits": [1],
        "clbits": [1],
        "clreg":[]
    }
    ],
    "has_cc": True,
    "sending_to":["losada", "exposito"],
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    }
}

d_qc_1_zmq = {
    "id": "losada",
    "instructions": [
    {
        "name":"recv",
        "qubits":[1],
        "circuits":["carballido"],
        "remote_conditional_reg":[1]
    },
    {
        "name":"h",
        "qubits":[0],
        "qubits":[0],
    },
    {
        "name": "measure",
        "qubits": [0],
        "clbits": [0], 
        "clreg":[]
    },
    {
        "name": "measure",
        "qubits": [1],
        "clbits": [1], 
        "clreg":[]
    }
    ],
    "has_cc": True,
    "sending_to":[],
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    }
}

d_qc_2_zmq = {
    "id": "exposito",
    "instructions": [
    {
        "name":"h",
        "qubits":[1]
    },
    {
        "name":"recv",
        "qubits":[0],
        "circuits":["carballido"],
        "remote_conditional_reg":[0]
    },
    {
        "name": "measure",
        "qubits": [0],
        "clbits": [0], 
        "clreg":[]
    },
    {
        "name": "measure",
        "qubits": [1],
        "clbits": [1], 
        "clreg":[]
    }
    ],
    "has_cc": True,
    "sending_to":[],
    "num_qubits": 2,
    "num_clbits": 2,
    "classical_registers": {
        "meas": [
            0,
            1
        ]
    }
}

# print(d_qc_0_zmq)
#TODO: Manage the number of shots authomatically. They have to be the same for all jobs
# print(qpu0._family_name)
# job0 = qpu0.run(d_qc_0_zmq, shots=10) 
# job1 = qpu1.run(d_qc_1_zmq, shots=10)
# job2 = qpu2.run(d_qc_2_zmq, shots=10)

from cunqa.mappers import run_distributed

job0, job1, job2 = run_distributed([d_qc_0_zmq, d_qc_1_zmq, d_qc_2_zmq], qpus[:3], shots = 10)

print("Result QPU0", job0.result.counts)
print("Result QPU1", job1.result.counts)
print("Result QPU2", job2.result.counts)

qdrop(family)









