import os, sys
from time import sleep

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator = "Aer", co_located = True)

qpus  = get_QPUs(on_node = False, family = family)

qc = CunqaCircuit(15)
for i in range(14):
    qc.h(i)
    qc.cx(i, i + 1)
qc.measure_all()

qpu = qpus[0]
qjob = qpu.run(qc, transpile = False, shots = 1_000_000) # non-blocking call

counts = qjob.result.counts
time = qjob.time_taken

print(qjob.result)
#print(f"Result: \n{counts}\n Time taken: {time} s.")

########## Drop the deployed QPUs #
qdrop(family)
