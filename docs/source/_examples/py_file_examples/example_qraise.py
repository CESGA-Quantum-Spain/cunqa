import os, sys
from time import sleep

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator = "Munich", co_located = True)
sleep(15)

qpus  = get_QPUs(on_node = False, family = family)

qc = CunqaCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

qpu = qpus[0]
qjob = qpu.run(qc, shots = 1000)# non-blocking call

counts = qjob.result.counts
time = qjob.time_taken

print(f"Result: \n{counts}\n Time taken: {time} s.")

########## Drop the deployed QPUs #
qdrop(family)
