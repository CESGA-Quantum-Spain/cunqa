import os
import sys

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
sys.path.insert(0, "/mnt/netapp1/Store_CESGA/home/cesga/mlosada/api/api-simulator/python")

from qpu import QPU, getQPUs
from qjob import gather
from qiskit import QuantumCircuit
# from scipy.optimize import differential_evolution


lista = getQPUs()

print(f"QPUs we are going to work with ({len(lista)}): ")
print(" ")
for q in lista:
    print(f"QPU {q.id_}: {q.backend.name}")


# we define a circuit, first as a json

circuit = {"num_clbits":2,
    "instructions": [
    {
        "name": "h",
        "qubits": [0]
    },
    {
        "name": "cx",
        "qubits": [0, 1]
    },
    {
        "name": "measure",
        "qubits": [0],
        "memory": [0]
    },
    {
        "name": "measure",
        "qubits": [1],
        "memory": [1]
    }
    ]
}

qjobs = []

for q in lista[:4]:
    qjobs.append(q.run(circ = circuit, shots=222))

for qj in qjobs:
    print("State: ", qj.state())

resultados = gather(qjobs)

for i,r in enumerate(resultados):
    print(f"For QPU {i}: {r.get_counts()}")








