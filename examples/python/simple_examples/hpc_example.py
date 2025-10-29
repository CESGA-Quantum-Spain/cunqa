import os, sys

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa import get_QPUs
from cunqa.circuit import CunqaCircuit


# --------------------------------------------------
# Key difference between co-located and HPC
# example: on_node = True. By default it is the case.
# This allows to look for QPUs out of the node where 
# the work is executing.
# --------------------------------------------------
qpus  = get_QPUs()

for q in qpus:
    print(f"QPU {q.id}, backend: {q.backend.name}, simulator: {q.backend.simulator}, version: {q.backend.version}.")

qc = CunqaCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

qpu = qpus[0]
qjob = qpu.run(qc, shots = 1000)# non-blocking call

counts = qjob.result.counts
time = qjob.time_taken

print(f"Result: \n{counts}\n Time taken: {time} s.")