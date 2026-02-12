import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import add
from pprint import pprint

circuit1 = CunqaCircuit(1, id = "circuit1") # adding ancilla
circuit1.h(0)

circuit2 = CunqaCircuit(1, id = "circuit2")
circuit2.cx(0,1)
circuit2.qsend(0, "circuit3")

circuit3 = CunqaCircuit(1, id = "circuit3")
circuit3.qrecv(0, "circuit2")
circuit3.measure_all()

union_circuit = add([circuit1, circuit2])
union_circuit.measure_all()
print(union_circuit.num_clbits)

family = qraise(2, "00:10:00", quantum_comm=True, simulator="Aer", co_located = True)
qpus  = get_QPUs(co_located = True, family = family)
qjobs = run([union_circuit, circuit3], qpus, shots = 1024)# non-blocking call
results = gather(qjobs)

pprint(union_circuit.instructions)
pprint(circuit3.instructions)

print("--------- Result after addition: ---------")
for q in results:
    print(f"Result: {q.counts}\n")

qdrop(family)