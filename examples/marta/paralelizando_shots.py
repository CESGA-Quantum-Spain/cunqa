import os
import sys
import json
import numpy

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
sys.path.insert(0, "/mnt/netapp1/Store_CESGA/home/cesga/mlosada/api/api-simulator/python")


from python.qclient import QClient
from qpu import QPU, getQPUs
from backend import Backend

lista = getQPUs()

print("QPUs we are going to work with: ")
print(" ")
for q in lista:
    print(f"For QPU {q.id_}({q.server_id}), backend {q.backend.name}, {q.backend.description}")

with open("../circuit_10qubits_10layers.json", "r") as file:
    circuit = json.load(file)

# ahora vamos a paralelizar los shots:

qjobs = []
for q in lista:
    qjobs.append(q.run(circuit, shots = 1000))

for qj in qjobs:
    print(qj.result())
    




