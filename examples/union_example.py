import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.partitioning import union
from pprint import pprint

""" circuit1 = CunqaCircuit(2, id = "circuit1") # adding ancilla
circuit2 = CunqaCircuit(1, id = "circuit2")
circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(1, "circuit2")# this qubit that is sent is reset
circuit2.qrecv(0, "circuit1")
circuit1.measure_all()
circuit2.measure_all()

family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True)
qpus = get_QPUs(co_located=True, family=family)
qjobs = run([circuit1, circuit2], qpus, shots=1024)
results = gather(qjobs)

pprint(circuit1.instructions)
pprint(circuit2.instructions)

print("Result before union:")
for q in results:
    print("Result: ", q.counts)
    print()
qdrop(family)

print("\n\n") """

circuit1 = CunqaCircuit(2, id = "circuit1") # adding ancilla
circuit2 = CunqaCircuit(1, id = "circuit2")
circuit3 = CunqaCircuit(1, id = "circuit3")

circuit1.h(0)
circuit1.cx(0,1)
circuit1.qsend(1, "circuit2")# this qubit that is sent is reset
circuit1.measure_all()

circuit2.qrecv(0, "circuit1")
circuit2.qsend(0, "circuit3")
circuit2.measure_all()

circuit3.qrecv(0, "circuit2")
circuit3.measure_all()

union_circuit = union([circuit1, circuit2])

family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True)
qpus  = get_QPUs(co_located = True, family = family)
qjobs = run([union_circuit, circuit3], qpus, shots = 1024)# non-blocking call
results = gather(qjobs)

print("Result before union:")
for q in results:
    print("Result: ", q.counts)
    print()
#qdrop(family)
