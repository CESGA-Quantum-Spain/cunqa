import os, sys
# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator = "Cunqa",  co_located = True)

qpus  = get_QPUs(co_located = True, family = family)

c = CunqaCircuit(2, 2)
c.h(0)
c.measure(0, 0)

with c.cif(0) as cgates:
    cgates.x(1)

c.measure(0,0)
c.measure(1,1)

qpu = qpus[0]
qjob = run(c, qpu, shots = 1024)# non-blocking call
counts = qjob.result.counts

print("Counts: \n\t", counts)

########## Drop the deployed QPUs #
qdrop(family)
