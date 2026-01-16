import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.partitioning import union
from pprint import pprint

circuit1 = CunqaCircuit(2, id = "circuit1") # adding ancilla
circuit2 = CunqaCircuit(1, id = "circuit2")
circuit1.h(0)
circuit1.cx(0,1)
circuit1.measure_all()
circuit2.measure_all()
circuit1.send(1, "circuit2")# this qubit that is sent is reset
circuit2.recv(0, "circuit1")

family = qraise(2, "00:10:00", simulator="Aer", classical_comm=True, co_located = True)
qpus = get_QPUs(co_located=True, family=family)
qjobs = run([circuit1, circuit2], qpus, shots=1024)
results = gather(qjobs)

print("--------- Result before union: ---------")
for q in results:
    print("Result: ", q.counts)
    print()
qdrop(family)
print("\n\n")



union_circuit = union([circuit1, circuit2])

family = qraise(1, "00:10:00", simulator="Aer", co_located = True)
[qpu]  = get_QPUs(co_located = True, family = family)
qjob = run(union_circuit, qpu, shots = 1024)# non-blocking call
counts = qjob.result.counts

print("--------- Result before union: ---------")
print("Result: ", counts)
qdrop(family)
