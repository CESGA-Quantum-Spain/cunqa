import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME") + "/.local")

from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit


# --------------------------------------------------
# Key difference between co-located and HPC
# example: on_node = False. This allows to look for
# QPUs out of the node where the work is executing.
# --------------------------------------------------
qpus  = get_QPUs(on_node=False)


for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}, simulator: {q.backend.simulator}, version: {q.backend.version}.")

qc = CunqaCircuit(2)
# Parámetros
theta1 = np.pi / 4
theta2 = np.pi / 3
theta3 = 2 * np.pi / 3

# 1) RY en qubit 0
qc.ry(theta1, 0)

# 2) Control invertido para |0>_0
qc.x(0)
qc.cry(theta2, 0, 1)
qc.x(0)

qc.measure_all()

qjobs = []
for _ in range(1):
    for qpu in qpus: 
        qjobs.append(qpu.run(qc, transpile=True, shots = 100))

print("Waiting for the results...")
results = gather(qjobs)

for result in results:
    print("Result: ", result.counts)