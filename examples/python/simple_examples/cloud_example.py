import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa import get_QPUs, gather
from cunqa.circuit import CunqaCircuit


# --------------------------------------------------
# Key difference between cloud and HPC
# example: local = False. This allows to look for
# QPUs out of the node where the work is executing.
# --------------------------------------------------
qpus  = get_QPUs(local=False)


for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}, simulator: {q.backend.simulator}, version: {q.backend.version}.")

qc = CunqaCircuit(2)
qc.h(0)
qc.cx(0,1)
qc.measure_all()

qjobs = []
for _ in range(1):
    for qpu in qpus: 
        qjobs.append(qpu.run(qc, transpile=True, shots = 100))

results = gather(qjobs)

for result in results:
    print("Resultado: ", result.counts)