import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# Raise QPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Qulacs", classical_comm=True, co_located = True)
qpus  = get_QPUs(co_located=True, family = family)

# Circuits to run
c1 = CunqaCircuit(10, 2, id="First")
c1.h(0)
c1.measure(0,0)
c1.send(0, recving_circuit = "Second")
c1.measure(1,1)

c2 = CunqaCircuit(2, 2, id="Second")
c2.recv(0, sending_circuit = "First")
with c2.cif(0) as cgates:
    cgates.x(1)
c2.measure(0,0)
c2.measure(1,1)

# Run and show the circuits 
circs = [c1, c2]
distr_jobs = run(circs, qpus, shots=1000) 
result_list = gather(distr_jobs)

for result in result_list:
    print(result)
qdrop(family)
