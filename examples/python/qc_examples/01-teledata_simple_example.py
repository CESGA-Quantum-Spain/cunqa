import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.mappers import run_distributed
from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True)

circuit1 = CunqaCircuit(2, id = "circuit1") # adding ancilla
circuit2 = CunqaCircuit(1, id = "circuit2")

circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(1, "circuit2")# this qubit that is sent is reset
circuit2.qrecv(0, "circuit1")

circuit1.measure_all()
circuit2.measure_all()

qpus = get_QPUs(on_node=False)

qjobs = run_distributed([circuit1, circuit2], qpus, shots = 100)

resutls = gather(qjobs)

for q in resutls:
    print("Result: ", q.counts)
    print()

qdrop(family)

