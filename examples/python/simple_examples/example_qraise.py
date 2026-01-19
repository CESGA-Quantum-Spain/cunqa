import os, sys
from time import sleep

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator = "Qulacs", co_located = True)

qpus  = get_QPUs(on_node = False, family = family)

qc = CunqaCircuit(5)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

qpu = qpus[0]
qjob = qpu.run(qc, transpile = False, shots = 10) # non-blocking call


counts = qjob.result.counts
time = qjob.time_taken

print(qjob.result)

########## Drop the deployed QPUs #
qdrop(family)
