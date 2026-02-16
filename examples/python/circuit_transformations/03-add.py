import os, sys

home = os.getenv("HOME")
sys.path.append(home)

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import add
from pprint import pprint

# ---------------------------
# Adquiring the resources
# ---------------------------
family = qraise(1, "00:10:00", simulator="Aer", co_located=True)
[qpu] = get_QPUs(co_located=True, family=family)

# ---------------------------
# Original circuits are:
#
#    | circuit1 | circuit2 |
# q0:|   ─[H]─  |   ──●──  |
#    |          |     |    |
# q1:|          |   ─[X]─  |
# 
# And, then, added_circuit is a Bell pair:
#
# added_circuit.q0: ─[H]──●──[M]─
#                         |
# added_circuit.q1: ─────[X]─[M]─
# ---------------------------
circuit1 = CunqaCircuit(1, id = "circuit1") # adding ancilla
circuit1.h(0)

circuit2 = CunqaCircuit(2, id = "circuit2")
circuit2.cx(0,1)

added_circuit = add([circuit1, circuit2])
added_circuit.measure_all()

qjob = run(added_circuit, qpu, shots = 1024)# non-blocking call
results = qjob.result

print(f"\nResult after addition: {results.counts}\n")

# ---------------------------
# Relinquishing of the resources
# ---------------------------
qdrop(family)